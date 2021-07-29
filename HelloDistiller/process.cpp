// Последнее обновление 2017-05-31 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам
// Здесь будут программы для всех процессов

#include "configuration.h"
#include "declarations.h"

void PrepareProcess()
{
    char i;
    // Максимальную температуру процессов зануляем.
    for (i = 0; i < MAX_DS1820; i++) {
        MaxTemps[i] = 0;
    }
    // Все таймеры зануляем.
    timeMIXER = 0;
    timeNBK = 0;
    time1 = 0;
    time2 = 0;
    time3 = 0;
    flAutoDetectPowerTEN = 0; // Убираем признак авто-детектирования ТЭНОв
    flNoAlarmLowPower = NO_LOW_POWER_WARNING; // Убираем признак невыдачи предупреждения о низком уровне напряжения

    // Все клапана зануляем
    CloseAllKLP();
    //  Разгон отключаем
    digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
    // Убираем мощность с ТЭНов
    UstPower = 0;
}

void ProcessTermostat()
{

    if (ds1820_devices == 0)
        StateMachine = 100;

    switch (StateMachine) {
    case 0: // Процесс не запущен
        // Процесс термостат запускается автоматически, чтобы он работал даже если выключат и включат свет.
        //break;
    case 1: // Старт процесса
        PrepareProcess();

        digitalWrite(PIN_TERMOSTAT_OFF, LOW); // Включаем охлаждение
        digitalWrite(PIN_TERMOSTAT_ON, LOW); // Выключаем нагрев
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);

        // Вот тут включили разгон
        digitalWrite(PIN_RZG_ON, RELAY_HIGH);

        StateMachine = 2;
    case 2: // Основной цикл программы, пытаемся регулировать мощность, не превышая температуру
        if (DS_TEMP(TEMP_TERMOSTAT) <= TempTerm) {
            digitalWrite(PIN_TERMOSTAT_OFF, LOW); // Включаем охлаждение
            digitalWrite(PIN_TERMOSTAT_ON, HIGH); // Выключаем нагрев
            digitalWrite(PIN_START_1, RELAY_HIGH);
            digitalWrite(PIN_START_2, RELAY_HIGH);
            ProcessPIDTemp(TempTerm, DS_TEMP(TEMP_TERMOSTAT));
            UstPower = (long)Power * KtT / 1000;

            // Мощность ограничена мощностью варки зерна
            if (UstPower > PowerVarkaZerno)
                UstPower = PowerVarkaZerno;

            // Если подошли на 5 градусов к целевой температуре, разгон выключаем
            if (DS_TEMP(TEMP_TERMOSTAT) > TempTerm - 50)
                digitalWrite(PIN_RZG_ON, !RELAY_HIGH);

            break;
        }
        // Температура все-таки стала выше нужной - идем к следующему этапу
        StateMachine = 3;
        // if (BeepStateProcess) my_beep(BEEP_LONG);

    case 3: //
        UstPower = 0; // Убираем мощность с ТЭНов
        if (DS_TEMP(TEMP_TERMOSTAT) > TempTerm - Delta) {
            digitalWrite(PIN_TERMOSTAT_ON, LOW); // Выключаем нагрев
            digitalWrite(PIN_TERMOSTAT_OFF, HIGH); // Включаем охлаждение
            digitalWrite(PIN_START_1, !RELAY_HIGH);
            digitalWrite(PIN_START_2, !RELAY_HIGH);

            break;
        }
        // Если температура упала ниже TempTerm-Delta, идем регулировать ТЭНы
        StateMachine = 2;
        // if (BeepStateProcess) my_beep(BEEP_LONG);
        break;
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        flAllOff = 1;
        digitalWrite(PIN_TERMOSTAT_ON, LOW); // Выключаем нагрев
        digitalWrite(PIN_TERMOSTAT_OFF, LOW); // Включаем охлаждение
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        UstPower = 0; // Убираем мощность с ТЭНов
        break;
    }
}

void ProcessTimerMaxPower()
{
    int minute;
    switch (StateMachine) {
    case 0: //
        // Процесс не запущен
        break;
    case 1: // Старт процесса
        PrepareProcess();

        timerMinute = 0;
        StateMachine = 2;

#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif
    case 2: // Температура Больше необходиомй
        minute = Seconds / 60;
        if (minute <= timerMinute) {
            UstPower = PowerMinute; // Подаем полную мощность на ТЭНЫ
            digitalWrite(PIN_START_1, RELAY_HIGH);
            digitalWrite(PIN_START_2, RELAY_HIGH);
            break;
        }
        StateMachine = 100;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    case 100: // Конечное состояние автомата

#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif

        if (BeepEndProcess)
            my_beep(BEEP_LONG);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);

        flAllOff = 1;
        UstPower = 0; // Убираем мощность с ТЭНов
        break;
    }
}

//    case 116:

void ProcessBeerCloneBrau()
{
    // Управление мешалкой;
    if (StateMachine > 0 && StateMachine < 100) {
        if (time1 == 0 || DS_TEMP(TEMP_TERMOSTAT) >= 880) {
            // Насос включаем только
            if (KlOpen[KLP_HLD] == 0 && DS_TEMP(TEMP_TERMOSTAT) < 880) {
                KlOpen[KLP_HLD] = 40;
                KlClose[KLP_HLD] = 0;
                KlOpen[KLP_GLV_HVS] = 40;
                KlClose[KLP_GLV_HVS] = 0;
                time1 = (int)tempP[0] * 60; // Взводим насос на десять минут работы
                timeMIXER = (int)tempP[0] * 60;

            } else {
                KlOpen[KLP_HLD] = 0;
                KlClose[KLP_HLD] = 40;
                KlOpen[KLP_GLV_HVS] = 0;
                KlClose[KLP_GLV_HVS] = 40;
                time1 = (unsigned int)timeP[0] * 60; // Взводим насос на две минуты простоя
                timeMIXER = 0;
            }
        }
    }

    switch (StateMachine) {
    case 0: //
        // Процесс не запущен
        time1 = 0;
        break;
    case 1: // Старт процесса
        KlTek = 1;
        time1 = 0;
        PrepareProcess();
        flNoAlarmLowPower = 1; // Предупреждение о низкой мощности отключим, здесь оно не важно
        StateMachine = 2;

    case 2: // Температура Больше необходиомй
        ProcessPIDTemp((int)tempP[KlTek] * 10, DS_TEMP(TEMP_TERMOSTAT));
        UstPower = (long)Power * KtT / 1000;
        // Мощность ограничена мощностью варки зерна
        if (UstPower > PowerVarkaZerno)
            UstPower = PowerVarkaZerno;
        // Если нужная температура меньше 100 градусов, то проверяем пока она не превысит заданную
        // Если нужная температура более 100 градусов, тогда пропускаем этот режим и считаем что началась варка.
        if (DS_TEMP(TEMP_TERMOSTAT) < (int)tempP[KlTek] * 10 && tempP[KlTek] <= 100)
            break;
        // Если темперетура больше либо равно 110, то закипание определяется по второму датчику температуры.
        //  if (DS_TEMP(TEMP_DEFL)<TempDeflBegDistil && ds1820_devices>1 || (TempDeflBegDistil<0 &&  DS_TEMP(TEMP_DEFL)<-TempDeflBegDistil) && tempP[KlTek]>=110 )  break;

        StateMachine = 3;
        time2 = (int)timeP[KlTek] * 60;
    case 3:
        if (tempP[KlTek] > 100) {
            UstPower = PowerVarkaZerno;
            // Если температура вверху заторника меньше заданной и температура варки больше 110, тогда считаем, что еще не закипело.
            // Проверка на закипание проводится только один раз в начале кипения, то есть если предыдущий этап не был кипением.
            if (DS_TEMP(TEMP_DEFL) < TempDeflBegDistil && ds1820_devices > 1 || (TempDeflBegDistil < 0 && DS_TEMP(TEMP_DEFL) < -TempDeflBegDistil) && tempP[KlTek] >= 110)
                time2 = (int)timeP[KlTek] * 60;

        } else {
            if (DS_TEMP(TEMP_TERMOSTAT) < (int)tempP[KlTek] * 10) {
                ProcessPIDTemp((int)tempP[KlTek] * 10, DS_TEMP(TEMP_TERMOSTAT));
                UstPower = (long)Power * KtT / 1000;
                if (UstPower > PowerVarkaZerno)
                    UstPower = PowerVarkaZerno;
            } else
                UstPower = 0;
        }

        if (timeP[KlTek] == 0)
            my_beep(2 * BEEP_LONG); // Если время ожидания равно нулю, тогда пищим, ожидая действия пользователя
        else {
            if (timeP[KlTek] == 255)
                timeP[KlTek] = 0;
            if (time2 == 0) {
                KlTek++; // Если все паузы выполнены, то переходим к окончанию процесса, иначе к следующей паузе.
                if (KlTek > CntPause)
                    StateMachine = 100;
                else
                    StateMachine = 2;
                if (BeepStateProcess)
                    my_beep(BEEP_LONG);
            }
        }
        break;
    case 4:
        // Это системное состояние к нему только через debug перейти можно, просто ручной переход на следующую паузу.
        KlTek++; // Если все паузы выполнены, то переходим к окончанию процесса, иначе к следующей паузе.
        if (KlTek > CntPause)
            StateMachine = 100;
        else
            StateMachine = 2;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        break;
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        if (BeepEndProcess)
            my_beep(BEEP_LONG);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        CloseAllKLP();
        flAllOff = 1;
        UstPower = 0; // Убираем мощность с ТЭНов
        break;
    }
}

