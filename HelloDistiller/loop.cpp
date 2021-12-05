// Последнее обновление 2018-07-25 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам
// Основной цикл программы

#include "configuration.h"
#include "declarations.h"

char my_tx_buffer[MY_TX_BUFFER_SIZE];
char my_rx_buffer[MY_RX_BUFFER_SIZE];

void GetStateSerial()
{
    sprintf_P(my_tx_buffer, PSTR("%lu\tT=\t%3u\t%3u\t%3u\tGlv=%4u\tI=%3u\tS=%3u\t%4uW\tOFF=%1i\t%3imm\t%3uV"), Seconds, DS_TEMP(TEMP_KUB), DS_TEMP(TEMP_DEFL), DS_TEMP(TEMP_TSA), U_GLV, IspReg, StateMachine, UstPower, (int)flAllOff, U_MPX5010, MaxVoltsOut);
    DEBUG_SERIAL.println(my_tx_buffer);
}

void GetStateSerialErr()
{
    sprintf_P(my_tx_buffer, PSTR("Err ds18b20 CntDs=%3u NumDs=%3u"), CntErrDs18, NumErrDs18);
    DEBUG_SERIAL.println(my_tx_buffer);
}

#define FILTERSHIFT 13 // for low pass filters to determine ADC offsets
#define FILTERROUNDING (1 << 12)

int voltsOffset = 512;
static long fVoltsOffset = 512L << 13;
int ampsOffset = 512;
static long fAmpsOffset = 512L << 13;

