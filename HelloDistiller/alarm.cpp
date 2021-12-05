// Последнее обновление 2017-05-31 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам
// Здесь обработка всяких страшилок

#include "configuration.h"
#include "declarations.h"

void ReadAlarm()
{
    boolean UprNBK;
    ProcessNPG();

    // Если уровень тревоги равен единице, то считаем, что датчик цифровой
    if (USE_ALARM_VODA) {
        if (UROVEN_ALARM != 1)
            U_VODA = analogRead(PIN_ALARM_VODA);
        else
            U_VODA = !digitalRead(PIN_ALARM_VODA); // Датчики влажности инверсные, то есть при не сработавшем состоянии у них на выходе 1, при сработке 0

        if (U_VODA >= UROVEN_ALARM) {
            CountAlarmVoda++;

#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("ALARM U_VODA=%i cnt=%i"), U_VODA, CountAlarmVoda);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif

            if (CountAlarmVoda > COUNT_ALARM) {
                DispDopInfo = 2;
                if (IspReg == 129)
                    DispPage = 4;

                if (flNeedCall == 0)
                    flNeedCall = 1;
                // По прошествии минуты устанавливаем флаг сработки.
                if (CountAlarmVoda > COUNT_ALARM_VODA) {
                    DispDopInfo = 0;
#ifndef TEST
#ifndef DEBUG
#ifndef TESTGSM
#ifndef TESTGSM1
                    IspReg = 250; // Переводим контроллер в режим тревоги, если не установлен тестовый режим.
#endif
#endif
#endif
#endif
                }
            }
        } else {
            if (DispDopInfo == 2) {
                flNeedCall = 0;
                DispDopInfo = 0;
            }
            CountAlarmVoda = 0;
        }
    }

    if (USE_ALARM_UROVEN) {
        if (UROVEN_ALARM > 1) {
            // Считаваем значение аналоговое  датчика
            U_UROVEN = analogRead(PIN_ALARM_UROVEN);
        } else {
            // Считаваем значение цифрового датчика
            // Датчики влажности инверсные, то есть при не сработавшем состоянии у них на выходе 1, при сработке 0
            U_UROVEN = !digitalRead(PIN_ALARM_UROVEN);
        }

        // Для фракционной перегонки первой фракции считаем сработкой датчика уровня также и сработку датчика голов.
        // Чтобы можно было отбирать головы через один клапан, тело через другой без фракционника.
        if (IspReg == 117 && TekFraction == 0) {
            if (UROVEN_ALARM != 1)
                U_UROVEN = U_UROVEN + analogRead(PIN_PROVODIMOST_SR);
            else
                U_UROVEN = U_UROVEN || !digitalRead(PIN_PROVODIMOST_SR); // Датчики влажности инверсные, то есть при не сработавшем состоянии у них на выходе 1, при сработке 0
        }
        // Предупреждение по уровню генерируем только уже не произошла сработка уроня
        // Если произошла, то уже смысла анализировать нет
        if (U_UROVEN >= UROVEN_ALARM && flAlarmUroven == 0) {
            CountAlarmUroven++;
#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("ALARM U_UROVEN=%i cnt=%i"), U_UROVEN, CountAlarmUroven);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif
            if (CountAlarmUroven > COUNT_ALARM) {
                DispDopInfo = 1;
                if (flNeedCall == 0)
                    flNeedCall = 1;
                // По прошествии минуты устанавливаем флаг сработки.
                if (CountAlarmUroven > COUNT_ALARM_UROVEN || ((IspReg == 117 || IspReg == 118) && CountAlarmUroven > COUNT_ALARM_UROVEN_FR)) {
                    flAlarmUroven = 1;
                    flNeedAnalyse = 1;
                    DispDopInfo = 0;
                }
            }
        } else {
            if (DispDopInfo == 1) {
                flNeedCall = 0;
                DispDopInfo = 0;
            }
            CountAlarmUroven = 0;
        }
    }

    if (USE_GAS_SENSOR) {
        if (UROVEN_GAS_SENSOR != 1)
            U_GAS = analogRead(PIN_GAS_SENSOR);
        else
            U_GAS = digitalRead(PIN_GAS_SENSOR);

        //      Если датчик еще не прогрелся, то выводим значение времени, оставшееся для прогрева датчика паров спирта
        if (Seconds < TIME_PROGREV_GAS_SENSOR)
            U_GAS = TIME_PROGREV_GAS_SENSOR - Seconds;
        else {

            if (U_GAS >= UROVEN_GAS_SENSOR) {
                CountGasSensor++;
#ifdef TEST
                sprintf_P(my_tx_buffer, PSTR("ALARM U_GAS=%i cnt=%i"), U_GAS, CountGasSensor);
                DEBUG_SERIAL.println(my_tx_buffer);
#endif

                if (CountGasSensor > COUNT_GAS_SENSOR) {
                    flNeedAnalyse = 1;
#ifndef TEST
                    IspReg = 252; // Переводим контроллер в режим тревоги, если не установлен тестовый режим.
#endif
                }
            } else
                CountGasSensor = 0;
        }
    }

#if USE_MPX5010_SENSOR && defined(PIN_MPX5010)
    U_MPX5010 = analogRead(PIN_MPX5010);
#if SIMPLED_VERSION == 20
    DEBUG_SERIAL.println(U_MPX5010);
    if (UstPowerReg != (U_MPX5010)) {
        UstPowerReg = U_MPX5010;
        flNeedAnalyse = 1;
    }
#endif

    // Updated on 2018-12-20 by Phisik
    // https://www.nxp.com/files-static/sensors/doc/data_sheet/MPX5010.pdf
    // Nominal Transfer Value:    P[kPa] = (Vout/Vs-0.04)/0.09
    //
    // Vout = analogRead(PIN_MPX5010);
    // Vs   = 1024;
    //
    // P[kPa]  = (100*Vout/1024-4)/9;
    // P[mmHg]    = P[kPa]*15/2;      //  1 mmHg  = 1 kPa  x 7.50062

    // C++ integer code
    // U_MPX5010  = 10 * (100*Vout/1023-4)/9 * 15 / 2

    //U_MPX5010 = int(((long(15000 * U_MPX5010) >> 10) - 600) / 18);

    // Simplified msg31 equation gives 1-2% error
    U_MPX5010 = (U_MPX5010 * 4 - 160) / 5;

    if (FlState != 253)
        U_MPX5010 = U_MPX5010 + P_MPX5010;

    if (U_MPX5010 > MaxPressByPeriod)
        MaxPressByPeriod = U_MPX5010;

    if (U_MPX5010 >= AlarmMPX5010 && AlarmMPX5010 > 0) {
        CountAlarmMPX5010++;
#ifdef TEST
        sprintf_P(my_tx_buffer, PSTR("ALARM MPX5010=%i cnt=%i"), AlarmMPX5010, CountAlarmMPX5010);
        DEBUG_SERIAL.println(my_tx_buffer);
#endif

        if (CountAlarmMPX5010 > COUNT_ALARM) {
            DispDopInfo = 5;
            if (flNeedCall == 0)
                flNeedCall = 1;
            // По прошествии минуты устанавливаем флаг сработки.
            if (CountAlarmMPX5010 > MAX_ERR_MPX5010) {
                flAlarmMPX5010 = 1;
                flNeedAnalyse = 1;
                DispDopInfo = 0;
            }
        }
    } else {
        if (DispDopInfo == 5) {
            DispDopInfo = 0;
            flNeedCall = 0;
        }
        CountAlarmMPX5010 = 0;
    }

#endif // #if  USE_MPX5010_SENSOR && defined(PIN_MPX5010)

    if (UrovenProvodimostSR != 0 || IspReg == 112 || FlState == 232 || IspReg == 129) // Читаем датчик уровня голов, если установлено, что его надо читать, либо если в данный момент используется режим НБК.
    {
        if (UrovenProvodimostSR != 1)
            U_GLV = analogRead(PIN_PROVODIMOST_SR);
        else
            U_GLV = !digitalRead(PIN_PROVODIMOST_SR); // Датчики влажности инверсные, то есть при не сработавшем состоянии у них на выходе 1, при сработке 0
    }

    // Для НБК датчик голов служит датчиком заполнения барды.
    if (IspReg == 112 || IspReg == 129) // Если режим - НБК, то открываем слив барды
    {
        if (timeBRD == 0) // Если датчик уже нужно анализировать
        {
            if (U_GLV >= UrovenBarda) // Если датчик НБК сработал, то открываем клапан слива барды
            {
                KlOpen[KLP_BARDA] = PER_KLP_OPEN;
                KlClose[KLP_BARDA] = PER_KLP_CLOSE;
                if (timeOpenBRD > 0)
                    timeBRD = timeOpenBRD;
                else
                    timeBRD = 2;

            } else // Если не сработал, то закрываем.
            {
                if (IspReg != 129) { // Клапан барды в режиме тестирования не закрываем.е
                    KlOpen[KLP_BARDA] = 0;
                    KlClose[KLP_BARDA] = 100;
                    UprNBK = LOW;
                    if (!(UprNasosNBK & B00000010))
                        UprNBK = !UprNBK;
                    digitalWrite(PIN_NASOS_NBK_BRD, UprNBK);
                }
            }
        }
    }
}