//    case 110:
// Процесс дистилляции с подключаемым дефлегматором
// Процесс поддерживат температуру в дефлегматоре не более заданной + дельта и не менее заданной - дельта
void ProcessDistilDefl()
{

    // Если достигнут уровень в приемной емкости или температуре в кубе превысыла допустимую температуру для этого режима

    if ((flAlarmUroven || DS_TEMP(TEMP_KUB) >= tEndDistDefl || deltaPower >= Power) && StateMachine < 7) { // Переводим автомат в стадию завершения
        StateMachine = 7;
        SecondsEnd = 2 * 60 + ((int)(FlAvtonom >> 1)) * 60; // Через одну минуту отключим воду
        SecTempPrev = Seconds;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    }
    // Проверяем температуру в ТСА
    if (DS_TEMP(TEMP_TSA) >= MAX_TEMP_TSA) {
        StateMachine = 101; // Переводим автомат в состояние аварии по ТСА.
        UstPower = 0;
    }
    if (flAlarmMPX5010)
        StateMachine = 100; // Переводим в режим тревоги по датчику давления

    switch (StateMachine) {
    case 0: // Процесс не запущен
        break;
    case 1: // Начало процесса
        PrepareProcess();
        //          if (FlToGSM && lastSMSState!=StateMachine) StateToSMS();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif
        KlOpen[KLP_VODA] = PER_KLP_OPEN;
        KlClose[KLP_VODA] = PER_KLP_CLOSE;
        StateMachine = 2; //Этот процесс пока заблокирован, сразу идем на окончание.
        //    digitalWrite(PIN_REG_ON,HIGH);
        digitalWrite(PIN_RZG_ON, RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, !ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, RELAY_HIGH);
        digitalWrite(PIN_START_2, RELAY_HIGH);
        flAllOff = 0;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        deltaPower = 0;
    case 2: // Ожидание, пока прогреется термометр в дефлегматоре

        UstPower = Power; // Устанавливаем максимальную мощность для разгона

        // Если температура в дефлегматоре меньше 70 градусов и датчик дефлегматора в принципе подключен, тогда ждем, пока
        // дефлегматор прогреется, чтобы запустить воду в холодильник (это для некоторой экономии воды)
        if (DS_TEMP(TEMP_DEFL) < TempDeflBegDistil && ds1820_devices > 1 || (TempDeflBegDistil < 0 && DS_TEMP(TEMP_KUB) < -TempDeflBegDistil))
            break;

        // Подаем питание на клапан подачи воды холодильника
        StateMachine = 3;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);

    case 3:
        // Ждем, пока температура в кубе не достигнет температуры начала дистилляции (если есть датчик дефлегматора, тогда этот пункт сразу
        // проскочится, поскольку темперетура в кубе не может быть меньше, чем температура в дефлегматоре
        if (DS_TEMP(TEMP_KUB) < TempDeflBegDistil)
            break;
        // Подаем питание на клапан подачи воды холодильника

        // Подаем питание на клапан подачи воды холодильника
        // для этого синхронизируем его с клапаном общей подачи воды (возможно, при этом будет меньше гидроударов, хотя я их и так не наблюдаю)
        KlOpen[KLP_HLD] = KlOpen[KLP_VODA];
        KlClose[KLP_HLD] = KlClose[KLP_VODA];
        KlState[KLP_HLD] = KlState[KLP_VODA];
        KlCount[KLP_HLD] = KlCount[KLP_VODA];

        StateMachine = 4;
        time2 = 15 * 60; // Взводиим таймер на 15 минут
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);

    case 4: // Дефлегматор отключен.
        // Если более 10 минут не вышли за пределы температуры, тогда увеличиваем мощность.
        if (time2 == 0) {
            deltaPower = deltaPower - (int)(PowerDistil / 20);
            if (deltaPower < 0)
                deltaPower = 0;
            time2 = 10 * 60;
        }

        UstPower = PowerDistil - deltaPower; // Устанавливаем мощность дистилляции.

        if (DS_TEMP(TEMP_DEFL) > TempDefl) {
            // Включаем дефлегматор и холодильник

            KlOpen[KLP_DEFL_D] = KlOpen[KLP_VODA];
            KlClose[KLP_DEFL_D] = KlClose[KLP_VODA];
            KlState[KLP_DEFL_D] = KlState[KLP_VODA];
            KlCount[KLP_DEFL_D] = KlCount[KLP_VODA];

            KlOpen[KLP_HLD] = KlOpen[KLP_VODA];
            KlClose[KLP_HLD] = KlClose[KLP_VODA];
            KlState[KLP_HLD] = KlState[KLP_VODA];
            KlCount[KLP_HLD] = KlCount[KLP_VODA];
            StateMachine = 5; // Переходим к этапу поддержания температуры путем частичной подачи воды в дефлегмато
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else
            break;

    case 5: // 50% воды из холодильника идет в дефлегматор.
        // Если более 10 минут не вышли за пределы температуры, тогда увеличиваем мощность.
        time2 = 10 * 60;
        if (time3 == 0) {
            deltaPower = deltaPower - (int)(PowerDistil / 30);
            if (deltaPower < 0)
                deltaPower = 0;
            time3 = 10 * 60;
        }
        UstPower = PowerDistil - deltaPower; // Устанавливаем мощность дистилляции.
        // Если температура превысила температуру, которую надо поддерживать в дефлегматоре+дельта, тогда переходим к режиму, когда
        // вся вода из холодильника идет через дефлегматор
        if (DS_TEMP(TEMP_DEFL) > TempDefl + DeltaDefl) {
            // Отключаем прямой выход из холодильника
            KlOpen[KLP_HLD] = 0;
            KlClose[KLP_HLD] = 10;
            // Включаем дефлегматор
            KlOpen[KLP_DEFL_D] = KlOpen[KLP_VODA];
            KlClose[KLP_DEFL_D] = KlClose[KLP_VODA];
            KlState[KLP_DEFL_D] = KlState[KLP_VODA];
            KlCount[KLP_DEFL_D] = KlCount[KLP_VODA];

            StateMachine = 6; // Переходим к этапу поддержания температуры путем 100% направления воды в дефлегматор
            time1 = 0;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else if (DS_TEMP(TEMP_DEFL) < TempDefl - DeltaDefl) // Если в результате работы с 50% дефлегматором, температура понизилась переходим в режим работы без дефлегматора
        {

            KlOpen[KLP_DEFL_D] = 0;
            KlClose[KLP_DEFL_D] = 10;

            KlOpen[KLP_HLD] = KlOpen[KLP_VODA];
            KlClose[KLP_HLD] = KlClose[KLP_VODA];
            KlState[KLP_HLD] = KlState[KLP_VODA];
            KlCount[KLP_HLD] = KlCount[KLP_VODA];
            StateMachine = 4; // Переходим к этапу без дефлегмации
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            break;
        } else
            break;

    case 6: // Вся вода из холодильника идет в дефлегматор.
        // Взводиим таймер на 10 минут
        time3 = 10 * 60;
        UstPower = PowerDistil - deltaPower; // Устанавливаем мощность дистилляции.
        // Если температура превысила требуемую + дельта более, чем на полдельта за три минуты, тогда уменьшааем мощность
        if (DS_TEMP(TEMP_DEFL) > TempDefl + DeltaDefl + DeltaDefl / 2) {
            if (time1 == 0) {
                deltaPower = deltaPower + (int)(PowerDistil / 10);
                time1 = 3 * 60; // Взводим таймер на три минуты.
            }
        }

        if (DS_TEMP(TEMP_DEFL) < TempDefl) // Если в результате работы с 100% дефлегматором, температура понизилась ниже установленной температуры переходим в режим работы с 50% дефлегма
        {
            // Включаем прямой выход из холодильника.
            KlOpen[KLP_HLD] = KlOpen[KLP_VODA];
            KlClose[KLP_HLD] = KlClose[KLP_VODA];
            KlState[KLP_HLD] = KlState[KLP_VODA];
            KlCount[KLP_HLD] = KlCount[KLP_VODA];

            KlOpen[KLP_DEFL_D] = KlOpen[KLP_VODA];
            KlClose[KLP_DEFL_D] = KlClose[KLP_VODA];
            KlState[KLP_DEFL_D] = KlState[KLP_VODA];
            KlCount[KLP_DEFL_D] = KlCount[KLP_VODA];
            StateMachine = 5; // Переходим к этапу поддержания температуры 50% дефлегмацией
            time2 = 15 * 60; // Взводиим таймер на 15 минут
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            break;
        } else
            break;

    case 7: // Ждем минуту для окончания подачи воды
        UstPower = 0;
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);

        SecondsEnd = SecondsEnd - (Seconds - SecTempPrev);
        SecTempPrev = Seconds;

        if (SecondsEnd <= 0) {
            // Переводим автомат в конечное состояние
            StateMachine = 100;
            SecondsEnd = Seconds;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }
        break;
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата
        if (BeepEndProcess)
            my_beep(BEEP_LONG);
    case 101:
        // Отключаем на всякий случай все!
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        if (BeepEndProcess)
            my_beep(BEEP_LONG);
        UstPower = 0;
        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        flAllOff = 1;

        break;
    }
}
//    case 117:

void ProcessDistillFractional()
{
    if (flAlarmUroven && StateMachine < 4) { // Если сработал датчик уровня, то переходим на следующую фракцию.
        TekFraction++;
        SecTempPrev = Seconds;
        flAlarmUroven = 0;
        CountAlarmUroven = 0;
        if (TekFraction < CountFractionDist)
            SetAngle(AngleFractionDist[TekFraction]);
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    }

    // Проверяем температуру в ТСА
    if (DS_TEMP(TEMP_TSA) >= MAX_TEMP_TSA) {
        StateMachine = 101; // Переводим автомат в состояние аварии по ТСА.
        UstPower = 0;
    }

    if (flAlarmMPX5010)
        StateMachine = 100; // Переводим в режим тревоги по датчику давления

    switch (StateMachine) {
    case 0: // Процесс не запущен
        break;
    case 1: // Начало процесса
        PrepareProcess();
        //          if (FlToGSM && lastSMSState!=StateMachine) StateToSMS();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif

        KlOpen[KLP_VODA] = PER_KLP_OPEN;
        KlClose[KLP_VODA] = PER_KLP_CLOSE;
        StateMachine = 2;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        //    digitalWrite(PIN_REG_ON,HIGH);
        digitalWrite(PIN_RZG_ON, RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, !ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, RELAY_HIGH);
        digitalWrite(PIN_START_2, RELAY_HIGH);
        flAllOff = 0;

    case 2: // Ожидание, пока прогреется термометр в дефлегматоре

        UstPower = Power; // Устанавливаем максимальную мощность для разгона
        // Подчиненные переводим в разгон
        digitalWrite(PIN_SLAVE_0, 1);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        // Если температура в дефлегматоре меньше 70 градусов и датчик дефлегматора в принципе подключен, тогда ждем, пока
        // дефлегматор прогреется, чтобы запустить воду в холодильник (это для некоторой экономии воды)
        if (DS_TEMP(TEMP_DEFL) < TempDeflBegDistil && ds1820_devices > 1 || (TempDeflBegDistil < 0 && DS_TEMP(TEMP_KUB) < -TempDeflBegDistil))
            break;

        // Подаем питание на клапан подачи воды холодильника
        // для этого синхронизируем его с клапаном общей подачи воды (возможно, при этом будет меньше гидроударов, хотя я их и так не наблюдаю)
        KlOpen[KLP_HLD] = KlOpen[KLP_VODA];
        KlClose[KLP_HLD] = KlClose[KLP_VODA];
        KlState[KLP_HLD] = KlState[KLP_VODA];
        KlCount[KLP_HLD] = KlCount[KLP_VODA];
        StateMachine = 3;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        TekFraction = 0;
        SetAngle(AngleFractionDist[TekFraction]);
        SecTempPrev = Seconds;

    case 3: // Ждем, пока температура в кубе не превысит установленную
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 1); // Переводим подчиненный в режим дистилляции
        digitalWrite(PIN_SLAVE_3, 0);

        if (TekFraction < CountFractionDist) {
            if (TekFraction == 0 && CountFractionDist == 2) // Если число фракций равно ровно две, тогда отбираем первую фракцию через клапан голов, остальные через клапан тела
            {
                // Для первой фракции (считаем ее головами) открываем клапан отбора голов, клапан тела закрываем.
                KlOpen[KLP_SR] = 0;
                KlClose[KLP_SR] = 10;
                KlOpen[KLP_GLV_HVS] = 200;
                KlClose[KLP_GLV_HVS] = 0;
            } else { // Для последующих фракций (считаем их телом) клапан тела открываем, клапан голов закрываем.
                KlOpen[KLP_SR] = 200;
                KlClose[KLP_SR] = 0;
                KlOpen[KLP_GLV_HVS] = 0;
                KlClose[KLP_GLV_HVS] = 10;
            }
            UstPower = PowerFractionDist[TekFraction]; // Устанавливаем максимальную мощность для разгона
            TekTemp = TempFractionDist[TekFraction]; //Текущее состояние - первый недробный перегон
            if (TekTemp >= 0) // Значение больше нуля, значит это температура
            {
                if (DS_TEMP(TEMP_KUB) >= TekTemp) {
                    TekFraction++;
                    SecTempPrev = Seconds;
                    if (TekFraction < CountFractionDist)
                        SetAngle(AngleFractionDist[TekFraction]);
                }
            } else {
                // Значение меньше нуля, значит это время в минутах
                SecOstatok = -((Seconds - SecTempPrev) / 60 + TekTemp);
                // Если времени не осталось, переходим на следующую фракцию
                if (SecOstatok <= 0) {
                    TekFraction++;
                    SecTempPrev = Seconds;
                    if (TekFraction < CountFractionDist)
                        SetAngle(AngleFractionDist[TekFraction]);
                }
            }
            break;
        } else {
            StateMachine = 4;
            time3 = 2 * 60 + ((int)(FlAvtonom >> 1)) * 60; // Через две минуты отключим воду
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }

    case 4: // Ждем минуту для окончания подачи воды
        UstPower = 0;
        // Закрываем клапана на отбор.
        KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = 10;
        KlOpen[KLP_GLV_HVS] = 0;
        KlClose[KLP_GLV_HVS] = 10;
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);

        // Отключаем подчиненные
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);
        //    SecondsEnd=SecondsEnd-(Seconds-SecTempPrev);
        //    SecTempPrev=Seconds;
        //
        //    if (SecondsEnd<=0)
        if (time3 == 0) {
            // Переводим автомат в конечное состояние
            StateMachine = 100;
            SecondsEnd = Seconds;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }
        break;
    case 101: // Превышение температуры в ТСА!!!
        my_beep(3 * BEEP_LONG);
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата
        // Отключаем на всякий случай все!
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        if (BeepEndProcess)
            my_beep(BEEP_LONG);
        UstPower = 0;
        // Отключаем подчиненные
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);
        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        flAllOff = 1;

        break;
    }
}
//    case 112:

void ProcessNBK()
{

    //#define KLP_NPG  0      // Номер клапана для управления НПГ (при дистилляции)
    //#define KLP_VODA 1      // Номер клапана для управления общей подачей воды в систему
    //#define KLP_DEFL 2      // Номер клапана для подачи воды в дейфлегматор
    //#define KLP_GLV_HVS  3       // Номер клапана отбора головных и хвостовых фракций
    //#define KLP_SR   4      // Номер клапана отбора ректификата
    //#define TEMP_KUB  0      // Номер датчика  термометра в кубе
    //#define TEMP_RK20 1      // Номер датчика термометра в РК 20 см от насадки
    //#define TEMP_TSA  2      // Номер термометра в трубке связи с атмосферой
    if (flAlarmUroven && StateMachine < 100) { // Переводим автомат в стадию завершения
        StateMachine = 100;
    }
    if (flAlarmMPX5010)
        StateMachine = 100; // Переводим в режим тревоги по датчику давления

    switch (StateMachine) {
    case 0: // Процесс не запущен
        break;
    case 1: // Начало процесса
        PrepareProcess();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif
        KlOpen[KLP_VODA] = PER_KLP_OPEN;
        KlClose[KLP_VODA] = PER_KLP_CLOSE;
        StateMachine = 2;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        MaxPressByPeriod = 0;
    case 2:
        UstPower = Power;
        digitalWrite(PIN_START_1, RELAY_HIGH);
        digitalWrite(PIN_START_2, RELAY_HIGH);
        if (DS_TEMP(TEMP_DEFL) < TempDeflBegDistil && ds1820_devices > 1 || (TempDeflBegDistil < 0 && DS_TEMP(TEMP_KUB) < -TempDeflBegDistil))
            break;
        KlOpen[KLP_HLD] = PER_KLP_OPEN;
        KlClose[KLP_HLD] = PER_KLP_CLOSE;
        StateMachine = 3;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    case 3:
        SpeedNBK = 0;
        UstPower = PowerNBK;
        time1 = 20 * 60; // Задаем время 20 минут с начала старта
        time2 = 0;
        my_beep(BEEP_LONG * 3);
        break;
    case 4:
        UstPower = PowerNBK;
        timeNBK = 20;
        // Работаем в следующем режиме - 2 минуты пауза, 1 минута работает миксер.
        if (time3 == 0) {
            time3 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
        }

        if (time2 == 0) {
            time2 = (int)timePressNBK * 5;
            ProcessPIDPress((int)minPressNBK * 5, MaxPressByPeriod);

            if (MaxPressByPeriod > (int)minPressNBK * 5 + (int)deltaPressNBK && minPressNBK > 0) { // Если превышено давление, то переходим в режим прекращения подачи браги.
                // Если минимальное давление не установлено, то ситуацию игнорируем.
                StateMachine = 6;
                time2 = ((int)timePressNBK * 5) / 8;
                if (BeepStateProcess)
                    my_beep(BEEP_LONG / 2);
                break;
            }
            if (MaxPressByPeriod < (int)minPressNBK * 5 && deltaPressNBK > 0) {
                SpeedNBKDst++;
                if (BeepStateProcess)
                    my_beep(BEEP_LONG / 2);
            }
            // Взводим таймер на время, определяемое мощностью регулятора мощности.
            MaxPressByPeriod = U_MPX5010;
        }
        // Если превысило двойную дельту, то не дожидаемся, пока окончится время и сразу переходим в режим превышения по давлению.
        // Если минимальное давление не установлено, то ситуацию игнорируем.
        if (MaxPressByPeriod > (int)minPressNBK * 5 + (int)deltaPressNBK + (int)deltaPressNBK && deltaPressNBK > 0 && minPressNBK > 0) {
            StateMachine = 6;
            time2 = ((int)timePressNBK * 5) / 8;
            if (BeepStateProcess)
                my_beep(BEEP_LONG / 2);
            break;
        }
        if (SpeedNBKDst > 125)
            SpeedNBKDst = 125;

        SpeedNBK = SpeedNBKDst;

        if (DS_TEMP(TEMP_DEFL) >= 980 && time1 <= 0) {
            //Если температура превысила максимальную и с момента старта прошло более 20 минут, то считаем, что произошла авария - заклинил насос или кончилась брага.
            // Ждем 5 минут и завершаем процесс.
            time2 = 5 * 60;
            StateMachine = 5;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }
        break;
    case 5:
        if (SpeedNBKDst > 125)
            SpeedNBKDst = 125;
        SpeedNBK = SpeedNBKDst;
        timeNBK = 20;

        if (DS_TEMP(TEMP_DEFL) < 970) {
            StateMachine = 4;
        }
        my_beep(BEEP_LONG * 3);
        if (time2 <= 0) {
            StateMachine = 100;
        }
        break;
    case 6: // Превышение давления, ожидаем снижения давления, подача браги при этом прекращается.
        UstPower = PowerNBK;
        timeNBK = 20;

        time1 = (int)timePressNBK * 5 * 4; // В процессе превышения давления не анализируем температуру верха колонны в течение четырехкратного периода анализа давления.

        // Работаем в следующем режиме - 2 минуты пауза, 1 минута работает миксер.
        if (time3 == 0) {
            time3 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
        }

        if (time2 == 0) {

            time2 = (int)timePressNBK * 5 / 8;
            ProcessPIDPress((int)minPressNBK * 5, MaxPressByPeriod);
            if (MaxPressByPeriod < (int)minPressNBK * 5 + (int)deltaPressNBK) // Если давление вернулось в норму, уменьшаем подачу и переходим в режим подачи браги
            {
                if (deltaPressNBK > 0)
                    SpeedNBKDst -= 1;
                // Время анализа ставим вдвое меньше, чем на превышение.
                time2 = (int)timePressNBK * 5 / 4;
                StateMachine = 4;
                if (BeepStateProcess)
                    my_beep(BEEP_LONG / 2);
            }
            // Взводим таймер на время, определяемое настройками
            MaxPressByPeriod = U_MPX5010;
        } else if (U_MPX5010 <= (int)minPressNBK * 5 || minPressNBK <= 0) // Если текущее давление стало меньше или равно минимальному, тогда заканчиваем прекращать подачу браги
        { // или если минимальное давление не установлено, что означает отключение его контроля
            if (deltaPressNBK > 0)
                SpeedNBKDst -= 1;
            // Время анализа ставим вдвое меньше, чем на превышение.
            time2 = (int)timePressNBK * 5 / 4;
            StateMachine = 4;
            if (BeepStateProcess)
                my_beep(BEEP_LONG / 2);
            MaxPressByPeriod = U_MPX5010;
        }

        SpeedNBK = 0;
        break;

    case 101: // Превышение температуры в ТСА!!!
        break;
        my_beep(3 * BEEP_LONG);
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата
        SpeedNBK = 0;
        timeNBK = 0;
        if (StateMachine == 100 && BeepEndProcess)
            my_beep(BEEP_LONG);
        UstPower = 0;
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        flAllOff = 1;
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        break;
    }
}
//    case 111:

void ProcessNDRF()
{

    //#define KLP_NPG  0      // Номер клапана для управления НПГ (при дистилляции)
    //#define KLP_VODA 1      // Номер клапана для управления общей подачей воды в систему
    //#define KLP_DEFL 2      // Номер клапана для подачи воды в дейфлегматор
    //#define KLP_GLV_HVS  3       // Номер клапана отбора головных и хвостовых фракций
    //#define KLP_SR   4      // Номер клапана отбора ректификата
    //#define TEMP_KUB  0      // Номер датчика  термометра в кубе
    //#define TEMP_RK20 1      // Номер датчика термометра в РК 20 см от насадки
    //#define TEMP_TSA  2      // Номер термометра в трубке связи с атмосферой
    static char Count_Provodimost = 0; // Количество срабатываний датчика проводимости для окончания отбора голов.
    static char OldStateMachine = 0; // Количество срабатываний датчика проводимости для окончания отбора голов.
    int V3; // Текущий уровень проводимости;

    if (flAlarmUroven && StateMachine < 8) { // Переводим автомат в стадию завершения
        if (USE_ALARM_UROVEN == 1) {
            StateMachine = 8; // Переводим в режим окончания
            SecTempPrev = Seconds; // Запомним время, когда закончилась ректификация
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else {
            OldStateMachine = StateMachine;
            StateMachine = 9; // Переводим в режим ожидания
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            Count_Provodimost = 0;
            KlOpen[KLP_GLV_HVS] = 0;
            KlClose[KLP_GLV_HVS] = 100;
            KlOpen[KLP_SR] = 0;
            KlClose[KLP_SR] = 100;
        }
    }

    // Проверяем температуру в ТСА
    if (DS_TEMP(TEMP_TSA) >= MAX_TEMP_TSA) {
        StateMachine = 101; // Переводим автомат в состояние аварии по ТСА.
    }

    if (flAlarmMPX5010)
        StateMachine = 102; // Переводим в режим тревоги по датчику давления

    // Проверяем окончание температуры окончания ректификации
    if (DS_TEMP(TEMP_KUB) >= tEndRect && StateMachine < 8 && IspReg != 118) {
        StateMachine = 8;
        SecTempPrev = Seconds; // Запомним время, когда закончилась ректификация
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    }

    switch (StateMachine) {
    case 0: // Процесс не запущен
        break;
    case 1: // Начало процесса
        PrepareProcess();
        KlReg[KLP_GLV_HVS] = PEREGREV_ON;
        KlReg[KLP_SR] = PEREGREV_ON;

        //          if (FlToGSM && lastSMSState!=StateMachine) StateToSMS();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif

        KlOpen[KLP_VODA] = PER_KLP_OPEN;
        KlClose[KLP_VODA] = PER_KLP_CLOSE;
        //    digitalWrite(PIN_REG_ON,HIGH);
        digitalWrite(PIN_RZG_ON, RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, !ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, RELAY_HIGH);
        digitalWrite(PIN_START_2, RELAY_HIGH);
        flAllOff = 0;
        StateMachine = 2;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        Count_Provodimost = 0;
    case 2: // Разгон
        UstPower = Power; // Устанавливаем максимальную мощность для разгона

        // Учитываем как температуру в кубе, так и температуру в колонне, в зависимости от того, положительные или отрицательные значения температур.
        if (DS_TEMP(TEMP_KUB) < tEndRectRazgon && tEndRectRazgon > 0)
            break;
        if (DS_TEMP(TEMP_RK20) < -tEndRectRazgon && tEndRectRazgon <= 0)
            break;

        // Подаем питание на клапан дефлегматора
        KlOpen[KLP_DEFL] = PER_KLP_OPEN;
        KlClose[KLP_DEFL] = PER_KLP_CLOSE;
        TempPrev = DS_TEMP(TEMP_RK20);
        SecTempPrev = Seconds; // Запоминаем дату последнего изменения температуры
        SecOstatok = 1800; // Стаблилизация колонный по-умолчанию полчаса
        StateMachine = 3;
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        if (BeepStateProcess)
            my_beep(BEEP_LONG);

    case 3: // Стабилизация температуры (строго по времени)
        UstPower = PowerRect;
        // Польский буфер откроем.
        KlOpen[KLP_PB] = 200;
        KlClose[KLP_PB] = 0;

        if (DS_TEMP(TEMP_RK20) > 0) // Если температура больше 70 градусов (вдруг куб прогрелся, а колонна еще нет, нелогично, конечно, но на всякий случай учтем)
        {
            // Стабилизация строго по температуре
            SecOstatok = SecOstatok - (Seconds - SecTempPrev);
            SecTempPrev = Seconds;
            if (SecOstatok <= 0) {
                StateMachine = 4;
                SecTempPrev = Seconds;
            } else
                break;
        } else {
            SecTempPrev = Seconds; // Запоминаем дату последнего изменения температуры
            break;
        }

    case 4: // Отбор голов
        UstPower = PowerRect;
        //unsigned int timeChimRectOtbGlv=2000;  // Шим отбора голов (в полупериодах сетевого напряжения в данном случае это около 20 секунд)
        //unsigned char ProcChimOtbGlv=5;  // Процент ШИМ отбора голов (в данном случае это 5 процентов)
        // Устанавливаем клапан отбора хвостов и голов в соответвии с установленнным ШИМ
        //int tEndRectOtbGlv=854;     // Температура окончания отбора голов 85.4 С
        KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = 10;
        // Польский буфер закроем.
        KlOpen[KLP_PB] = 0;
        KlClose[KLP_PB] = 10;

        if (ProcChimOtbGlv > 0)
            KlOpen[KLP_GLV_HVS] = (timeChimRectOtbGlv / 100) * ProcChimOtbGlv;
        else
            KlOpen[KLP_GLV_HVS] = -ProcChimOtbGlv;

        KlClose[KLP_GLV_HVS] = timeChimRectOtbGlv - KlOpen[KLP_GLV_HVS];
        tStabSR = DS_TEMP(TEMP_RK20); // Это температура, относительно которой будем стабилизировать отбор

        if (UrovenProvodimostSR == 0) // Если проводимость спирта не используется, головы отбираем по температуре
        {
            if (DS_TEMP(TEMP_KUB) < tEndRectOtbGlv) { // Пока температура в кубе не выросла выше окончания отбора голов, продолжаем это состояни
                break;
            }
        } else {

            if (UrovenProvodimostSR > 0) // Если проводимость больше нуля, используем проводимость в том или ином виде
            {
                V3 = U_GLV;
                // Если проводимость менее заданного уровня, и проводимость вообще сущетвует, то увеличиваем счетчик срабатываний
                // или ести проводимость больше уровня тревоги и установлен признак отбирать головы по уровню в приемной емкости.

                if ((V3 < UrovenProvodimostSR && V3 >= 2 && UrovenProvodimostSR >= 5) || (V3 >= UROVEN_ALARM && UrovenProvodimostSR <= 2)) {
                    my_beep(BEEP_LONG); // Сначала пищим, предупреждая.
                    Count_Provodimost++;
                } else
                    Count_Provodimost = 0;

                // Если счетчик срабатываний не превысил максимальное число раз, тогда продолжаем процесс отбора голов.
                if (Count_Provodimost < MAX_COUNT_PROVODIMOST_SR)
                    break;
            } else // Если уровень проводимости SR меньше нуля, тогда это означает, что отбор голов производится по времени (в десятках минут).
            {
                // Анализируем, сколько минут прошло с начала отбора голов.
                SecOstatok = -((Seconds - SecTempPrev) / 600 + UrovenProvodimostSR);
                if (SecOstatok > 0)
                    break;
            }
        }

        // Если температура превысила указанный рубеж, то переходим в состояние отбора спирта-ректификата.
        // Клапан отбора голов и хвостов отключаем
        tStabSR = DS_TEMP(TEMP_RK20); // Это температура, относительно которой будем стабилизировать отбор
        KlOpen[KLP_GLV_HVS] = 0;
        KlClose[KLP_GLV_HVS] = 10;
        ProcChimSR = begProcChimOtbSR;
        StateMachine = 5;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);

    case 5: // Ожидание, пока температура не вернется к состоянию стабилизации
        // Это для отладки страт-стопа, на самом деле температура 28,5 градуса в этом режиме в колонне быть не может
        if (tStabSR == 0)
            tStabSR = 285;

        KlOpen[KLP_GLV_HVS] = 0;
        KlClose[KLP_GLV_HVS] = 10;
        KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = 10;
        if (DS_TEMP(TEMP_KUB) >= tEndRectOtbSR) {
            StateMachine = 7;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            break;
        }

        if (DS_TEMP(TEMP_RK20) > tStabSR) {
            break;
        }
        SecTempPrev = Seconds; // Запомним время, когда стабилизировалась температура
        StateMachine = 6;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    case 6: // Отбор СР
        //unsigned int timeChimRectOtbSR=1000;  // Шим отбора СР (в полупериодах сетевого напряжения в данном случае это около 10 секунд)
        //unsigned char tDeltaRect=10;  // Дельта ректификации (в десятых градуса Цельсия) в данном случае - это 10 десятых, то есть 1 градус Цельсия
        //unsigned int tEndRectOtbSR=965;  // Температура окончания отбора спирта-ректификата и начала отбора хвостов (в данном случе 96.5 С)
        // Устанавливаем ШИМ отбора СР
        UstPower = PowerRect;
        if (ProcChimOtbCP >= 0) {
            if (ProcChimOtbGlv > 0)
                KlOpen[KLP_GLV_HVS] = (timeChimRectOtbGlv / 100) * ProcChimOtbGlv;
            else
                KlOpen[KLP_GLV_HVS] = -ProcChimOtbGlv;

            KlOpen[KLP_GLV_HVS] = ((long)KlOpen[KLP_GLV_HVS] * (long)ProcChimOtbCP) / 100;
            KlClose[KLP_GLV_HVS] = timeChimRectOtbGlv - KlOpen[KLP_GLV_HVS];

        } else {
            KlOpen[KLP_GLV_HVS] = -ProcChimOtbCP;
            KlClose[KLP_GLV_HVS] = timeChimRectOtbSR - KlOpen[KLP_GLV_HVS];
        }
        // Польский буфер закроем.
        KlOpen[KLP_PB] = 0;
        KlClose[KLP_PB] = 10;

        if (ProcChimSR > 0)
            KlOpen[KLP_SR] = ((timeChimRectOtbSR / 10) * ProcChimSR) / 10;
        else
            KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = timeChimRectOtbSR - KlOpen[KLP_SR];

        if (DS_TEMP(TEMP_RK20) >= tStabSR + tDeltaRect) // Если текущая температура превысила базовую, тогда останавливаем отбор
        {
            StateMachine = 5;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }

        // Если температура в кубе превысила температуру при которой надо отбирать СР
        if (DS_TEMP(TEMP_KUB) >= tEndRectOtbSR) {
            StateMachine = 7;
            KlOpen[KLP_SR] = 0; // Отключаем клапана отбора СР
            KlClose[KLP_SR] = 10;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else
            break;
    case 7: // Отбор Хвостов
        UstPower = PowerRect;
        KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = 10;
        KlOpen[KLP_GLV_HVS] = (timeChimRectOtbGlv / 100) * 90;
        KlClose[KLP_GLV_HVS] = timeChimRectOtbGlv - KlOpen[KLP_GLV_HVS];
        if (DS_TEMP(TEMP_KUB) >= tEndRect) {
            StateMachine = 8;
            SecTempPrev = Seconds; // Запомним время, когда закончилась ректификация
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else
            break;
    case 8: // Ждем три минуты для окончания подачи воды
        UstPower = 0;
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        KlOpen[KLP_GLV_HVS] = 0; // Отключаем клапана отбора хвостов
        KlClose[KLP_GLV_HVS] = 10;

        if (Seconds - SecTempPrev > 180 + ((int)(FlAvtonom >> 1)) * 60) {
            // Переводим автомат в конечное состояние
            StateMachine = 100;
            SecondsEnd = Seconds;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }
        break;
        V3 = U_UROVEN;
        if (V3 < UROVEN_ALARM) {
            Count_Provodimost++;
#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("ALARM VOZVRAT U_UROVEN=%i cnt=%i"), U_UROVEN, Count_Provodimost);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif
            // По прошествии минуты устанавливаем флаг сработки.
            if (Count_Provodimost > 2) {
                flAlarmUroven = 0;
                StateMachine = OldStateMachine;
                Count_Provodimost = 0;
            }
        } else {
            Count_Provodimost = 0;
        }
        break;
    case 101: // Превышение температуры в ТСА!!!
        my_beep(2 * BEEP_LONG);
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата

        // Отключаем на всякий случай все!
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif

        if (StateMachine == 100 && BeepEndProcess)
            my_beep(BEEP_LONG);

        UstPower = 0;
        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        flAllOff = 1;

        break;
    }
}
//    case 108:
//    case 113:

void ProcessRazvarZerno()
{
    switch (StateMachine) {
    case 0: //Не запущено

        break;
    case 1: //Нагрев до температуры 50 градусов
        PrepareProcess();
        //    TempHLDZatorBrog1=36;
        //    if (TempZSP<40) TempZSP=40;
        //    if (TempZSPSld<55) TempZSPSld=55;
        timeNBK = 0;
        SpeedNBK = 0;
        // Этот пункт пропускаем, потому что и так работает мешалка.
        //      digitalWrite(PIN_RZG_ON,!RELAY_HIGH);
        //
        //      // Если необходимо отправлять состояние на сотовый и это состояние еще не отправлялось, тогда информируем о состоянии процесса
        //  //    if (FlToGSM && lastSMSState!=StateMachine) StateToSMS();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif
        //
        //
        if (DS_TEMP(TEMP_RAZVAR) < (int)TempZSP * 10) {
            ProcessPIDTemp((int)TempZSP * 10, DS_TEMP(TEMP_RAZVAR));
            UstPower = (long)Power * KtT / 1000;
            // Мощность ограничена мощностью варки зерна
            if (UstPower > PowerVarkaZerno)
                UstPower = PowerVarkaZerno;
            break;
        }
        StateMachine = 2;
        //      if (BeepStateProcess) my_beep(BEEP_LONG);

    case 2: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
        // Из этого режима можно выйти только вручную, нажав кнопку вверх или отладочной функцией одновременно нажав вверх и вниз.
        // Пищим, чтобы привлечь внимание
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        ProcessPIDTemp(500, DS_TEMP(TEMP_RAZVAR));
        UstPower = (long)Power * KtT / 1000;
        // Мощность ограничена мощностью варки зерна
        if (UstPower > PowerVarkaZerno)
            UstPower = PowerVarkaZerno;
        break;
        StateMachine = 3;
        time1 = 0;
        timeMIXER = 0;
    case 3: //Нагрев до температуры 64 градуса
        if (DS_TEMP(TEMP_RAZVAR) < (int)TempZSPSld * 10) {
            // Работаем согласно настроек пивоварения (по умолчанию 10 минут работы, 2 минуты пауза).
            if (time1 == 0) {
                time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
                timeMIXER = (int)tempP[0] * 60;
            }
            ProcessPIDTemp((int)TempZSPSld * 10, DS_TEMP(TEMP_RAZVAR));
            UstPower = (long)Power * KtT / 1000;
            // Мощность ограничена мощностью варки зерна
            if (UstPower > PowerVarkaZerno)
                UstPower = PowerVarkaZerno;
            break;
        }
        time1 = 0;
        timeMIXER = 0;
        StateMachine = 4;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        SecTempPrev = Seconds;
        time2 = 15 * 60; // Взводим таймер на 15 секунд паузы
        if (IspReg == 113) {
            // Если затор из муки и солода, переходим сразу к размешиванию и последующему осахариванию.
            time2 = 10 * 60; // Устанавливаем время на работу мешалки после засыпки солода - 10 минут
            time1 = 0;
            StateMachine = 9;
            break;
        }

    case 4: //Ожидание 15 минут, поддержка температуры
        timeNBK = 0;
        SpeedNBK = 0;

        // Работаем в следующем режиме - 2 минуты пауза, 1 минута работает миксер.
        if (time1 == 0) {
            time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
        }

        ProcessPIDTemp((int)TempZSPSld * 10, DS_TEMP(TEMP_RAZVAR));
        UstPower = (long)Power * KtT / 1000;
        // Мощность ограничена мощностью варки зерна
        if (UstPower > PowerVarkaZerno)
            UstPower = PowerVarkaZerno;

        // Закончилось время паузы
        if (time2 == 0) {
            StateMachine = 5;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else
            break;

    case 5:
        timeNBK = 0;
        SpeedNBK = 0;
        //  Нагрев до закипания закипание определяется по преввшениею температуры либо в заторнике, либо в верхней части заторника.
        //  Миксер Работаем в следующем режиме - 5 минут пауза, 1 минута работает миксер.
        if (time1 == 0) {
            time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
        }

        UstPower = PowerVarkaZerno;

        if ((DS_TEMP(TEMP_RAZVAR) >= TempKipenZator && ds1820_devices < 2) || DS_TEMP(TEMP_DEFL) >= TempKipenZator) // Если температура затора, либо температура дефлегматора больше заданной
        {
            // Переходим к следующему этапу - варка 1 часа.
            StateMachine = 6;
            time2 = 1 * 60 * 60; // Это 1.5 часа в секундах
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else
            break;

    case 6: //Варка
        timeNBK = 0;
        SpeedNBK = 0;
        UstPower = PowerRazvZerno;
        //  Миксер Работаем в следующем режиме - 5 минут пауза, 1 минута работает миксер.
        if (time1 == 0) {
            time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
        }

        if (time2 == 0) {
            StateMachine = 7;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            time1 = 0;
        } else
            break;
    case 7: //охлаждение до температуры осахаривания.
        timeNBK = 0;
        SpeedNBK = 0;
        UstPower = 0;
        // Мешалку и воду включаем одновременно, две минуты работаем, минуту отдыхаем.
        if (time1 == 0) {
            time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
            KlOpen[KLP_HLD] = PER_KLP_OPEN;
            KlClose[KLP_HLD] = PER_KLP_CLOSE;
        }
        // Если миксер закончил вращаться, то холодильник тоже отрубаем.
        if (timeMIXER == 0) {
            KlOpen[KLP_HLD] = 0;
            KlClose[KLP_HLD] = 100;
        }

        if (DS_TEMP(TEMP_RAZVAR) <= (int)TempZSPSld * 10) {
            // Если температура меньше либо равна температуре засыпки солода, то переходим к следующему этапу
            // - ожидание засыпки солода.
            StateMachine = 8;
        } else
            break;
    case 8: //ожидание засыпки солода, поддержка температуры
        timeNBK = 0;
        SpeedNBK = 0;
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        KlOpen[KLP_HLD] = 0;
        KlClose[KLP_HLD] = 100;
        timeMIXER = 0;
        // Из этого режима можно выйти только вручную, нажав кнопку вверх или отладочной функцией одновременно нажав вверх и вниз.
        // Пищим, чтобы привлечь внимание
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        ProcessPIDTemp((int)TempZSPSld * 10, DS_TEMP(TEMP_RAZVAR));
        UstPower = (long)Power * KtT / 1000;
        // Мощность ограничена мощностью варки зерна
        if (UstPower > PowerVarkaZerno)
            UstPower = PowerVarkaZerno;
        time2 = 10 * 60; // Устанавливаем время на работу мешалки после засыпки солода - 10 минут
        time1 = 0;
        break;
    case 9: // Мешаем затор 10 минут.
        timeNBK = 0;
        SpeedNBK = 0;

        KlOpen[KLP_HLD] = 0;
        KlClose[KLP_HLD] = 100;

        ProcessPIDTemp((int)TempZSPSld * 10, DS_TEMP(TEMP_RAZVAR));
        UstPower = (long)Power * KtT / 1000;
        // Мощность ограничена мощностью варки зерна
        if (UstPower > PowerVarkaZerno)
            UstPower = PowerVarkaZerno;

        if (time2 > 0) {
            // Время работы - 1.5 минуты работы, 30 секунд отдых
            if (time1 == 0) {
                time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
                timeMIXER = (int)tempP[0] * 60;
            }

            break;
        }
        StateMachine = 10;
        time2 = 60 * 60; // Время осахаривания - 1 час.
        if (IspReg == 113) {
            time2 = 60 * 60 * 2 / 3; // Время осахаривания - 1.5 часа.
        }

    case 10: // осахаривание затора.
        timeNBK = 0;
        SpeedNBK = 0;
        KlOpen[KLP_HLD] = 0;
        KlClose[KLP_HLD] = 100;
        if (time2 > 0) {
            // Время работы - 1 минуты работы, 10 минут отдых, то есть при осахаривании меняем время работы и время отдыха
            if (time1 == 0) {
                time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
                timeMIXER = (int)timeP[0] * 60;
            }

            ProcessPIDTemp((int)TempZSPSld * 10, DS_TEMP(TEMP_RAZVAR));
            UstPower = (long)Power * KtT / 1000;
            // Мощность ограничена мощностью варки зерна
            if (UstPower > PowerVarkaZerno)
                UstPower = PowerVarkaZerno;

            break;
        }
        StateMachine = 11;
        time1 = 0;
    case 11: // Остужение до 40 градусов
        // Мешалку и воду включаем одновременно, две минуты работаем, минуту отдыхаем.
        UstPower = 0;
        if (time1 == 0) {
            time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
            KlOpen[KLP_HLD] = PER_KLP_OPEN;
            KlClose[KLP_HLD] = PER_KLP_CLOSE;
        }
        // Если миксер закончил вращаться, то холодильник тоже отрубаем.
        if (timeMIXER == 0) {
            KlOpen[KLP_HLD] = 0;
            KlClose[KLP_HLD] = 100;
        }

        if (DS_TEMP(TEMP_RAZVAR) <= 400) {
            // Если температура меньше 40 градосов, то зададим небольшую порцию дрожжей
            StateMachine = 12;
            SpeedNBK = 125;
            timeNBK = 5; // Запускаем насос для первоначальной задачи дрожжей
            time1 = 0;
            my_beep(BEEP_LONG * 3);

        } else
            break;

    case 12: // Остужение до 355 градусов
        UstPower = 0;
        // Мешалку и воду включаем одновременно, две минуты работаем, 60 секунд отдыхаем.
        if (time1 == 0) {
            time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
            KlOpen[KLP_HLD] = PER_KLP_OPEN;
            KlClose[KLP_HLD] = PER_KLP_CLOSE;
        }
        // Если миксер закончил вращаться, то холодильник тоже отрубаем.
        if (timeMIXER == 0) {
            KlOpen[KLP_HLD] = 0;
            KlClose[KLP_HLD] = 100;
        }

        if (DS_TEMP(TEMP_RAZVAR) <= (int)TempHLDZatorBrog1 * 10) {
            // переходи  к следующему этапу - поддержка брожения.
            StateMachine = 13;
            SpeedNBK = 125;
            timeNBK = 25; // Запускаем насос для окончательной задачи дрожжей
            time1 = 0;
            my_beep(BEEP_LONG * 3);
        } else
            break;
    case 13: // Поддержка брожения - температура нормальна
        timeNBK = 0;
        SpeedNBK = 0;
        // Поперла пена - мешаем чаще и пищим
        if (flAlarmUroven) {

            my_beep(BEEP_LONG * 5);
            if (time1 == 0 || time1 > 3 * 60) {
                time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
                timeMIXER = (int)tempP[0] * 60;
            }
            flAlarmUroven = 0;
        }

        UstPower = 0;
        // Мешалку и воду включаем одновременно, две минуты работаем, 60 секунд отдыхаем.
        if (DS_TEMP(TEMP_RAZVAR) <= (int)TempHLDZatorBrog1 * 10) {
            KlOpen[KLP_HLD] = 0;
            KlClose[KLP_HLD] = 1000;
            //Если температура меньше 36 градусов, тогда периодически помешиваем, раз в 20 минут на 10 секунд
            if (time1 == 0) {
                time1 = 10 * 60;
                timeMIXER = 30;
            }
            break;
        } else {
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            StateMachine = 14;
            time1 = 0;
        }

    case 14: // Поддержка брожения температура превышена
        // Поперла пена
        if (flAlarmUroven) {
            my_beep(BEEP_LONG * 5);
            if (time1 == 0) {
                // Звоним на сотовый, что поперла пена раз в три минуты - заливает!!!
                // На сотовый будет звонок при предупреждении
            }
            flAlarmUroven = 0;
        }

        if (DS_TEMP(TEMP_RAZVAR) > (int)TempHLDZatorBrog1 * 10 - 5) {
            //Если температура меньше 36 градусов, тогда интенсивно мешаем, раз в 15 минут на 60 секунд
            if (time1 == 0) {
                time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
                timeMIXER = (int)timeP[0] * 60;
                KlOpen[KLP_HLD] = PER_KLP_OPEN;
                KlClose[KLP_HLD] = PER_KLP_CLOSE;
            }

            if (timeMIXER == 0) {
                KlOpen[KLP_HLD] = 0;
                KlClose[KLP_HLD] = 100;
            }
            break;
        } else {
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            StateMachine = 13;
            time1 = 0;
        }
        break;

    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: //Окончание
        // Если необходимо отправлять состояние на сотовый и это состояние еще не отправлялось, тогда информируем о состоянии процесса
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        if (BeepEndProcess)
            my_beep(BEEP_LONG);
        UstPower = 0;
        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        flAllOff = 1;

        break;
    }
}

//case 114:

void ProcessHLDZatorByChiller()
{
    switch (StateMachine) {
    case 0: //Не запущено
        break;
    case 1: //Инициализация
        PrepareProcess();
        timeNBK = 0;
        SpeedNBK = 0;
        // Этот пункт пропускаем, потому что и так работает мешалка.
        //
        //
        //      // Если необходимо отправлять состояние на сотовый и это состояние еще не отправлялось, тогда информируем о состоянии процесса
        //  //    if (FlToGSM && lastSMSState!=StateMachine) StateToSMS();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif
        //
        //
        //      if (DS_TEMP(TEMP_RAZVAR)<500)
        //      {
        //        UstPower=Power;
        //        break;
        //      }
        StateMachine = 12;
        //      if (BeepStateProcess) my_beep(BEEP_LONG);
        time1 = 0;
    case 12: // Остужение до 355 градусов
        UstPower = 0;
        // Мешалку и воду включаем одновременно, две минуты работаем, 60 секунд отдыхаем.
        if (time1 == 0) {
            time1 = 4 * 60;
            timeMIXER = 3 * 60;
            KlOpen[KLP_HLD] = PER_KLP_OPEN;
            KlClose[KLP_HLD] = PER_KLP_CLOSE;
        }
        // Если миксер закончил вращаться, то холодильник тоже отрубаем.
        if (timeMIXER == 0) {
            KlOpen[KLP_HLD] = 0;
            KlClose[KLP_HLD] = 100;
        }

        if (DS_TEMP(TEMP_RAZVAR) <= (int)TempHLDZatorBrog1 * 10) {
            // переходи  к следующему этапу - поддержка брожения.
            StateMachine = 13;
            time1 = 0;
            my_beep(BEEP_LONG * 5);
        } else
            break;
    case 13: // Поддержка брожения - температура нормальна
        timeNBK = 0;
        SpeedNBK = 0;
        // Поперла пена - мешаем чаще и пищим
        if (flAlarmUroven) {

            my_beep(BEEP_LONG * 5);
            if (time1 == 0 || time1 > 3 * 60) {
                time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
                timeMIXER = (int)tempP[0] * 60;
            }
            flAlarmUroven = 0;
        }

        UstPower = 0;
        // Мешалку и воду включаем одновременно, две минуты работаем, 60 секунд отдыхаем.
        if (DS_TEMP(TEMP_RAZVAR) <= (int)TempHLDZatorBrog1 * 10) {
            KlOpen[KLP_HLD] = 0;
            KlClose[KLP_HLD] = 1000;
            //Если температура меньше 36 градусов, тогда периодически помешиваем, раз в 20 минут на 10 секунд
            if (time1 == 0) {
                time1 = 10 * 60;
                timeMIXER = 30;
            }
            break;
        } else {
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            StateMachine = 14;
            time1 = 0;
        }

    case 14: // Поддержка брожения температура превышена
        // Поперла пена
        if (flAlarmUroven) {
            my_beep(BEEP_LONG * 5);
            if (time1 == 0) {
                // Звоним на сотовый, что поперла пена раз в три минуты - заливает!!!
                // На сотовый будет звонок при предупреждении
            }
            flAlarmUroven = 0;
        }

        if (DS_TEMP(TEMP_RAZVAR) > (int)TempHLDZatorBrog1 * 10 - 5) {
            //Если температура меньше 36 градусов, тогда интенсивно мешаем, раз в 15 минут на 60 секунд
            if (time1 == 0) {
                time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
                timeMIXER = (int)timeP[0] * 60;
                KlOpen[KLP_HLD] = PER_KLP_OPEN;
                KlClose[KLP_HLD] = PER_KLP_CLOSE;
            }

            if (timeMIXER == 0) {
                KlOpen[KLP_HLD] = 0;
                KlClose[KLP_HLD] = 100;
            }
            break;
        } else {
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            StateMachine = 13;
            time1 = 0;
        }
        break;

    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: //Окончание
        // Если необходимо отправлять состояние на сотовый и это состояние еще не отправлялось, тогда информируем о состоянии процесса
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        if (BeepEndProcess)
            my_beep(BEEP_LONG);
        UstPower = 0;
        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        flAllOff = 1;

        break;
    }
}
//    case 118: //Текущее состояние - фракционная ректификация
//    case 109: //Текущее состояние - ректификация

void ProcessRectif()
{
    static char Count_Provodimost = 0; // Количество срабатываний датчика проводимости для окончания отбора голов.
    static char OldStateMachine = 0; // Количество срабатываний датчика проводимости для окончания отбора голов.
    int V3; // Текущий уровень проводимости;

    if (flAlarmUroven && StateMachine < 8) { // Переводим автомат в стадию завершения

        if (IspReg != 118) {
            // Не фракционная перегонка и указано явно выключиться, то останавливаем процесс
            if (USE_ALARM_UROVEN == 1) {
                StateMachine = 8; // Переводим в режим окончания
                SecTempPrev = Seconds; // Запомним время, когда закончилась ректификация
                if (BeepStateProcess)
                    my_beep(BEEP_LONG);
            } else {
                //	USE_ALARM_UROVEN == 0 || USE_ALARM_UROVEN > 1
                OldStateMachine = StateMachine;
                StateMachine = 9; // Переводим в режим ожидания
                if (BeepStateProcess)
                    my_beep(BEEP_LONG);
                Count_Provodimost = 0;
                KlOpen[KLP_GLV_HVS] = 0;
                KlClose[KLP_GLV_HVS] = 100;
                KlOpen[KLP_SR] = 0;
                KlClose[KLP_SR] = 100;
            }
        } else {
            // Фракционная ректификация - переходим на следующую фракцию
            TekFraction++;
            SecTempPrev1 = Seconds;
            flAlarmUroven = 0;
            CountAlarmUroven = 0;
            if (TekFraction < CountFractionRect)
                SetAngle(AngleFractionRect[TekFraction]);
        }
    }

    // Проверяем температуру в ТСА
    if (DS_TEMP(TEMP_TSA) >= MAX_TEMP_TSA) {
        StateMachine = 101; // Переводим автомат в состояние аварии по ТСА.
    }

    // Проверяем окончание температуры окончания ректификации
    // Окончание ректификации также определяется, если шим меньше нуля, для этого минимальный шим  надо поставить 0
    if ((DS_TEMP(TEMP_KUB) >= tEndRect) && StateMachine < 8 && IspReg != 118) {
        StateMachine = 8;
        SecTempPrev = Seconds; // Запомним время, когда закончилась ректификация
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    }

    switch (StateMachine) {
    case 0: // Процесс не запущен
        break;
    case 1: // Начало процесса
        PrepareProcess();

#if ADJUST_COLUMN_STAB_TEMP
        lastStableT = 0; // Обнуляем нашу температуру, чтобы в Case 6 подхватило текущую
        SecTempPrev2 = 0; // Обнуляем счетчик времени
#endif // ADJUST_COLUMN_STAB_TEMP

        KlReg[KLP_GLV_HVS] = PEREGREV_ON;
        KlReg[KLP_SR] = PEREGREV_ON;

        //          if (FlToGSM && lastSMSState!=StateMachine) StateToSMS();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif
        KlOpen[KLP_VODA] = PER_KLP_OPEN;
        KlClose[KLP_VODA] = PER_KLP_CLOSE;
        //    digitalWrite(PIN_REG_ON,HIGH);
        digitalWrite(PIN_RZG_ON, RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, !ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, RELAY_HIGH);
        digitalWrite(PIN_START_2, RELAY_HIGH);

        flAllOff = 0;
        StateMachine = 2;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        Count_Provodimost = 0;
        if (CntCHIM < 0)
            CntCHIM = -1;

    case 2: // Разгон
        UstPower = Power; // Устанавливаем максимальную мощность для разгона
        // Подчиненные контроллеры переводим в разгон
        digitalWrite(PIN_SLAVE_0, 1);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        // Учитываем как температуру в кубе, так и температуру в колонне, в зависимости от того, положительные или отрицательные значения температур.
        if (DS_TEMP(TEMP_KUB) < tEndRectRazgon && tEndRectRazgon > 0)
            break;
        if (DS_TEMP(TEMP_RK20) < -tEndRectRazgon && tEndRectRazgon <= 0)
            break;

        // Подаем питание на клапан дефлегматора
        KlOpen[KLP_DEFL] = PER_KLP_OPEN;
        KlClose[KLP_DEFL] = PER_KLP_CLOSE;
        TempPrev = DS_TEMP(TEMP_RK20);
        SecTempPrev = Seconds; // Запоминаем дату последнего изменения температуры
        StateMachine = 3;
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        TekFraction = 0;
        SetAngle(AngleFractionRect[TekFraction]);
    case 3: // Стабилизация температуры
        UstPower = PowerRect;
        // Подчиненные контроллеры переводим в ректификацию
        digitalWrite(PIN_SLAVE_0, 1);
        digitalWrite(PIN_SLAVE_1, 1);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);
        // Польский буфер откроем.
        KlOpen[KLP_PB] = 200;
        KlClose[KLP_PB] = 0;

        if (DS_TEMP(TEMP_RK20) > 0) // Если температура больше 70 градусов (вдруг куб прогрелся, а колонна еще нет, нелогично, конечно, но на всякий случай учтем)
        {
            if (TimeStabKolonna >= 0) // Если время стабилизации колонны больше нуля, то тогда считаем время относительно последнего изменения температуры
            {
                if (abs(DS_TEMP(TEMP_RK20) - TempPrev) < 2) //Если текущая температура колонны равна температуре, запомненной ранее за исключением погрешности 0,2 градуса
                {
                    if (Seconds - SecTempPrev > TimeStabKolonna) //Если с момента последнего измерения прошло более 600 секунд (10 минут), тогда считаем, что температура в колоенне стабилизировалась
                    {
                        // Переходим к следующему этапу - отбору голов.
                        StateMachine = 4;
                        if (BeepStateProcess)
                            my_beep(BEEP_LONG);
                        SecTempPrev = Seconds;
                    } else
                        break;

                } else {
                    TempPrev = DS_TEMP(TEMP_RK20); // Запоминаем температуру
                    SecTempPrev = Seconds; // Запоминаем время последнего изменения температуры
                    break;
                }
            } else {
                // Если время стабилизации колонны меньше нуля, то тогда считаем абсолютное время
                if (Seconds - SecTempPrev > -TimeStabKolonna) //Если с момента начала стабилизации прошло больше заданного количества секунд
                {
                    // Переходим к следующему этапу - отбору голов.
                    StateMachine = 4;
                    if (BeepStateProcess)
                        my_beep(BEEP_LONG);
                    SecTempPrev = Seconds;
                } else
                    break;
            }
        } else
            break;
    case 4: // Отбор голов
        UstPower = PowerRect;
        // Подчиненные контроллеры переводим в ректификацию
        digitalWrite(PIN_SLAVE_0, 1);
        digitalWrite(PIN_SLAVE_1, 1);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        //unsigned int timeChimRectOtbGlv=2000;  // Шим отбора голов (в полупериодах сетевого напряжения в данном случае это около 20 секунд)
        //unsigned char ProcChimOtbGlv=5;  // Процент ШИМ отбора голов (в данном случае это 5 процентов)
        // Клапан отбора ректификата закроем на всякий случай
        KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = 10;
        // Польский буфер закроем.
        KlOpen[KLP_PB] = 0;
        KlClose[KLP_PB] = 10;

        // Устанавливаем клапан отбора хвостов и голов в соответвии с установленнным ШИМ
        //int tEndRectOtbGlv=854;     // Температура окончания отбора голов 85.4 С
        if (ProcChimOtbGlv > 0)
            KlOpen[KLP_GLV_HVS] = (timeChimRectOtbGlv / 100) * ProcChimOtbGlv;
        else
            KlOpen[KLP_GLV_HVS] = -ProcChimOtbGlv;

        KlClose[KLP_GLV_HVS] = timeChimRectOtbGlv - KlOpen[KLP_GLV_HVS];
        tStabSR = DS_TEMP(TEMP_RK20); // Это температура, относительно которой будем стабилизировать отбор

        if (IspReg != 118) { // Обычная ректификация
            if (UrovenProvodimostSR == 0) // Если проводимость спирта не используется, головы отбираем по температуре
            {
                if (DS_TEMP(TEMP_KUB) < tEndRectOtbGlv) { // Пока температура в кубе не выросла выше окончания отбора голов, продолжаем это состояни
                    break;
                }
            } else {

                if (UrovenProvodimostSR > 0) // Если проводимость больше нуля, используем проводимость в том или ином виде
                {
                    V3 = U_GLV;
                    // Если проводимость менее заданного уровня, и проводимость вообще сущетвует, то увеличиваем счетчик срабатываний
                    // или ести проводимость больше уровня тревоги и установлен признак отбирать головы по уровню в приемной емкости.

                    if ((V3 < UrovenProvodimostSR && V3 >= 2 && UrovenProvodimostSR >= 5) || (V3 >= UROVEN_ALARM && UrovenProvodimostSR <= 2)) {
                        my_beep(BEEP_LONG); // Сначала пищим, предупреждая.
                        Count_Provodimost++;
                    } else
                        Count_Provodimost = 0;

                    // Если счетчик срабатываний не превысил максимальное число раз, тогда продолжаем процесс отбора голов.
                    if (Count_Provodimost < MAX_COUNT_PROVODIMOST_SR)
                        break;
                } else // Если уровень проводимости SR меньше нуля, тогда это означает, что отбор голов производится по времени (в десятках минут).
                {
                    // Анализируем, сколько минут прошло с начала отбора голов.
                    SecOstatok = -((Seconds - SecTempPrev) / 600 + UrovenProvodimostSR);
                    if (SecOstatok > 0)
                        break;
                }
            }
        } else {
            // При фракционном отборе отбор продукта идет через клапан СР.
            KlOpen[KLP_SR] = KlOpen[KLP_GLV_HVS];
            KlClose[KLP_SR] = KlClose[KLP_GLV_HVS];

            if (TekFraction == 0) // Текущая фракция при перегонке всегда равна нулю, если вдруг она сменилась, например по наполнению приемной емкости (или вручную ее поменяли), тогда перейдем на отбор СР
            {
                // Фракционная ректификация - там немного по-другому
                TekTemp = TempFractionRect[TekFraction]; //Текущее состояние - первый недробный перегон
                if (TekTemp >= 0) // Значение больше нуля, значит это температура
                {
                    if (DS_TEMP(TEMP_KUB) >= TekTemp) {
                        TekFraction++;
                        SecTempPrev1 = Seconds;
                        if (TekFraction < CountFractionRect)
                            SetAngle(AngleFractionRect[TekFraction]);
                    } else
                        break;
                } else {
                    // Значение меньше нуля, значит это время в минутах
                    SecOstatok = -((Seconds - SecTempPrev) / 60 + TekTemp);
                    // Если времени не осталось, переходим на следующую фракцию
                    if (SecOstatok <= 0) {
                        TekFraction++;
                        SecTempPrev1 = Seconds;

                        if (TekFraction < CountFractionRect)
                            SetAngle(AngleFractionRect[TekFraction]);

                    } else
                        break;
                }
            }
        }
        // Если температура превысила указанный рубеж, то переходим в состояние отбора спирта-ректификата.
        // Клапан отбора голов и хвостов отключаем
        tStabSR = DS_TEMP(TEMP_RK20); // Это температура, относительно которой будем стабилизировать отбор
        KlOpen[KLP_GLV_HVS] = 0;
        KlClose[KLP_GLV_HVS] = 10;
        ProcChimSR = begProcChimOtbSR;
        StateMachine = 5;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        SecTempPrev1 = Seconds; // Запомним время, когда произошел переход с фракции голов на отбор тела
        time1 = 5;
    case 5: // Ожидание, пока температура не вернется к состоянию стабилизации
        // Это для отладки старт-стопа, на самом деле температура 28,5 градуса в этом режиме в колонне быть не может
        //if (tStabSR==0) tStabSR=285;

        // Если колонна слишком долго находится в режиме стопа, то температуру стабилизации примем за новую
        if (time1 == 0 && TimeRestabKolonna != 0) {
            tStabSR = DS_TEMP(TEMP_RK20); // Это температура, относительно которой будем стабилизировать отбор
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            // Если время рестабилизации колонны меньше нуля, тогда при долгом простое переходим на отбор хвостов.
            if (TimeRestabKolonna < 0) {
                if (IspReg != 118) // Если не фракционная перегонка, то все как обычно.
                {
                    // При не фракционной перегонке
                    StateMachine = 7;
                    KlOpen[KLP_SR] = 0; // Отключаем клапана отбора СР
                    KlClose[KLP_SR] = 10;
                    if (BeepStateProcess)
                        my_beep(BEEP_LONG);
                } else {
                    // Переходим на последнюю фракцию, то есть отбор хвостов.
                    SecTempPrev1 = Seconds;
                    TekFraction = CountFractionRect - 1;
                    if (TekFraction < CountFractionRect)
                        SetAngle(AngleFractionRect[TekFraction]);
                    StateMachine = 7;
                }
            }
        }

        KlOpen[KLP_GLV_HVS] = 0;
        KlClose[KLP_GLV_HVS] = 10;
        KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = 10;
        if (DS_TEMP(TEMP_KUB) >= tEndRectOtbSR) {
            StateMachine = 7;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
            break;
        }

        if (DS_TEMP(TEMP_RK20) > tStabSR) {
            break;
        }

        // Phisik: Если мы тут, то температура в колонне вернулась к tStabSR или мы подкрутили tStabSR выше,
        // запоминаем этот момент и идем собирать спирт
        SecTempPrev = Seconds;
        StateMachine = 6;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    case 6: // Отбор СР
        //unsigned int timeChimRectOtbSR=1000;  // Шим отбора СР (в полупериодах сетевого напряжения в данном случае это около 10 секунд)
        //unsigned char tDeltaRect=10;  // Дельта ректификации (в десятых градуса Цельсия) в данном случае - это 10 десятых, то есть 1 градус Цельсия
        //unsigned int tEndRectOtbSR=965;  // Температура окончания отбора спирта-ректификата и начала отбора хвостов (в данном случе 96.5 С)
        // Работа Клапана отбора голов зависит от работы царги пастеризации

        if (ProcChimOtbCP >= 0) {
            if (ProcChimOtbGlv > 0)
                KlOpen[KLP_GLV_HVS] = (timeChimRectOtbGlv / 100) * ProcChimOtbGlv;
            else
                KlOpen[KLP_GLV_HVS] = -ProcChimOtbGlv;

            KlOpen[KLP_GLV_HVS] = ((long)KlOpen[KLP_GLV_HVS] * (long)ProcChimOtbCP) / 100;
            KlClose[KLP_GLV_HVS] = timeChimRectOtbGlv - KlOpen[KLP_GLV_HVS];

        } else {
            KlOpen[KLP_GLV_HVS] = -ProcChimOtbCP;
            KlClose[KLP_GLV_HVS] = timeChimRectOtbSR - KlOpen[KLP_GLV_HVS];
        }

        // Польский буфер закроем.
        KlOpen[KLP_PB] = 0;
        KlClose[KLP_PB] = 10;

        // Устанавливаем ШИМ отбора СР
        UstPower = PowerRect;
        // Подчиненные контроллеры переводим в ректификацию
        digitalWrite(PIN_SLAVE_0, 1);
        digitalWrite(PIN_SLAVE_1, 1);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        ProcChimSR = GetCHIMOtbor(); // Реализуется отбор по-шпоре, что в функции прописано то и будет возвращено.
        if (ProcChimSR > 0)
            KlOpen[KLP_SR] = ((timeChimRectOtbSR / 10) * ProcChimSR) / 10;
        else
            KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = timeChimRectOtbSR - KlOpen[KLP_SR];

#if ADJUST_COLUMN_STAB_TEMP
        // Подхватываем текущую температуру, если мы тут в первый раз
        if (lastStableT < 500) {
            lastStableT = tStabSR;

            // Запоминаем текущее время
            SecTempPrev2 = millis();
        } else if (millis() - SecTempPrev2 > tStabCheckPeriod) {
            // Вычисляем средневзвешенную температуру ...
            lastStableT = (lastStableT + float(DS_TEMP(TEMP_RK20)) / tStabTimeConstant) / tStabAverageDivisor;
            // ... и новую температуру стабилизации
            tStabSR = round(lastStableT);

            // DEBUG_SERIAL.println(String(F("Tstab = ")) + String(0.1*tStabSR, 1) + String(F("lastStableT = ")) + String(0.1*lastStableT, 5));

            // Запоминаем текущее время
            SecTempPrev2 = millis();
        } // if
#endif // ADJUST_COLUMN_STAB_TEMP

        if (DS_TEMP(TEMP_RK20) >= tStabSR + tDeltaRect) // Если текущая температура превысила базовую
        {
            //        //Если температура превысилась менее, чем за одну минуту
            //        if (Seconds-SecTempPrev<60)
            //          {
            // произошел стоп - если значение количества температур в кубе меньше нуля,
            // тогда запомним температуру в кубе за вычетом 0.1 градуса и процент ШИМ, при котором это произошло.
            if (CntCHIM < 0) {
                // Запоминаем температуру, когда произошел стоп за вычетом 0.1 градуса.
                tempK[-CntCHIM] = DS_TEMP(TEMP_KUB) - 1;
                CHIM[-CntCHIM] = ProcChimSR;
                if (-CntCHIM < COUNT_CHIM - 1)
                    CntCHIM--;
            }

            if (DecrementCHIM >= 0)
                ProcChimSR = ProcChimSR - DecrementCHIM; // Тогда уменьшаем  ШИМ указанное число процентов в абсолютном выражении
            else {
                V3 = ((int)ProcChimSR * (int)-DecrementCHIM) / 100;
                // Процентное отношение может быть очень мало, поэтому если получилось нулевое значение, то вычтем единицу.
                if (V3 <= 0)
                    V3 = 1;
                ProcChimSR = ProcChimSR - (char)V3; // Тогда увеличиваем ШИМ на число процентов в относительном выражении
            }

            if (ProcChimSR < minProcChimOtbSR)
                ProcChimSR = minProcChimOtbSR;

            // Если в результате стопа произошло уменьшение отбора до -1 и минимальный тоже -1, тогда переходим на хвосты.
            if ((ProcChimSR == -1 && minProcChimOtbSR == -1) && IspReg != 118) {
                StateMachine = 7;
                if (BeepStateProcess)
                    my_beep(BEEP_LONG);
                KlOpen[KLP_SR] = 0; // Отключаем клапана отбора СР
                KlClose[KLP_SR] = 10;
                break;
            }

            // Если в результате стопа произошло уменьшение отбора до 0 и минимальный тоже 0, тогда переходим на окончание отбора.
            if ((ProcChimSR == 0 && minProcChimOtbSR == 0) && IspReg != 118) {
                StateMachine = 8;
                SecTempPrev = Seconds; // Запомним время, когда закончилась ректификация
                if (BeepStateProcess)
                    my_beep(BEEP_LONG);
                break;
            }

            StateMachine = 5;
            // Взводим таймер для возможной рестабилизации колонны
            if (TimeRestabKolonna < 0)
                time1 = -TimeRestabKolonna;
            else
                time1 = TimeRestabKolonna;

            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else // if (DS_TEMP(TEMP_RK20)>=tStabSR+tDeltaRect)
        {
            // Если температура не выросла более, чем за TimeAutoIncCHIM минут, прибавим ШИМ на 5%
            if (Seconds - SecTempPrev > TimeAutoIncCHIM) {

                // Шим прибавляем только если не дошли до минимального шима, если дошли, то считаем, что хватит играть ШИМом
                if (ProcChimSR > minProcChimOtbSR) {
                    if (IncrementCHIM >= 0)
                        ProcChimSR = ProcChimSR + IncrementCHIM; // Тогда увеличиваем ШИМ указанное число процентов в абсолютном выражении
                    else
                        ProcChimSR = ProcChimSR + ((int)ProcChimSR * (int)-IncrementCHIM) / 100; // Тогда увеличиваем ШИМ на число процентов в относительном выражении

                    // if (ProcChimSR>95) ProcChimSR=95;   // Phisik: на кой клапаном щелкать, если ограничитель стоит?...
                    if (ProcChimSR > 100)
                        ProcChimSR = 100;
                }
                SecTempPrev = Seconds; // Запомним время, когда стабилизировалась температура
            }
        } // if (DS_TEMP(TEMP_RK20)>=tStabSR+tDeltaRect)

        if (IspReg != 118) // Если не фракционная перегонка, то все как обычно.
        {
            // Если температура в кубе превысила температуру при которой надо отбирать СР
            if (DS_TEMP(TEMP_KUB) >= tEndRectOtbSR) {
                StateMachine = 7;
                KlOpen[KLP_SR] = 0; // Отключаем клапана отбора СР
                KlClose[KLP_SR] = 10;
                if (BeepStateProcess)
                    my_beep(BEEP_LONG);
            } else
                break;
        } else {
            if (TekFraction < CountFractionRect - 1) // смотрим на количество фракций, а оно может измениться при сработке датчика уровня.
            {
                TekTemp = TempFractionRect[TekFraction]; //Текущее состояние - первый недробный перегон
                if (TekTemp >= 0) // Значение больше нуля, значит это температура
                {
                    if (DS_TEMP(TEMP_KUB) >= TekTemp) {
                        TekFraction++;
                        SecTempPrev1 = Seconds;
                        if (TekFraction < CountFractionRect)
                            SetAngle(AngleFractionRect[TekFraction]);
                    }
                } else {
                    // Значение меньше нуля, значит это время в минутах
                    SecOstatok = -((Seconds - SecTempPrev1) / 60 + TekTemp);
                    // Если времени не осталось, переходим на следующую фракцию
                    if (SecOstatok <= 0) {
                        TekFraction++;
                        SecTempPrev1 = Seconds;
                        if (TekFraction < CountFractionRect)
                            SetAngle(AngleFractionRect[TekFraction]);
                    }

                } // if (TekTemp >= 0)
                break;
            } else //
            { // Если это последняя фракция, то переводим в режим отбора хвостов.
                SecTempPrev1 = Seconds;
                if (TekFraction < CountFractionRect)
                    SetAngle(AngleFractionRect[TekFraction]);
                StateMachine = 7;
            } // if (TekFraction < CountFractionRect - 1)
        } // if (IspReg != 118)
    case 7: // Отбор Хвостов
        UstPower = PowerRect;
        // Подчиненные контроллеры переводим в ректификацию
        digitalWrite(PIN_SLAVE_0, 1);
        digitalWrite(PIN_SLAVE_1, 1);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        if (IspReg != 118) // Если не фракционный отбор, то все как обычно
        {
            KlOpen[KLP_SR] = 0;
            KlClose[KLP_SR] = 10;
            KlOpen[KLP_GLV_HVS] = (timeChimRectOtbGlv / 100) * 90;
            KlClose[KLP_GLV_HVS] = timeChimRectOtbGlv - KlOpen[KLP_GLV_HVS];

            if (DS_TEMP(TEMP_KUB) >= tEndRect) {
                StateMachine = 8;
                SecTempPrev = Seconds; // Запомним время, когда закончилась ректификация
                if (BeepStateProcess)
                    my_beep(BEEP_LONG);
            } else
                break;
        } else { // Отбор фракционный, все через клапан отбора СР
            KlOpen[KLP_SR] = (timeChimRectOtbGlv / 100) * 90;
            KlClose[KLP_SR] = timeChimRectOtbGlv - KlOpen[KLP_SR];

            if (TekFraction < CountFractionRect) { // Если количество фракций не превышает заданную.
                TekTemp = TempFractionRect[TekFraction]; //Текущее состояние - первый недробный перегон
                if (TekTemp >= 0) // Значение больше нуля, значит это температура
                {
                    if (DS_TEMP(TEMP_KUB) >= TekTemp) {
                        TekFraction++;
                        SecTempPrev1 = Seconds;
                        if (TekFraction < CountFractionRect)
                            SetAngle(AngleFractionRect[TekFraction]);
                    }
                } else {
                    // Значение меньше нуля, значит это время в минутах
                    SecOstatok = -((Seconds - SecTempPrev1) / 60 + TekTemp);
                    // Если времени не осталось, переходим на следующую фракцию
                    if (SecOstatok <= 0) {
                        TekFraction++;
                        SecTempPrev1 = Seconds;
                        if (TekFraction < CountFractionRect)
                            SetAngle(AngleFractionRect[TekFraction]);
                    }
                }
                break;
            } else {
                // Иначе переходим к завершению процесса
                StateMachine = 8;
                SecTempPrev = Seconds; // Запомним время, когда закончилась ректификация
                if (BeepStateProcess)
                    my_beep(BEEP_LONG);
            }
        }
    case 8: // Ждем три минуты для окончания подачи воды
        UstPower = 0;
        // Подчиненные контроллеры отключаем
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);

        KlOpen[KLP_GLV_HVS] = 0; // Отключаем клапана отбора хвостов
        KlClose[KLP_GLV_HVS] = 10;
        KlOpen[KLP_SR] = 0;
        KlClose[KLP_SR] = 10;

        if (Seconds - SecTempPrev > 180 + ((int)(FlAvtonom >> 1)) * 60) {
            // Переводим автомат в конечное состояние
            StateMachine = 100;
            SecondsEnd = Seconds;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }
        break;
    case 9:
        // Ожидание, пока датчик уровня отменяет сработку
        V3 = U_UROVEN;
        if (V3 < UROVEN_ALARM) {
            Count_Provodimost++;
#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("ALARM VOZVRAT U_UROVEN=%i cnt=%i"), U_UROVEN, Count_Provodimost);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif
            // По прошествии минуты устанавливаем флаг сработки.
            if (Count_Provodimost > 2) {
                flAlarmUroven = 0;
                StateMachine = OldStateMachine;
                Count_Provodimost = 0;
            }
        } else {
            Count_Provodimost = 0;
        }
        break;

    case 101: // Превышение температуры в ТСА!!!
    case 102: // Превышение давления
        my_beep(2 * BEEP_LONG);
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата

        // Отключаем на всякий случай все!
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif

        if (StateMachine == 100 && BeepEndProcess)
            my_beep(BEEP_LONG);

        UstPower = 0;
        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        flAllOff = 1;
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        // Отключаем подчиненные контроллеры
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        break;
    }
#ifdef TESTMEM
    DEBUG_SERIAL.println(F("\n[memCheck_Rectif]"));
    DEBUG_SERIAL.println(freeRam());
#endif
}
//    case 104: //Текущее состояние - первый недробный перегон
//    case 106: //Текущее состояние - второй дробный
//    case 107: //Текущее состояние - третий дробный
//    case 105: //Текущее состояние - отбор голов

// Процесс отбора голов (без дефлегматора)
void ProcessSimpleGlv()
{

    if (flAlarmUroven && StateMachine < 4) { // Переводим автомат в стадию завершения
        StateMachine = 4;
        SecondsEnd = 60 + ((int)(FlAvtonom >> 1)) * 60; // Через одну минуту отключим воду
        SecTempPrev = Seconds;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    }

    // Проверяем температуру в ТСА
    if (DS_TEMP(TEMP_TSA) >= MAX_TEMP_TSA) {
        StateMachine = 101; // Переводим автомат в состояние аварии по ТСА.
        UstPower = 0;
    }
    if (flAlarmMPX5010)
        StateMachine = 100; // Переводим в режим тревоги по датчику давления

    switch (StateMachine) {
    case 0: // Процесс не запущен
        break;
    case 1: // Начало процесса
        PrepareProcess();
        //         if (FlToGSM && lastSMSState!=StateMachine) StateToSMS();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif

        KlOpen[KLP_VODA] = PER_KLP_OPEN;
        KlClose[KLP_VODA] = PER_KLP_CLOSE;
        StateMachine = 2;
        //    digitalWrite(PIN_REG_ON,HIGH);
        digitalWrite(PIN_RZG_ON, RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, !ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, RELAY_HIGH);
        digitalWrite(PIN_START_2, RELAY_HIGH);
        flAllOff = 0;

        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    case 2: // Ожидание, пока прогреется термометр в дефлегматоре

        UstPower = Power; // Устанавливаем максимальную мощность для разгона
        // Подчиненные в разгон
        digitalWrite(PIN_SLAVE_0, 1);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        // Если температура в дефлегматоре меньше 70 градусов и датчик дефлегматора в принципе подключен, тогда ждем, пока
        // дефлегматор прогреется, чтобы запустить воду в холодильник (это для некоторой экономии воды)
        if (DS_TEMP(TEMP_DEFL) < TempDeflBegDistil && ds1820_devices > 1 || (TempDeflBegDistil < 0 && DS_TEMP(TEMP_KUB) < -TempDeflBegDistil))
            break;

        // Подаем питание на клапан подачи воды холодильника
        StateMachine = 3;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        // Ждем, пока температура в кубе не достигнет температуры начала дистилляции (если есть датчик дефлегматора, тогда этот пункт сразу
        // проскочится, поскольку темперетура в кубе не может быть меньше, чем температура в дефлегматоре
        //if (DS_TEMP(TEMP_KUB)<TempDeflBegDistil) break;

        // Подаем питание на клапан подачи воды холодильника
        KlOpen[KLP_HLD] = KlOpen[KLP_VODA];
        KlClose[KLP_HLD] = KlClose[KLP_VODA];
        KlState[KLP_HLD] = KlState[KLP_VODA];
        KlCount[KLP_HLD] = KlCount[KLP_VODA];
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
    case 3: // Работа в режиме отбра голов идет исключетельно по наполнению приемной емкости, но все-таки температуру
        // на всякий случай анализируем, ведь температура в кубе при отборе голов, в любом случае не может быть больше, чем температура окончания
        // третьей дробной перегонки

        UstPower = PowerGlvDistil; // Устанавливаем мощность отбора голов.
        // Клапан отбора голов устанавливаем, поскольку по совместительству этот режим может использоваться и для эпюрации
        // Устанавливаем клапан отбора хвостов и голов в соответвии с установленнным ШИМ
        //int tEndRectOtbGlv=854;     // Температура окончания отбора голов 85.4 С
        if (ProcChimOtbGlv > 0)
            KlOpen[KLP_GLV_HVS] = (timeChimRectOtbGlv / 100) * ProcChimOtbGlv;
        else
            KlOpen[KLP_GLV_HVS] = -ProcChimOtbGlv;

        KlClose[KLP_GLV_HVS] = timeChimRectOtbGlv - KlOpen[KLP_GLV_HVS];

        // Подчиненные в режим отбора голов
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 1);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        if (DS_TEMP(TEMP_KUB) >= Temp3P) {
            StateMachine = 4;
            SecondsEnd = 1 * 60 + ((int)(FlAvtonom >> 1)) * 60; // Через две минуты отключим воду
            SecTempPrev = Seconds;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);

        } else
            break;
    case 4: // Ждем минуту для окончания подачи воды
        UstPower = 0;
        // Подчиненные отключаем
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);

        SecondsEnd = SecondsEnd - (Seconds - SecTempPrev);
        SecTempPrev = Seconds;

        if (SecondsEnd <= 0) {
            // Переводим автомат в конечное состояние
            StateMachine = 100;
            SecondsEnd = Seconds;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }
        break;
    case 101: // Превышение температуры в ТСА!!!
        my_beep(3 * BEEP_LONG);
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата
        // Отключаем на всякий случай все!
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        if (BeepEndProcess)
            my_beep(BEEP_LONG);
        UstPower = 0;
        // Подчиненные отключаем
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        flAllOff = 1;

        break;
    }
}