void loop()
{
    int i, j;
    char k;
    unsigned long SqNapr = 0, SqNaprPrev = 0, FindPower, FindPowerKLP;
    int FindKtKLP;
    byte data[12];
    byte present = 0;
    unsigned int raw;
    int raw1;
    byte ds18b20CRC;
    //  static char  FlFindPower;
    //  static char  FlFindPowerKLP;
    char flErrDs18;
    //unsigned int tic;
    static char trig_on = true; //Признак того, что выдаем в данном цикле - 0 или 1

    static uint32_t lastSensorCheckTime = 0;
    // Раз в 5 секунд проверяем датчики, если их нет
    if (ds1820_devices < 1 && millis() > lastSensorCheckTime + 60000L) {
        ds1820_devices = 0;
        while (ds.search(ds1820_rom_codes[ds1820_devices])) {
            ds1820_devices++;
            if (ds1820_devices >= MAX_DS1820)
                break;
        }
        ds.reset_search();
        lastSensorCheckTime = millis();
    }

    if (bLCDclearFlag) {
        dirtyTrickLcdClear();
        bLCDclearFlag = false;
    }

#ifdef DEBUG
    my_debug();
#endif

    //  static int PowerPrev=0;
    //  static int UstPowerPrev=0;
    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    // print the number of seconds since reset:
    // Если есть необходимость произвести отображениеш  информации на дисплее и среднеквадратичное уже рассчитано.
    // И контроллен находится в режиме "лазанья" по меню, тогда отображаем информацию.
    // Или контроллер не находится в режиме ошибки.
    // Обработка уровней и НПГ
    if (flNeedReadAlarm && StateVolts != 1) {
        //    #ifndef DEBUG
        ReadAlarm();
        //    #endif
        flNeedReadAlarm = 0;
    }

#if USE_MQTT_BROKER
    // Обмен данными с ESP для работы с протоколом MQTT
    handleMqttSerial();
#endif // USE_MQTT_BROKER

    // Для обеспечения равномерности выборки делаем это в цикле подряд.
    while (StateVolts == 1) {
#ifdef DEBUG
        if (DEBUG_SERIAL.available())
            return;
#endif

#if USE_BRESENHAM_ASC712 == 0
        // если датчик тока не используем, тогда читаем значение и делим его на два
        if (!flCorrASC712) {
#if ZMPT101B_MODULE_ENABLE
            // Phisik: по сравнению со старой схемой датчика, ZMPT101B выдет синус с
            // амплитудой ~1,25В вокруг U/2. Поэтому, чтобы не менять остальной код,
            // мы смещаем середину в ноль и умножаем показания на 1.5

            // Отклонение от среднего
            resultU = analogRead(PIN_READU) - voltsOffset;

            // Пересчитываем среднее для напряжения
            // https://learn.openenergymonitor.org/electricity-monitoring/ctac/digital-filters-for-offset-removal
            fVoltsOffset += resultU; // update the filter
            voltsOffset = (int)((fVoltsOffset + FILTERROUNDING) >> FILTERSHIFT);

            //  Целочисленно умножаем показания на 1.5
            resultU += resultU >> 1; // resultU = resultU + resultU/2
#else
            // читаем показания с pin А3 и делим их на два, для увеличения
            // измеряемого выходного напряжения.
            resultU = (analogRead(PIN_READU) >> 1);
#endif
        } else {
            if (StateVolts == 1) {
                resultU = analogRead(PIN_READI) - ampsOffset; // читаем показания с датчика тока.

                // Пересчитываем среднее для тока
                // https://learn.openenergymonitor.org/electricity-monitoring/ctac/digital-filters-for-offset-removal
                fAmpsOffset += resultU; // update the filter
                ampsOffset = (int)((fAmpsOffset + FILTERROUNDING) >> FILTERSHIFT);
            }
        }

#else
        // Если используем датчик тока, тогда читаем значение, если оно больше 512, значит приводим его к 512;
        resultU = analogRead(PIN_READI) - ampsOffset; // читаем показания с датчика тока.
        // if (resultU<0) resultU=-resultU;

        // Пересчитываем среднее для тока
        // https://learn.openenergymonitor.org/electricity-monitoring/ctac/digital-filters-for-offset-removal
        fAmpsOffset += resultU; // update the filter
        ampsOffset = (int)((fAmpsOffset + FILTERROUNDING) >> FILTERSHIFT);
#endif

        //      data_adc[index_input]=resultU;
        // Пробуем расчет квадратов на лету.
        SqNaprT = SqNaprT + (unsigned long)resultU * (unsigned long)resultU;

#ifdef TESTRM
        //        tic=(int) TCNT4L | (int) (TCNT4H<<8); // Расчитывааем текущее значение таймера
        //        tic_adc[index_input]=tic;
#endif
        if (index_input < MAX_INDEX_INPUT)
            index_input++;
        else
            StateVolts = 0; // Если превышер размер массива, ставим флаг расчета заново.
    }

    if (NeedDisplaying && (StateVolts != 1 || IspReg >= 240) && FlState == 0) {
        DisplayData();
    }

    // Сканируем клавиатуру, если установлен флаг.
    if (flNeedScanKbd == 1) {
        ScanKbd();
        flNeedScanKbd = 0;
    }

    if (flNeedTemp == 1 && IspReg >= 240) {
        flNeedAnalyse = 1;
    }

    if (StateVolts != 1) {
#if USE_BMP280_SENSOR
        if (flReadPress > timePressAtm) {
            flReadPress = 0;
            if (timePressAtm >= 30) {
                PressAtm = (int)(bmp.readPressure() / 133.3224);
            }

            if (PressAtm >= 640 && PressAtm <= 820) {
                for (i = 0; i < ds1820_devices; i++) {
                    if (timePressAtm != 0) {
                        raw1 = (PressAtm - 760) * 37;
                        raw1 = -raw1 / 10;

                        if (abs(raw1) % 10 >= 5) {
                            raw1 = raw1 / 10;
                            if (raw1 > 0)
                                raw1 = raw1 + 1;
                            else
                                raw1 = raw1 - 1;
                        } else {
                            raw1 = raw1 / 10;
                        }

                        ds1820_poprPress[i] = (char)raw1;

                        //Serial.println((int)ds1820_poprPress[i]);
                    } else
                        ds1820_poprPress[i] = 0;
                }
            }
        }
#endif

        if (flNeedTemp == 1) {
            flNoPhase = 1; // временно убираем с фазового управления клапана, чтобы они не закрылись
            OpenKLP(); // открываем на всякий случай все клапана, которые необходимо открыть.
            if (StepOut == 0) {
                ds.reset();
                ds.write(0xCC, 1); // start conversion, with parasite power on at the end
                ds.write(0x44, 1); // start conversion, with parasite power on at the end
                flNeedTemp = 0;
                for (j = 0; j < ds1820_devices; j++)
                    ds1820_flread[j] = 0;
                StepOut = 1;
            } else {

                flErrDs18 = 0; // Сбрасываем флаг nошибки датчика ds18b20
                for (j = 0; j < ds1820_devices; j++) {
                    // По каждому датчику температуру считаем три раза
                    k = 0;
                    while (k < 3) {
                        if (ds1820_flread[j] == 0) {
                            present = ds.reset();

#ifdef TIMING
                            TCNT4H = 0x00;
                            TCNT4L = 0x00;
#endif

                            // Здесь вообще засада, передача кода датчика занимает около 1600 тиков таймера, что гарантирует пропуск регуляции как минимум одного полупериода.
                            // Возможно решение - посадить каждый датчик на отдельный ПИН, тогда код датчика передавать не нужно.

                            // Phisik: просто нужно делать все асинхронно, а не ждать пока рак на горе свистнет. Послали в шину флаг чтения. На следующем loop-e считали значение(я)

                            ds.select(ds1820_rom_codes[j]);

#ifdef TIMING
                            unsigned int Tic2 = (int)TCNT4L | (int)(TCNT4H << 8); // Расчитывааем текущее значение таймера
                            sprintf_P(my_tx_buffer, PSTR("TicDs18Select=%i"), Tic2);
                            FlUsart = 1;
#endif

                            ds.write(0xBE); // Read Scratchpad
                            //          DEBUG_SERIAL.print(present,HEX);
                            //          DEBUG_SERIAL.print(F(" "));
                            //        Читаем пока только 2 байта из 9, остальные не трогаем.
                            for (i = 0; i < 9; i++) {
                                data[i] = ds.read();
                                //            DEBUG_SERIAL.print(data[i], HEX);
                                //            DEBUG_SERIAL.print(F(" "));
                            }
                            //          DEBUG_SERIAL.print(F(" CRC="));
                            ds18b20CRC = OneWire::crc8(data, 8);

                            if (ds18b20CRC != data[8]) //  Ошибка чтения, записываем признак ошибки
                            {
#ifndef DEBUG
                                if (!flErrDs18 && k == 2) // Если ранее в этом сеансе не было ошибок чтения датчика, то прибавляем количество ошибок чтения на единицу
                                {
                                    flErrDs18 = 1;
                                    CntErrDs18++;
                                    NumErrDs18 = j + 1; // Запоминаем номер сглючившего датчика.
                                    if (CntErrDs18 > 30)
                                        DispDopInfo = 4; // Устанавливаем признак вывода информации об ошибке если ошибок больше 4
                                    if (FlToUSART)
                                        GetStateSerialErr();
                                }
#endif
                            } else // Если контрольная сумма совпадает, то считаем, что все нормально.
                            {
                                raw = (data[1] << 8) | data[0];
                                if (raw & 0x8000) // Если там единица - число отрицательное и его надо преобразовать
                                { // Стандартное преобразование отрицательного числа, которое в микроконтроллере в дополнительной кодировке
                                    raw = (raw ^ 0xffff) + 1; // Путем исключающего ИЛИ плюс единица
                                    raw1 = raw * 5 / 8;
                                    raw1 = -raw;
                                } else
                                    raw1 = raw * 5 / 8;
                                // Температуру равной 850 считаем ошибочной.
                                if (raw1 == 850 || raw1 > 1200) {
                                    // Если температура скаканула больше чем на 10 градусов, то считаем ошибкой.
                                    if (abs(raw1 - temps[j]) > 100 && temps[j] != 0 || raw1 > 1200) {
                                        if (!flErrDs18 && k == 2) // Если ранее в этом сеансе не было ошибок чтения датчика, то прибавляем количество ошибок чтения на единицу
                                        {
                                            flErrDs18 = 1;
                                            CntErrDs18++;
                                            NumErrDs18 = j + 1; // Запоминаем номер сглючившего датчика.
                                            if (CntErrDs18 > 30)
                                                DispDopInfo = 4; // Устанавливаем признак вывода информации об ошибке если ошибок больше 4
                                            if (FlToUSART)
                                                GetStateSerialErr();
                                        }
                                    }
                                } else

                                {
#ifndef DEBUG
                                    // Если температуру считали успешно, то поставим соотв флаг.
                                    ds1820_flread[j] = 1;
                                    k = 3; // Выходим из цикла чтения
                                    temps[j] = raw1;
                                    // Если мы сейчас не находимся в режиме ввода поправок к датчикам, то прибавляем к температуре значение поправки

                                    if (FlState != 300) {
#if USE_BMP280_SENSOR
                                        temps[j] = temps[j] + (int)ds1820_popr[j] + (int)ds1820_poprPress[j];
#else
                                        temps[j] = temps[j] + (int)ds1820_popr[j];
#endif // USE_BMP280_SENSOR
                                    }
                                    // Запоминаем максимальную температуру датчиков
                                    if (MaxTemps[j] < temps[j])
                                        MaxTemps[j] = temps[j];
#endif
                                }
                            }
                        }
                        k++;
                    }
                }

                StepOut = 0;
                flNeedTemp = 0;

#ifdef DEBUG
                flErrDs18 = 0;
#endif

                flNoPhase = 0; // восстанавливаем фазовое управление клапанами.

                if (!flErrDs18) // Если не было ошибок, то сбрасываем счетчики.
                {
                    flNeedAnalyse = 1; // Все необходимые данные прочитаны устанавливаем флаг того, что нужно анализировать состояние выполнения процесса.
                    CntErrDs18 = 0;
                    NumErrDs18 = 0;
                    // Сбрасываем признак вывода информации об ошибке
                    if (DispDopInfo == 4)
                        DispDopInfo = 0;
                } else {
                    // Если были ошибки, заново запускаем определение температуры
                    ds.reset();
                    ds.write(0xCC, 1); // start conversion, with parasite power on at the end
                    ds.write(0x44, 1); // start conversion, with parasite power on at the end
                    flNeedTemp = 0;
                    StepOut = 1;
                }
            }
        }
    }

    if (CntErrDs18 > MAX_ERR_DS18) {
        flNeedAnalyse = 1;
#ifndef TEST
        IspReg = 251; // Переводим контроллер в режим ошибки датчиков.
#endif
    }

    if (flNeedAnalyse == 1) {

        trig_on = !trig_on;
        digitalWrite(PIN_RST_WDT, trig_on); // Выдаем признак работы для внешнего контроля

#ifdef USE_SLAVE
        SlaveON = digitalRead(PIN_SLAVE_3) * 8 + digitalRead(PIN_SLAVE_2) * 4 + digitalRead(PIN_SLAVE_1) * 2 + digitalRead(PIN_SLAVE_0);
#endif

#ifdef TEST
        DEBUG_SERIAL.println(F("BEGIN PROSSES"));
#endif

        switch (IspReg) {
        case 101: //Текущее состояние - мониторинг
            UstPower = 0; // Phisik: при переходе в мониторинг не выключался ТЭН.
            break;
        case 102: //Текущее состояние - термостат
            //UstPower=0;
            ProcessTermostat();
            break;
        case 103: //Текущее состояние - регулятор мощности
#ifndef USE_SLAVE
            if (StateMachine < 100)
                UstPower = UstPowerReg;
            else
                UstPower = 0;
#else
            if (SlaveON == 0)
                UstPower = 0;
            if (SlaveON == 1)
                UstPower = Power;
            if (SlaveON == 2)
                UstPower = PowerGlvDistil;
            if (SlaveON == 3)
                UstPower = PowerRect;
            if (SlaveON == 4)
                UstPower = PowerDistil;
#endif
            // Если стоит признак авто-детектирования ТЭНа, то мощность устанавливаем максимально возможную.
            if (flAutoDetectPowerTEN == 1 && StateMachine < 100)
                UstPower = 9999;
            break;
        case 108:
        case 113:
            ProcessRazvarZerno();
            break;
        case 114:
            ProcessHLDZatorByChiller();
            break;
        case 115:
            ProcessTimerMaxPower();
            break;
        case 116:
            ProcessBeerCloneBrau();
            break;
        case 117:
            ProcessDistillFractional();
            break;
        case 118: //Текущее состояние - фракционная ректификация
        case 109: //Текущее состояние - ректификация
            ProcessRectif();
            break;
        case 104: //Текущее состояние - первый недробный перегон
        case 106: //Текущее состояние - второй дробный
        case 107: //Текущее состояние - третий дробный
            ProcessSimpleDistill();
            break;
        case 105: //Текущее состояние - отбор голов
            ProcessSimpleGlv();
            break;
        case 110:
            ProcessDistilDefl();
            break;
        case 111:
            ProcessNDRF();
            break;
        case 112:
            ProcessNBK();
            break;
        case 129:
            ProcessTestKLP();
            break;
        case 130:
            if (StateMachine == 1) // Если состояние автомата - не запущено
            {
                //            flRing=1;
                KlOpen[KLP_VODA] = 900;
                KlClose[KLP_VODA] = 100;
                KlOpen[KLP_HLD] = KlOpen[KLP_VODA];
                KlClose[KLP_HLD] = KlClose[KLP_VODA];
                KlState[KLP_HLD] = KlState[KLP_VODA];
                KlCount[KLP_HLD] = KlCount[KLP_VODA];
                KlOpen[KLP_DEFL] = KlOpen[KLP_VODA];
                KlClose[KLP_DEFL] = KlClose[KLP_VODA];
                KlState[KLP_DEFL] = KlState[KLP_VODA];
                KlCount[KLP_DEFL] = KlCount[KLP_VODA];
                KlOpen[KLP_SR] = KlOpen[KLP_VODA];
                KlClose[KLP_SR] = KlClose[KLP_VODA];
                KlState[KLP_SR] = KlState[KLP_VODA];
                KlCount[KLP_SR] = KlCount[KLP_VODA];
                digitalWrite(PIN_TERMOSTAT_ON, HIGH);
                digitalWrite(PIN_TERMOSTAT_OFF, HIGH);
                //             digitalWrite(30,HIGH);
                //             digitalWrite(31,HIGH);
                //             digitalWrite(32,HIGH);

                StateMachine = 100; // Переводим процесс в состояние выполнения
            }
            break;

        case 248:
        case 249:
        case 250:
        case 251:
        case 252:
        case 253:
        case 254:
#if NUM_PHASE > 1
            KlOpen[KLP_HLD] = 100;
            KlClose[KLP_HLD] = 0;
            digitalWrite(PIN_KLP_BEG + KLP_HLD, HIGH);
#endif
#if USE_GSM_WIFI == 1

            StateMachine = 100;
            if (flNeedCall == 0)
                flNeedCall = 1;
            if (flNeedCall == 1) {
                // Звонок на телефон в обязательном порядке.
                flGPRSState = 40; // Активируем звонок если в этом есть необходимость
                flNeedRefrServer = 1;
                timeGPRS = 60;

            } else {
                // Если в данный момент не дозавниваемся, то отправляем SMS или GPRS
                if (flNeedCall != 2)
                    if (FlToGSM && lastSMSState != StateMachine)
                        StateToSMS();
            }

#endif

            // Тревога либо ошибки протечка воды, отключаем воду, отключаем мощность.
            UstPower = 0;
            CloseAllKLP();

            // Пищим, независимо от настроек.
#ifndef TEST
            my_beep(BEEP_LONG * 5);
#endif

            digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
            digitalWrite(PIN_TRIAC, LOW);
            digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
            flAllOff = 1;
            break;
        }

        if (FlToUSART)
            GetStateSerial();

#ifdef TEST
        DEBUG_SERIAL.println(F("END PROSSES"));
#endif

        flNeedAnalyse = 0;
        RaspredPowerByPhase();
    }

    if (StateVolts != 1) {
#if NUM_PHASE < 2
        //  анализ сотового
#if USE_GSM_WIFI
        ProcessGSM();
#endif // #if USE_GSM_WIFI
#else
        ProcessSlavePhase();
#endif
    }
    //  анализ отладки

    if (FlUsart == 1) {
        // Признак того, что надо выдать информацию в ком-порт (есл ее вдруг невозможно сразу выдать, например, из прерывания).
        DEBUG_SERIAL.println(my_tx_buffer);
        FlUsart = 0;
    }

    // Начало расчета среднеквадратичного

    if (StateVolts == 2) {

#ifdef TESTRM
        DEBUG_SERIAL.println(F("BEGIN SQNAPR"));
        TCNT4H = 0x00;
        TCNT4L = 0x00;
#endif

        StateVolts = 3;

        // поскольку тут идет преобразование и умножение 32 разрядных чисел, делаем это по необходимости, а не каждый раз (хотя какая разница, тут и таких умножений дохрена)
        SqNapr = (long)220 * 220 * 20 / PowerPhase[0]; // Рассчитали сопротивление ТЭНа, умноженное на 20
        R_TEN20 = SqNapr;

#ifdef TESTRM
        DEBUG_SERIAL.print(F("UstPower="));
        DEBUG_SERIAL.println(UstPower);
        DEBUG_SERIAL.print(F("Power="));
        DEBUG_SERIAL.println(Power);
        DEBUG_SERIAL.print(F("NaprPeregrev="));
        DEBUG_SERIAL.println(NaprPeregrev);
        DEBUG_SERIAL.print(F("MaxVoltsOut="));
        DEBUG_SERIAL.println(MaxVoltsOut);
        DEBUG_SERIAL.print(F("R_TEN20="));
        DEBUG_SERIAL.println(R_TEN20);
#endif

        SqNapr = 0;
        SqNaprPrev = 0;
        //   MaxVolts=0;

#ifdef TESTRM
        TCNT4H = 0x00;
        TCNT4L = 0x00;
#endif

#ifdef TESTRM
        DEBUG_SERIAL.print(F("SqNapr="));
        DEBUG_SERIAL.println(SqNaprT);
        //Считаем напряжение в сети исходя из среднеквадратичного, средневыпрямленного, максимального
        DEBUG_SERIAL.print(F("RMS/AVG/MAX"));
        DEBUG_SERIAL.write(9);
        DEBUG_SERIAL.println((long)my_sqrt(SqNaprT / index_input));
        //DEBUG_SERIAL.write(9);
        //DEBUG_SERIAL.println((long)MaxVolts*707/1000);
#endif

#if USE_BRESENHAM_ASC712 == 0
        if (!flCorrASC712) // Расчет напряжения
        {

            TekPower = SqNaprT / index_input; // Определили квадрат напряжения

            // Но фактически мощность подаваемая на ТЭНы меньше из-за падения на симисторе.

            MaxVoltsOut = my_sqrt(TekPower); // Рассчитываем среднеквадратичное

#ifdef TESTRM
            DEBUG_SERIAL.print(F("index_input="));
            DEBUG_SERIAL.println(index_input);
            DEBUG_SERIAL.print(F("TekPower2="));
            DEBUG_SERIAL.println(TekPower);
#endif

            TekPower = (TekPower * 20) / R_TEN20; // Определили фактическую мощность в зависимости от сопротивления ТЭНа

#ifdef TESTRM
            DEBUG_SERIAL.print(F("TekPower2="));
            DEBUG_SERIAL.println(TekPower);
#endif

            TekPowerKLP = (unsigned long)NaprPeregrev * NaprPeregrev;

#ifdef TESTRM
            DEBUG_SERIAL.print(F("UstPower="));
            DEBUG_SERIAL.println(UstPwrPH1);
            DEBUG_SERIAL.print(F("FactPower="));
            DEBUG_SERIAL.println(FactPower);
            DEBUG_SERIAL.print(F("MaxVoltsOut="));
            DEBUG_SERIAL.println(MaxVoltsOut);
            DEBUG_SERIAL.print(F("TekPowerKLP="));
            DEBUG_SERIAL.println(TekPowerKLP);
#endif

            if (FactPower > 15 && CorrectASC712 >= 2 && !(UstPwrPH1 >= PowerPhase[0])) // Если фактическая мощность рассчитана и мы не находимся в режиме подачи полного напряжения на ТЭНы
            {
                // Это формулы, для ПИД-регулировани, предоставленные коллегой m16 c homedistiller.ru
                //          ;******************************************************************************
                //          ;*
                //          ;*	new_err = Ps_r - Pv			;вычисляем новую ошибку
                //          ;*
                //          ;*
                //          ;*	It = It + ki_r * new_err	        ;считаем интегральную составляющую
                //          ;*
                //          ;*		if ( It < 0.0 ) It = 0.0        ;если интегральная отрицательна то присваиваем ей ноль
                //          ;*		else  if( It > Sc_r ) It = Sc_r	;если интегральная превысит установленный предел то присваиваем значение предела
                //          ;*
                //          ;*	Dt =  kd_r * ( new_err - old_err )	;считаем дифференциальную
                //          ;*
                //          ;*	Ut = ( new_err*10 + It + Dt ) /  Sc_r	;вычисляем % от максимальной мощности
                //          ;*
                //          ;*		if ( Ut < 0.0 ) Ut = 0.0
                //          ;*		else if ( Ut > 100 ) Ut = 100
                //          ;*
                //          ;*	old_err = new_err                       ;присваиваем значению новой ошибке предыдущую
                //          ;******************************************************************************
                //          где
                //          Ps_r - мощность установленная
                //          Pv   - мощность измеренная
                //          new_err- новая ошибка рассогласования
                //          old_err- предыдущая ошибка рассогласования
                //          Sc_r   - максимальное накопление интегральной составляющей
                //          It     - интегральная
                //          ki_r   - коэф. интегральной
                //          Dt     - дифю составляющая
                //          kd_r   - коэф. дифференциальной
                //          Ut     - % от макс. мощности
                // Считаем ошибку рассоглавования.

                // Ошибку рассогласования учитвыаем, если она не более 1/8 мощности ТЭНа.
                NewErr = ((long)UstPwrPH1 - (long)FactPower) * 200 / (long)Power;
                if (NewErr < -30)
                    NewErr = -30;
                else if (NewErr > 30)
                    NewErr = 30;

                Dt = NewErr - OldErr;
                It = It + NewErr;
                // Накопленную ошибку не более 1/2 мощности ТЭНа.
                if (It < -100)
                    It = -100;
                else if (It > 100)
                    It = 100;

                OldErrOut = OldErr; // Старая ошибка для отображения на экране
                OldErr = NewErr; // Запоминаем старую ошибку.
                //          if (UstPower>FactPower) It=It+5;
                //          else
                //            if (UstPower<FactPower) It=It-5;
            } else {
                // Если мощность не рассчитана, то считаем, что находимся в режиме отладки и не подключили ТЭНы В этом случае просто делаем фазовое регулирование исходя из
                // входного напряжения, не учитывая фактическую мощность, рассчитанную исходя из датчика тока на ТЭНах
                NewErr = 0;
                OldErr = 0;
                OldErrOut = 0;
                It = 0;
                Dt = 0;
            }

            // Пока без коэффиццииенов
            FindKt = (long)(UstPwrPH1)*200 / TekPower; // Установили коэффициент поиска мощности для регулятора мощности как соотношение необходимой мощности к имеющейся в данный момент.
            FindKt = FindKt + It; // Прибавляем коэффициент.

            // Нестабилизированная версия, фактическую мощность не учитываем.
#if SIMPLED_VERSION == 20
            FindKt = (long)(UstPowerReg)*200 / Power; // Установили коэффициент поиска мощности для регулятора мощности как соотношение необходимой мощности к имеющейся в данный момент.
            MaxVoltsOut = 220;
            flNoAlarmLowPower = 1;
#endif

            if (FindKt > 200 || It == -100) {
                CntErrPower++;
                if (UstPwrPH1 < PowerPhase[0] && UstPwrPH1 > 15 && CntErrPower > 100 && !flNoAlarmLowPower) {
                    if (FindKt > 200) {
                        DispDopInfo = 3;
                        FindKt = 200;
                    } // Выводим на экран информацию о низком напряжении но только в том случае, когда не в режиме разгона и установленная мощность больше 15
                    else
                        DispDopInfo = 6; // и в том случае когда не установлено, что не нужно выводить предупреждение о низком уровне напряжения
#if NUM_PHASE > 1
                    if (FindKt >= 200) {
                        KlOpen[KLP_HLD] = 100; // Раз в секунду
                        KlClose[KLP_HLD] = 100;
                    } else {
                        KlOpen[KLP_HLD] = 15; // Два раза в секунду
                        KlClose[KLP_HLD] = 15;
                    }

#endif
                }
            } else {
                CntErrPower = 0;
#if NUM_PHASE > 1
                KlOpen[KLP_HLD] = 0;
                KlClose[KLP_HLD] = 50;
#endif

                if (DispDopInfo == 3 || DispDopInfo == 6) {
                    DispDopInfo = 0; // Убираем информацию о низком напряжении, если она была ранее установлена
                }
            }

#if NUM_PHASE > 1
            if (IspReg >= 240) {
                KlOpen[KLP_HLD] = 100;
                KlClose[KLP_HLD] = 0;
            }
#endif

            // В клапанах тоже учитываем поправку мощности
            //TekPowerKLP=(TekPowerKLP+It)*1000/( (long) MaxVoltsOut* (long)MaxVoltsOut);
            //Пока убираем поправку мощности в клапанах, возможно из-за этого "пробивает" симистор.
            TekPowerKLP = (TekPowerKLP)*1000 / ((long)MaxVoltsOut * (long)MaxVoltsOut);

            FindKtKLP = TekPowerKLP; // Установили коэффициент поиска мощности для клапанов

            // Проверка минимума и максимума
            if (FindKtKLP > 1000)
                FindKtKLP = 1000;
            if (FindKtKLP < 5)
                FindKtKLP = 5;

#ifdef TESTRM
            DEBUG_SERIAL.print(F("NewErr="));
            DEBUG_SERIAL.println(NewErr);
            DEBUG_SERIAL.print(F("OldErr="));
            DEBUG_SERIAL.println(OldErrOut);
            DEBUG_SERIAL.print(F("FindKt="));
            DEBUG_SERIAL.println(FindKt);
            DEBUG_SERIAL.print(F("FindKtNoErr="));
            DEBUG_SERIAL.println((long)(UstPower)*1000 / TekPower);
            DEBUG_SERIAL.print(F("NaprPeregrev="));
            DEBUG_SERIAL.println(NaprPeregrev);
            DEBUG_SERIAL.print(F("FindKtKLP="));
            DEBUG_SERIAL.println(FindKtKLP);
            DEBUG_SERIAL.print(F("SqNaprT="));
            DEBUG_SERIAL.println(SqNapr);
            DEBUG_SERIAL.print(F("SqNaprPrev="));
            DEBUG_SERIAL.println(SqNaprPrev);
#endif

            if (FindKt < 1)
                FindKt = 1;
            if (FindKt > 200)
                FindKt = 200;

            // Поскольку таблица разбита по мощности равномерно в пределах 5/1000 доли, то нет необходимости высчитывать и искать по таблице, достаточно просто поделить.
            TimeOpenTriac = tableS10[FindKt - 1];
            TimeOpenTriac *= 10;
            // Учиываем предельно допустимый угол открытия триака
            if (FindKtKLP < 1000) {
                TimeOpenKLP = tableS10[FindKtKLP / 5 - 1];
                // Защита от "дребезга клапана" на малых углах открытия симистора.
                if (TimeOpenKLP < 70)
                    TimeOpenKLP = 0;
                TimeOpenKLP *= 10;

            } else
                TimeOpenKLP = 0;

            if (TimeOpenKLP <= 10)
                TimeOpenKLP = 9999; // Если не нашли нужную мощность, то устанавливаем максимальную.
            if (flAutoDetectPowerTEN)
                TimeOpenTriac = 9999;
        } else // Это расчет фактического тока и фактической мощности
        {

            if (CorrectASC712 == 3) // Расчет по напряжениию
            {
                TekPower = SqNaprT / index_input; // Определили квадрат напряжения
                MaxIOut = my_sqrt(TekPower); // Рассчитываем среднеквадратичное
#ifdef TESTRM
                DEBUG_SERIAL.print(F("index_input="));
                DEBUG_SERIAL.println(index_input);
                DEBUG_SERIAL.print(F("TekPower2="));
                DEBUG_SERIAL.println(TekPower);
#endif
                // FactPower=(TekPower*20)/R_TEN20; // Определили фактическую мощность в зависимости от сопротивления ТЭНа

                FactPower = (1 - POWER_SMOOTHING_FACTOR) * FactPower + POWER_SMOOTHING_FACTOR * (TekPower * 20) / R_TEN20;
            } else {
                // Здесь определили квадрат тока умноженный на 10.
                SqNaprPrev = SqNaprT * 100 / ((unsigned long)index_input * SENSITIVE_ASC712);
                // Определили ток, умноженный на 10 (для вывода на экран).
                MaxIOut = my_sqrt((long)SqNaprPrev * 100 / SENSITIVE_ASC712);
                if (flAutoDetectPowerTEN) {
                    FindPower = (long)SqNaprPrev * 100 / (SENSITIVE_ASC712);
                    //            DEBUG_SERIAL.print(F("I^2="));
                    //            DEBUG_SERIAL.println(my_sqrt(FindPower));
                    TekPower = (unsigned long)4840 * my_sqrt(FindPower) / MaxVoltsOut; // 4840=220*22
                    if (TekPower != 0) {
                        Power = (int)(TekPower / 10) * 10;
                        PowerPhase[0] = Power;
                    }
                }

                FindPower = (long)SqNaprPrev * R_TEN20 / (20 * SENSITIVE_ASC712);
                FactPower = (1 - POWER_SMOOTHING_FACTOR) * FactPower + POWER_SMOOTHING_FACTOR * FindPower;

#ifdef TESTRM
                DEBUG_SERIAL.print(F("SqNaprT="));
                DEBUG_SERIAL.println(SqNaprT);
                DEBUG_SERIAL.print(F("index_input="));
                DEBUG_SERIAL.println(index_input);
                DEBUG_SERIAL.print(F("index_input*SENSITIVE_ASC712="));
                DEBUG_SERIAL.println((unsigned long)index_input * SENSITIVE_ASC712);
                DEBUG_SERIAL.print(F("SqNaprPrev="));
                DEBUG_SERIAL.println(SqNaprPrev);
                DEBUG_SERIAL.print(F("FindPower="));
                DEBUG_SERIAL.println(FindPower);
                DEBUG_SERIAL.print(F("FactPower="));
                DEBUG_SERIAL.println(FactPower);
#endif
            }
        }

#endif

        //   Сумму квадратов измерений определили, она хранится в переменной SqNapr.
        //   Теперь надо перевести это в фактический ток.
#if USE_BRESENHAM_ASC712 == 1

        // Рассчитали сумму, пересчитанную в фактические значения тока.
        SqNaprPrev = SqNaprT * 100 / ((unsigned long)index_input * SENSITIVE_ASC712);
#ifdef TESTRM
        DEBUG_SERIAL.print(F("SqNapr="));
        DEBUG_SERIAL.println(SqNapr);
        DEBUG_SERIAL.print(F("SqNaprPrev="));
        DEBUG_SERIAL.println(SqNaprPrev);
#endif

        FindPower = (long)SqNaprPrev * R_TEN20 / (20 * SENSITIVE_ASC712);
        FactPower = FindPower;

        MaxVoltsOut = my_sqrt(FindPower * R_TEN20 / 20);

        // Теперь зная мощность рассчитываем процент для брезенхема
        b_value[0] = (long)UstPower * b_size[0] / (long)FindPower;

        if (b_value[0] > b_size[0] && UstPower < Power && !flNoAlarmLowPower)
            DispDopInfo = 3; // Выводим на экран информацию о низком напряжении если находимся не в режиме разгона.
        else if (DispDopInfo == 3)
            DispDopInfo = 0; // Убираем информацию о низком напряжении, если она была ранее установлена

#ifdef TESTRM
        DEBUG_SERIAL.print(F("MaxVoltsOut="));
        DEBUG_SERIAL.println(MaxVoltsOut);
        DEBUG_SERIAL.print(F("FindPower="));
        DEBUG_SERIAL.println(FindPower);
        DEBUG_SERIAL.print(F("FactPower="));
        DEBUG_SERIAL.println(FactPower);
        DEBUG_SERIAL.print(F("b_value[0]="));
        DEBUG_SERIAL.println((int)b_value[0]);
#endif
#endif

#ifdef TESTRM

        DEBUG_SERIAL.println(F("VOLTS"));
        DEBUG_SERIAL.print(F("j="));
        DEBUG_SERIAL.println(j);
        DEBUG_SERIAL.print(F("SqNapr="));
        DEBUG_SERIAL.println(SqNapr);
        DEBUG_SERIAL.print(F("SqNaprPrev="));
        DEBUG_SERIAL.println(SqNaprPrev);
        DEBUG_SERIAL.print(F("FindPower="));
        DEBUG_SERIAL.println(FindPower);
        DEBUG_SERIAL.print(F("b_value="));
        DEBUG_SERIAL.println(b_value[0]);
        DEBUG_SERIAL.print(F("OpenTriac="));
        DEBUG_SERIAL.println(TimeOpenTriac);
        DEBUG_SERIAL.print(F("OpenKLP="));
        DEBUG_SERIAL.println(TimeOpenKLP);

        DEBUG_SERIAL.print(F("COUNT="));
        DEBUG_SERIAL.println(index_input);

        //    for(i=0;i<index_input;i++)
        //    {
        //      // Сканируем клавиатуру, если установлен флаг.
        //      if (flNeedScanKbd==1 )
        //      {
        //        ScanKbd();
        //        flNeedScanKbd=0;
        //      }
        //
        //      DEBUG_SERIAL.print(tic_adc[i]);
        //      DEBUG_SERIAL.write(9);
        //      DEBUG_SERIAL.println(data_adc[i]);
        //    }
        DEBUG_SERIAL.print(F("TICZERO="));
        DEBUG_SERIAL.write(9);
        DEBUG_SERIAL.println(TicZero);

        //    DEBUG_SERIAL.println(F("TABLET"));
        //    for(i=0;i<MAX_TABLE_T;i++)
        //    {
        //      // Сканируем клавиатуру, если установлен флаг.
        //      if (flNeedScanKbd==1 )
        //      {
        //        ScanKbd();
        //        flNeedScanKbd=0;
        //      }
        //
        //      DEBUG_SERIAL.print(tableK[i]);
        //      DEBUG_SERIAL.write(9);
        //      DEBUG_SERIAL.println(tableT[i]);
        //    }

        DEBUG_SERIAL.println(F("ENDVOLTS"));

#endif

        StateVolts = 4;
    }
}