void ProcessSimpleDistill()
{

    if (flAlarmUroven && StateMachine < 4) { // Переводим автомат в стадию завершения
        StateMachine = 4;
        SecondsEnd = 2 * 60 + ((int)(FlAvtonom >> 1)) * 60; // Через две минуты отключим воду
        SecTempPrev = Seconds;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
    }

    // Проверяем температуру в ТСА
    if (DS_TEMP(TEMP_TSA) >= MAX_TEMP_TSA) {
        StateMachine = 101; // Переводим автомат в состояние аварии по ТСА.
        UstPower = 0;
    }

    if (flAlarmMPX5010)
        StateMachine = 100; // Переводим в режим тревоги по датчику давления

    // Если первая недробная дистилляция и процесс запущен, тогда включаем мешалку.
    if (IspReg == 104 && StateMachine > 0 && StateMachine <= 4) {
        // Работаем в следующем режиме - 2 минуты пауза, 1 минута работает миксер.
        if (time3 == 0) {
            time3 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
            timeMIXER = (int)tempP[0] * 60;
        }
    }

    switch (StateMachine) {
    case 0: // Процесс не запущен
        break;
    case 1: // Начало процесса
        PrepareProcess();
        //          if (FlToGSM && lastSMSState!=StateMachine) StateToSMS();
#if USE_GSM_WIFI == 1
        lastSMSState = 0;
#endif

        KlOpen[KLP_VODA] = PER_KLP_OPEN;
        KlClose[KLP_VODA] = PER_KLP_CLOSE;
        StateMachine = 2;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        //    digitalWrite(PIN_REG_ON,HIGH);
        digitalWrite(PIN_RZG_ON, RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, !ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, RELAY_HIGH);
        digitalWrite(PIN_START_2, RELAY_HIGH);
        flAllOff = 0;

    case 2: // Ожидание, пока прогреется термометр в дефлегматоре

        UstPower = Power; // Устанавливаем максимальную мощность для разгона
        // Подчиненные переводим в разгон
        digitalWrite(PIN_SLAVE_0, 1);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);

        // Если температура в дефлегматоре меньше 70 градусов и датчик дефлегматора в принципе подключен, тогда ждем, пока
        // дефлегматор прогреется, чтобы запустить воду в холодильник (это для некоторой экономии воды)
        if (DS_TEMP(TEMP_DEFL) < TempDeflBegDistil && ds1820_devices > 1 || (TempDeflBegDistil < 0 && DS_TEMP(TEMP_KUB) < -TempDeflBegDistil))
            break;

        // Подаем питание на клапан подачи воды холодильника
        // для этого синхронизируем его с клапаном общей подачи воды (возможно, при этом будет меньше гидроударов, хотя я их и так не наблюдаю)
        KlOpen[KLP_HLD] = KlOpen[KLP_VODA];
        KlClose[KLP_HLD] = KlClose[KLP_VODA];
        KlState[KLP_HLD] = KlState[KLP_VODA];
        KlCount[KLP_HLD] = KlCount[KLP_VODA];
        StateMachine = 3;
        if (BeepStateProcess)
            my_beep(BEEP_LONG);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);

    case 3: // Ждем, пока температура в кубе не превысит установленную
        UstPower = PowerDistil; // Устанавливаем максимальную мощность для разгона
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 1); // Переводим подчиненный в режим дистилляции
        digitalWrite(PIN_SLAVE_3, 0);

        if (IspReg == 104)
            TekTemp = Temp1P; //Текущее состояние - первый недробный перегон
        if (IspReg == 106)
            TekTemp = Temp2P; //Текущее состояние - второй дробный
        if (IspReg == 107)
            TekTemp = Temp3P; //Текущее состояние - третий дробный

        if (DS_TEMP(TEMP_KUB) >= TekTemp) {
            StateMachine = 4;
            SecondsEnd = 2 * 60 + ((int)(FlAvtonom >> 1)) * 60; // Через две минуты отключим воду
            SecTempPrev = Seconds;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        } else
            break;
    case 4: // Ждем минуту для окончания подачи воды
        UstPower = 0;
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);

        // Отключаем подчиненные
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);
        SecondsEnd = SecondsEnd - (Seconds - SecTempPrev);
        SecTempPrev = Seconds;

        if (SecondsEnd <= 0) {
            // Переводим автомат в конечное состояние
            StateMachine = 100;
            SecondsEnd = Seconds;
            if (BeepStateProcess)
                my_beep(BEEP_LONG);
        }
        break;
    case 101: // Превышение температуры в ТСА!!!
        my_beep(3 * BEEP_LONG);
    default: // Phisik@24.02.2020: Любое ошибочное значение машины, должно приводить к остановке
    case 100: // Конечное состояние автомата
        // Отключаем на всякий случай все!
#if USE_GSM_WIFI == 1
        if (FlToGSM && lastSMSState != StateMachine)
            StateToSMS();
#endif
        if (BeepEndProcess)
            my_beep(BEEP_LONG);
        UstPower = 0;
        // Отключаем подчиненные
        digitalWrite(PIN_SLAVE_0, 0);
        digitalWrite(PIN_SLAVE_1, 0);
        digitalWrite(PIN_SLAVE_2, 0);
        digitalWrite(PIN_SLAVE_3, 0);
        CloseAllKLP();
        //    digitalWrite(PIN_REG_ON,LOW);
        digitalWrite(PIN_RZG_ON, !RELAY_HIGH);
        digitalWrite(PIN_ALL_OFF, ALL_OFF_HIGH);
        digitalWrite(PIN_START_1, !RELAY_HIGH);
        digitalWrite(PIN_START_2, !RELAY_HIGH);
        flAllOff = 1;

        break;
    }
}
