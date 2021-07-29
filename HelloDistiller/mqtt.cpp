// Последнее обновление 2018-08-09 by Phisik
// Phisik: Этот файл был заметно переписан.
// 1. Добавлена асинхронная работа с UART (как для ввода, так и для вывода)
// 2. Ручное копирование из PROGMEM заменено на strncmp_P()
// 3. Добавлены некоторые поля
//
// В результате изменений стабильная скорость обмена между ESP & MEGA выросла
// минимум до 500000 бод, дальше не тестировал.
//
// При такой высокой скорости, можно отправлять больше данных каждые 2 секунды
//

#include "configuration.h"
#include "declarations.h"

#define MQTT_DEBUG 0

#ifndef MQTT_SERIAL_MODE
#define MQTT_SERIAL_MODE SERIAL_8N1 // попытка как-то поменять режим работы COM порта
#endif // !MQTT_SERIAL_MODE

#if USE_MQTT_BROKER

unsigned char CurrentStage = 0; // текущий этап процесса
unsigned long TimeStage = 0; // Время работы на текущем этапе
unsigned int CntStop = 0; // Счетчик СТОПОВ

unsigned long SecPrevBuf = 0; // время (Seconds) предыдущего обновления данных дисплея
unsigned long SecPrevOper = 0; // время (Seconds) предыдущего обновления оперативных данных
unsigned long SecCmdDisp = 0; // время (Seconds) прихода команды, приводящей к изменению информации на дисплее

bool FlagRefreshOnline = true; // признак необходимости обновления данных в online режиме
bool NeedRefresh = false; // признак необходимости обновления показаний дисплея
bool extGenerator = false; // Генератор для проверки целостности линии связи автоматика-scada

char lcd_mqtt_buf1[LCD_BUFFER_SIZE]; // первая строка экрана для отправки по MQTT
char lcd_mqtt_buf2[LCD_BUFFER_SIZE]; // вторая строка экрана для отправки по MQTT

#ifndef MQTT_BUFFER_SIZE
#define MQTT_BUFFER_SIZE 50
#endif // !MQTT_BUFFER_SIZE

// ########################################################
// # Описание форматов для передачи на ESP
// ########################################################

// оперативные данные
PROGMEM const char fmt_lcd1[] = "lcd1=%s"; // первая строка дисплея
PROGMEM const char fmt_lcd2[] = "lcd2=%s"; // вторая строка дисплея
PROGMEM const char fmt_klpHLD[] = "klpHLD=%d"; // клапан холодильника (для дистилляции)
PROGMEM const char fmt_klpDEFL[] = "klpDEFL=%d"; // клапан для подачи воды в дефлегматор
PROGMEM const char fmt_klpGLV_HVS[] = "klpGLV_HVS=%d"; // клапан отбора головных и хвостовых фракций
PROGMEM const char fmt_klpSR[] = "klpSR=%d"; // клапан отбора ректификата
PROGMEM const char fmt_stage[] = "stage=%d"; // этапы текущего процесса
PROGMEM const char fmt_t_kub[] = "t_kub=%d"; // температура в кубе
PROGMEM const char fmt_t_col[] = "t_col=%d"; // температура в колонне
PROGMEM const char fmt_t_tsa[] = "t_tsa=%d"; // температура в ТСА
PROGMEM const char fmt_t_pvk[] = "t_pvk=%d"; // температура в рубашке ПВК
PROGMEM const char fmt_work_time[] = "work_time=%02u:%02u:%02u"; // время текущего процесса
PROGMEM const char fmt_ProcShimSR[] = "ProcShimSR=%d"; // % ШИМ отбора тела
PROGMEM const char fmt_tStabSR[] = "tStabSR=%d"; // температура стабилизации колонны
PROGMEM const char fmt_CntStop[] = "CntStop=%u"; // количество СТОПОВ
PROGMEM const char fmt_TimeStage[] = "TimeStage=%02u:%02u:%02u"; // время текущего этапа
PROGMEM const char fmt_UU_MPX5010[] = "UU_MPX5010=%d"; // Текущее значение давления
PROGMEM const char fmt_extGenerator[] = "extGenerator=%d"; // Генератор для проверки целостности линии связи автоматика-scada

PROGMEM const char fmt_razgonTEH[] = "razgonTEH=%d"; // Флаг работы разгонного ТЕНа

// limon: 2018-07-17
PROGMEM const char fmt_SpdNBK[] = "SpdNBK=%d"; //30 Скорость насоса НБК
PROGMEM const char fmt_StateMachine[] = "StatMachine=%d"; //31 State Machine
PROGMEM const char fmt_CntMinute[] = "CntMinute=%u"; //32 Время работы автоматики, минут
PROGMEM const char fmt_online[] = "online=%u"; // надо дать знать ESP в каком режиме мы находимся

// Phisik: выбор режима
PROGMEM const char fmt_IspReg[] = "IspReg=%d"; // Текущий режим
PROGMEM const char fmt_FactPower[] = "FactPower=%d"; // Фактическая мощность
PROGMEM const char fmt_MaxVoltsOut[] = "MaxVoltsOut=%d"; // Напряжение в сети

// настройки
PROGMEM const char fmt_BeepKeyPress[] = "BeepKeyPress=%u"; // звук кнопок
PROGMEM const char fmt_BeepEndProc[] = "BeepEndProc=%u"; // звук окончания процесса
PROGMEM const char fmt_BeepStateProc[] = "BeepStateProc=%u"; // звук смены этапа
PROGMEM const char fmt_FlToUSART[] = "FlToUSART=%u"; // вывод в UART
PROGMEM const char fmt_tEndRectRazgon[] = "tEndRectRazgon=%d"; // температура окончания разгона
PROGMEM const char fmt_tEndRectOtbGlv[] = "tEndRectOtbGlv=%d"; // температура окончания отбора голов
PROGMEM const char fmt_tEndRectOtbSR[] = "tEndRectOtbSR=%d"; // температура окончания отбора тела
PROGMEM const char fmt_tEndRect[] = "tEndRect=%d"; // температура окончания ректификации
PROGMEM const char fmt_ShimGlv[] = "ShimGlv=%d"; // ШИМ отбора голов
PROGMEM const char fmt_ProcShimGlv[] = "ProcShimGlv=%d"; // % ШИМ отбора голов
PROGMEM const char fmt_ProcChimOtbCP[] = "ProcChimOtbCP=%d"; // % ШИМ отбора голов через царгу пастеризации
PROGMEM const char fmt_ShimSR[] = "ShimSR=%u"; // Период ШИМ отбора тела
PROGMEM const char fmt_MinProcShimSR[] = "MinProcShimSR=%u"; // минимальный % ШИМ отбора тела
PROGMEM const char fmt_BegProcShimSR[] = "BegProcShimSR=%u"; // начальный % ШИМ отбора тела
PROGMEM const char fmt_timeStabKolonna[] = "timeStabKolonna=%d"; // Время стабилизации колонны TimeStabKolonna
PROGMEM const char fmt_tDeltaRect[] = "tDeltaRect=%d"; // дельта ректификации
PROGMEM const char fmt_ProvodSR[] = "ProvodSR=%d"; // настройка окончания отбора голов
PROGMEM const char fmt_PowRect[] = "PowRect=%u"; // Мощность ректификации
PROGMEM const char fmt_PowerTEH[] = "Power=%u"; // Мощность тена
//PROGMEM const char fmt_PowGlvDistil[]   =  "PowGlvDistil=%u";            // Мощность отбора голов при дистилляции
PROGMEM const char fmt_PowDistil[] = "PowDistil=%u"; // Мощность дистилляции
PROGMEM const char fmt_Tem1P[] = "Tem1P=%d"; // температура окончания дистилляции 1
//PROGMEM const char fmt_Tem2P[]          =  "Tem2P=%d";                   // температура окончания дистилляции 2
//PROGMEM const char fmt_Tem3P[]          =  "Tem3P=%d";                   // температура окончания дистилляции 3
PROGMEM const char fmt_TDeflBegDistil[] = "TDeflBegDistil=%d"; // температура окончания разгона
PROGMEM const char fmt_UPeregrev[] = "UPeregrev=%u"; // какое U надо поддерживать для защиты клапанов от перегрева.
PROGMEM const char fmt_AvtonomHLD[] = "AvtonomHLD=%u"; // Признак того, что используется автономная система охлаждения
PROGMEM const char fmt_iCorrectASC712[] = "iCorrectASC712=%u"; // Признак того, как надо использовать датчик asc712
PROGMEM const char fmt_PAlarmMPX5010[] = "PAlarmMPX5010=%d"; // Давление при котором надо выдавать сингал тревоги
PROGMEM const char fmt_ds1820popr0[] = "ds1820popr0=%d"; // Поправка для датчика температуры 0
PROGMEM const char fmt_ds1820popr1[] = "ds1820popr1=%d"; // Поправка для датчика температуры 1
PROGMEM const char fmt_ds1820popr2[] = "ds1820popr2=%d"; // Поправка для датчика температуры 2
PROGMEM const char fmt_ds1820popr3[] = "ds1820popr3=%d"; // Поправка для датчика температуры 3
PROGMEM const char fmt_PP_MPX5010[] = "PP_MPX5010=%d"; // Поправка для датчика давления
//PROGMEM const char fmt_PU_MPX5010[]     =  "PU_MPX5010=%03d/%03u";     // Поправка датчика давления и текушее давление
PROGMEM const char fmt_DDopInfo[] = "DDopInfo=%u"; // Флаг отображения аварийной информации

PROGMEM const char fmt_CCHIM[] = "CCHIM=%d"; // Количество точек при отборе по Т куба
PROGMEM const char fmt_tempKCHIM[] = "tempKCHIM%d=%d"; // Температура в точке 1 при отборе по Т куба
PROGMEM const char fmt_CHIMTK[] = "CHIMTK%d=%d"; // % Шим в точке 1 при отборе по Т куба

PROGMEM const char fmt_CntPause[] = "CntPause=%d"; // Количество заторных пауз
PROGMEM const char fmt_MixerDuty[] = "MixerDuty=%d"; // Количество минут работы мешалки
PROGMEM const char fmt_MixerPause[] = "MixerPause=%d"; // Количество минут простоя мешалки
PROGMEM const char fmt_BEERtempP[] = "BEERtempP%d=%d"; // Температуры заторных пауз
PROGMEM const char fmt_BEERtimeP[] = "BEERtimeP%d=%u"; // Время паузы

PROGMEM const char fmt_IncChim[] = "IncChim=%d"; // Автоматическое увеличение % ШИМ отбора
PROGMEM const char fmt_DecChim[] = "DecChim=%d"; // Автоматическое уменьшение % ШИМ отбора

// limon: 2018-07-17
PROGMEM const char fmt_UstPowReg[] = "UstPowReg=%u"; // Мощность регулятора мощности
PROGMEM const char fmt_PowVarkaZerno[] = "PowVarkaZerno=%u"; // Мощность варка зерно для термостата
PROGMEM const char fmt_TmpTerm[] = "TmpTerm=%d"; // Задание t на термостат (уставка)
PROGMEM const char fmt_TmstDelta[] = "TmstDelta=%d"; // Дельта t на термостат
PROGMEM const char fmt_PID_Temp0[] = "PID_Temp0=%d"; // PID пропорциональный коэффициент
PROGMEM const char fmt_PID_Temp1[] = "PID_Temp1=%d"; // PID интегральный коэффициент
PROGMEM const char fmt_PID_Temp2[] = "PID_Temp2=%d"; // PID дифференциальный коэффициент
PROGMEM const char fmt_PowNBK[] = "PowNBK=%u"; // Мощность в режиме нбк
PROGMEM const char fmt_minPrNBK[] = "minPrNBK=%d"; // min давление нбк
PROGMEM const char fmt_timePrNBK[] = "timePrNBK=%d"; // Период корректировки скорости подати
PROGMEM const char fmt_NasosNBK[] = "NasosNBK=%u"; // Управление насосом нвк
PROGMEM const char fmt_dPrNBK[] = "dPrNBK=%d"; // дельта давления нбк

// Phisik: 2020-03-10
PROGMEM const char fmt_TempZSP[] = "TempZSP=%d"; // Температура при которой можно засыпать несоложенку.
PROGMEM const char fmt_TempZSPSld[] = "TempZSPSld=%d"; // Температура при которой можно засыпать солод.
PROGMEM const char fmt_TempKipenZator[] = "TempKipenZator=%d"; // Температура начала закипания затора (либо это температура в верхней части заторника (если два термометра, либо в заторнике, если стоит один термометр)
PROGMEM const char fmt_TempHLDZatorBrog1[] = "TempHLDZatorBrog1=%d"; // Температура брожения
PROGMEM const char fmt_PowerRazvZerno[] = "PowerRazvZerno=%d"; // Мощность, при которой будут развариваться зерновые (2 часа).

PROGMEM const char fmt_dsnums[] = "dsnums=%d,%d,%d,%d,%d"; // порядок сортировки датчиков

// ########################################################
// # Название изменяемых значений
// ########################################################
PROGMEM const char val_BeepKeyPress[] = "BeepKeyPress"; // звук кнопок
PROGMEM const char val_BeepEndProc[] = "BeepEndProc"; // звук окончания процесса
PROGMEM const char val_BeepStateProc[] = "BeepStateProc"; // звук смены этапа
PROGMEM const char val_FlToUSART[] = "FlToUSART"; // вывод в UART
PROGMEM const char val_tEndRectRazgon[] = "tEndRectRazgon"; // температура окончания разгона
PROGMEM const char val_tEndRectOtbGlv[] = "tEndRectOtbGlv"; // температура окончания отбора голов
PROGMEM const char val_tEndRectOtbSR[] = "tEndRectOtbSR"; // температура окончания отбора тела
PROGMEM const char val_tEndRect[] = "tEndRect"; // температура окончания ректификации
PROGMEM const char val_ShimGlv[] = "ShimGlv"; // ШИМ отбора голов
PROGMEM const char val_ProcShimGlv[] = "ProcShimGlv"; // % ШИМ отбора голов
PROGMEM const char val_ProcChimOtbCP[] = "ProcChimOtbCP"; // % ШИМ отбора голов
PROGMEM const char val_ShimSR[] = "ShimSR"; // ШИМ отбора тела
PROGMEM const char val_ProcShimSR[] = "ProcShimSR"; // Текущий % ШИМ отбора тела
PROGMEM const char val_MinProcShimSR[] = "MinProcShimSR"; // минимальный % ШИМ отбора тела
PROGMEM const char val_BegProcShimSR[] = "BegProcShimSR"; // начальный % ШИМ отбора тела
PROGMEM const char val_timeStabKolonna[] = "timeStabKolonna"; // Время стабилизации колонны TimeStabKolonna
PROGMEM const char val_tDeltaRect[] = "tDeltaRect"; // дельта ректификации
PROGMEM const char val_ProvodSR[] = "ProvodSR"; // настройка окончания отбора голов
PROGMEM const char val_online[] = "online"; // настройка обновления дисплея
PROGMEM const char val_save[] = "save"; // сохранение в EEPROM
PROGMEM const char val_refresh[] = "refresh"; // считывание значений настроек
PROGMEM const char val_keystrokes[] = "keystrokes"; // нажатие кнопки
PROGMEM const char val_commands[] = "cmd"; // команды контроллеру (аварийная остановка, переключение этапов и т.п.)
PROGMEM const char val_PowRect[] = "PowRect"; // Мощность ректификации
PROGMEM const char val_PowerTEH[] = "Power"; // Мощность ТЭНа
//PROGMEM const char val_PowGlvDistil[]   =  "PowGlvDistil";            // Мощность отбора голов при дистилляции
PROGMEM const char val_PowDistil[] = "PowDistil"; // Мощность дистилляции
PROGMEM const char val_Tem1P[] = "Tem1P"; // температура дистилляции 1
//PROGMEM const char val_Tem2P[]          =  "Tem2P";                   // температура дистилляции 2
//PROGMEM const char val_Tem3P[]          =  "Tem3P";                   // температура дистилляции 3
PROGMEM const char val_TDeflBegDistil[] = "TDeflBegDistil"; // температура окончания разгона
PROGMEM const char val_UPeregrev[] = "UPeregrev"; // какое U надо поддерживать для защиты клапанов от перегрева.
PROGMEM const char val_AvtonomHLD[] = "AvtonomHLD"; // Признак того, что используется автономная система охлаждения
PROGMEM const char val_iCorrectASC712[] = "iCorrectASC712"; // Признак того, как надо использовать датчик asc712
PROGMEM const char val_PAlarmMPX5010[] = "PAlarmMPX5010"; // Давление при котором надо выдавать сингал тревоги.
PROGMEM const char val_ds1820popr0[] = "ds1820popr0"; // Поправка для датчика температуры 0
PROGMEM const char val_ds1820popr1[] = "ds1820popr1"; // Поправка для датчика температуры 1
PROGMEM const char val_ds1820popr2[] = "ds1820popr2"; // Поправка для датчика температуры 2
PROGMEM const char val_ds1820popr3[] = "ds1820popr3"; // Поправка для датчика температуры 3
PROGMEM const char val_PP_MPX5010[] = "PP_MPX5010"; // Поправка для датчика давления
PROGMEM const char val_DDopInfo[] = "DDopInfo"; // Флаг отображения аварийной информации

PROGMEM const char val_IncChim[] = "IncChim"; // Автоматическое увеличение % ШИМ отбора
PROGMEM const char val_DecChim[] = "DecChim"; // Автоматическое уменьшение % ШИМ отбора

// limon: 2018-07-17
PROGMEM const char val_UstPowReg[] = "UstPowReg"; // Мощность регулятора мощности
PROGMEM const char val_PowVarkaZerno[] = "PowVarkaZerno"; // Мощность варка зерно для термостата
PROGMEM const char val_TmpTerm[] = "TmpTerm"; // Задание t на термостат (уставка)
PROGMEM const char val_TmstDelta[] = "TmstDelta"; // Дельта t на термостат
PROGMEM const char val_PID_Temp0[] = "PID_Temp0"; // PID пропорциональный коэффициент
PROGMEM const char val_PID_Temp1[] = "PID_Temp1"; // PID интегральный коэффициент
PROGMEM const char val_PID_Temp2[] = "PID_Temp2"; // PID дифференциальный коэффициент
PROGMEM const char val_SpdNBK[] = "SpdNBK"; // Скорость насоса НВК
PROGMEM const char val_PowNBK[] = "PowNBK"; // Мощность в режиме нбк
PROGMEM const char val_minPrNBK[] = "minPrNBK"; // min давление нбк
PROGMEM const char val_timePrNBK[] = "timePrNBK"; // Период корректировки скорости подати
PROGMEM const char val_NasosNBK[] = "NasosNBK"; // Управление насосом нвк
PROGMEM const char val_dPrNBK[] = "dPrNBK"; // дельта давления нбк

// Phisik: 2017-07-17
PROGMEM const char val_IspReg[] = "IspReg"; // Текущий режим
PROGMEM const char val_tStabSR[] = "tStabSR"; // температура стабилизации колонны

// Phisik: 2020-03-03
PROGMEM const char val_StateMachine[] = "StateMachine"; // температура стабилизации колонны

void initMqtt()
{
    // инициализация UART для обмена с модулем ESP
    MQTT_SERIAL.begin(MQTT_SERIAL_BAUDRATE, MQTT_SERIAL_MODE);
}

void mqttSerialPrint(char* s)
{
    // Ждем освобождения буфера отправки
    //while (MQTT_SERIAL.availableForWrite() < strlen(s) + 1)
    //	yield();

#if MQTT_DEBUG
    DEBUG_SERIAL.print("[MQTT][MEGA] Sent: ");
    DEBUG_SERIAL.println(s);
#endif // MQTT_DEBUG

    MQTT_SERIAL.println(s);
}

// Отправка данных на ESP через UART
// Phisik: отправка данных теперь полностью асинхронная, мы не задерживаемся в этой функции
//         отправили, и освобождаем очередь. На следующем loop-e отправим следующую порцию
//         Это позволяет отправлять много информации, не блокирую выполнение остальных функций

#define MQTT_STATE_SETTINGS 50
static short nMqttStateMachine = 0;

bool mqttSendStatus()
{
    static boolean bSendAllData = false;
    char buf[MQTT_BUFFER_SIZE + 1] = { 0 };

    // этот блок требуется для возможности расчета времени работы на текущем этапе и количества СТОПОВ
    // если произошла смена этапа
    if (CurrentStage != StateMachine) {
        // сохраним новый этап и время перехода на новый этап
        CurrentStage = StateMachine;
        TimeStage = Seconds;
        //  если перешли в СТОП - увеличим счетчик стопов
        if (StateMachine == 5)
            CntStop++;
    }

    switch (nMqttStateMachine) {
    case 0: // idle
        if (Seconds % 30 == 0 && Seconds != SecPrevOper) {
            // если наступило время отправки и еще не отправляли в эту секунду
            bSendAllData = true;
            nMqttStateMachine = 1;

            // сохраним время отправки
            SecPrevOper = Seconds;
        } else if (
            (Seconds % 2 == 0 && Seconds != SecPrevBuf && (FlagRefreshOnline || millis() < 60000L)) || (NeedRefresh && millis() - SecCmdDisp > 300)) {
            // если наступило время отправки и еще не отправляли в эту секунду и разрешено обновление в online режиме
            // ИЛИ требуется обновление по нажатию кнопки и дисплей успел обновиться (задержка 0.5 сек)
            // Phisik: первые 60 секунд мы отправляем постоянно, вне зависимости от флага FlagRefreshOnline
            //         это надо чтобы esp получило начальные данные после загрузки и подключения к WiFi
            bSendAllData = false;
            nMqttStateMachine = 1;

            // сохраним время отправки
            SecPrevBuf = Seconds;
            // сбросим признак обновления
            NeedRefresh = false;
        } else
            break;

    // Отправляем текущие данные
    case 1: // Вывод LCD экрана
        mqttSerialPrint(lcd_mqtt_buf1);
        mqttSerialPrint(lcd_mqtt_buf2);
        break;
    case 2: // Текущий режим
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_IspReg, IspReg);
        break;
    case 3: // State Machine
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_StateMachine, StateMachine);
        break;
    case 4: // Флаг отображения аварийной информации
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_DDopInfo, DispDopInfo);
        break;
    case 5: // температура в кубе
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_t_kub, DS_TEMP(TEMP_KUB));
        break;
    case 6: // температура в колонне
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_t_col, DS_TEMP(TEMP_RK20));
        break;
    case 7: // температура в TSA
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_t_tsa, DS_TEMP(TEMP_TSA));
        break;
    case 8: // время с момента включения системы/начала процесса
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_work_time, hour, minute, second);
        break;
    case 9: // Скорость насоса НБК
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_SpdNBK, SpeedNBKDst);
        break;
    case 10: // Фактическая мощность
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_FactPower, FactPower);
        break;
    case 11: // Напряжение в сети
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_MaxVoltsOut, (uint16_t)MaxVoltsOut);
        break;
    case 12: // состояние клапана отбора голов
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_klpGLV_HVS, KlState[KLP_GLV_HVS]);
        break;
    case 13: // состояние клапана отбора спирта
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_klpSR, KlState[KLP_SR]);
        break;
    case 14: // состояние клапана холодильника
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_klpHLD, KlState[KLP_HLD]);
        break;
    case 15: // состояние клапана для подачи воды в дефлегматор
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_klpDEFL, KlState[KLP_DEFL]);
        break;

    case 16: // температура в ПВК
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_t_pvk, DS_TEMP(TEMP_DEFL));
        break;

    case 17: // Флаг активности разгонного ТЭна
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_razgonTEH, digitalRead(PIN_RZG_ON) == RELAY_HIGH);
        // DEBUG_SERIAL.println(buf);
        break;
    case 18:
        // Если не надо все данные отправлять, то переходим в idle state
        if (!bSendAllData)
            nMqttStateMachine = 0;
        break;

    case 19 ... 28: // Phisik: this is GCC C++ extension, not included in C++ standard. Don't do like this ;)
        nMqttStateMachine = 29;

        // Ниже break отключен, чтобы не терять инетацию, т.к. case-ы выполняются последовательно
        // break;
    case 29:
        // Phisik: Показываю как пользоваться пользовательскими параметрами
        // esp будет прокидывать на сервер 10 параметров: userValue0 - userValue9
        // Используем один из них и конфигурируем webui.json под свои параметры
        // У меня тут будет расчет крепости спирта в кубе и парах, при обычной дистиляции
        if (DS_TEMP(TEMP_KUB) < 790) {
            snprintf_P(buf, MQTT_BUFFER_SIZE, PSTR("userValue0=0"));
        } else {
            // Аппроксимация крепости бинарного раствора вода - спирт полиномом x^6, R = 0.9998
            // Выдран из какого справочника, креля или ему подобных...
            const uint8_t NPOLY = 4;
            long double liquidStrength[NPOLY] = {
                11214.09529,
                -346.4862143,
                3.581060166,
                -0.012376322
            };

            // Approximation of gas strength from boiling T
            // Polynomial fit x^6, R = 0.9998
            long double gasStrength[NPOLY] = {
                9732.37736218615000000000,
                -335.57619886484000000000,
                3.90936812841569000000,
                -0.01526887873952520000
            };

            long double liquid = liquidStrength[0], gas = gasStrength[0];
            const double tmp = 0.1 * DS_TEMP(TEMP_KUB);
            long double xn = tmp;
            for (int i = 1; i < NPOLY; i++, xn *= tmp) {
                liquid += liquidStrength[i] * xn;
                gas += gasStrength[i] * xn;
            }

            snprintf_P(buf, MQTT_BUFFER_SIZE, PSTR("userValue0=%d.%d%% (%d.%d%%)"), int(liquid), int(liquid * 10) % 10, int(gas), int(gas * 10) % 10);
        }
        break;
    case 30: // этап процесса
        // Phisik: это дубляж fmt_StateMachine, оставил только для совместимости, надо убрать
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_stage,
            ((StateMachine > -1 && StateMachine < 9) || (StateMachine > 99 && StateMachine < 103)) ? StateMachine : 404);
        break;
    case 31: // время с момента включения системы/начала процесса
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_work_time, hour, minute, second);
        break;
    case 32: // текущий % отбора спирта
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ProcShimSR, ProcChimSR);
        break;
    case 33: // температура стабилизации колонны
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_tStabSR, tStabSR);
        break;
    case 34: // количество СТОПОВ
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_CntStop, CntStop);
        break;
    case 35: // время текущего этапа
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_TimeStage, (int)((Seconds - TimeStage) / 3600), (int)((Seconds - TimeStage) % 3600) / 60, (int)((Seconds - TimeStage) % 3600) % 60);
        break;
    case 36: // Текущее значение давления
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_UU_MPX5010, U_MPX5010);
        break;
    case 37: // Генератор для проверки целостности линии связи автоматика-scada
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_extGenerator, extGenerator = !extGenerator);
        break;
    case 38: // Время работы автоматики, минут
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_CntMinute, hour * 60 + minute);
        break;
    case 39: // Флаг частого обновления
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_online, FlagRefreshOnline);
        break;
    // Можно тут добавить другие параметры, но конструкцию "хх ... MQTT_STATE_SETTINGS" надо скопировать
    case 40 ...(MQTT_STATE_SETTINGS - 1):
        nMqttStateMachine = 0;
        break;

    // Далее передаются настройки контроллера при запросе со стороны ESP
    case MQTT_STATE_SETTINGS + 0: // звук при нажатии клавиш
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_BeepKeyPress, BeepKeyPress);
        break;
    case MQTT_STATE_SETTINGS + 1: // звук при окончании процесса
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_BeepEndProc, BeepEndProcess);
        break;
    case MQTT_STATE_SETTINGS + 2: // звук при переходе между этапами процесса
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_BeepStateProc, BeepStateProcess);
        break;
    case MQTT_STATE_SETTINGS + 3: // флаг вывода в USART
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_FlToUSART, FlToUSART);
        break;
    case MQTT_STATE_SETTINGS + 4: // температура окончания режима разгона
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_tEndRectRazgon, tEndRectRazgon);
        break;
    case MQTT_STATE_SETTINGS + 5: // температура окончания отбора голов
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_tEndRectOtbGlv, tEndRectOtbGlv);
        break;
    case MQTT_STATE_SETTINGS + 6: // температура окончания отбора спирта
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_tEndRectOtbSR, tEndRectOtbSR);
        break;
    case MQTT_STATE_SETTINGS + 7: // температура окончания ректификации
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_tEndRect, tEndRect);
        break;
    case MQTT_STATE_SETTINGS + 8: // дельта ректификации
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_tDeltaRect, tDeltaRect);
        break;
    case MQTT_STATE_SETTINGS + 9: // ШИМ отбора голов
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ShimGlv, timeChimRectOtbGlv / 100);
        break;
    case MQTT_STATE_SETTINGS + 10: // % ШИМ отбора голов
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ProcShimGlv, ProcChimOtbGlv);
        break;
    case MQTT_STATE_SETTINGS + 11: // ШИМ отбора спирта
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ShimSR, timeChimRectOtbSR / 100);
        break;
    case MQTT_STATE_SETTINGS + 12: // Время стабилизации колонны TimeStabKolonna
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_timeStabKolonna, TimeStabKolonna);
        break;
    case MQTT_STATE_SETTINGS + 13: // минимальный % отбора спирта
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_MinProcShimSR, minProcChimOtbSR);
        break;
    case MQTT_STATE_SETTINGS + 14: // начальный % отбора спирта
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_BegProcShimSR, begProcChimOtbSR);
        break;
    case MQTT_STATE_SETTINGS + 15: // режим отбора голов
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ProvodSR, UrovenProvodimostSR);
        break;
    case MQTT_STATE_SETTINGS + 16: // Мощность ректификации
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PowRect, PowerRect);
        break;
    case MQTT_STATE_SETTINGS + 17: // мощность установленного ТЭНа
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PowerTEH, Power);
        break;
    case MQTT_STATE_SETTINGS + 18: // настройка мощности дистилляции
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PowDistil, PowerDistil);
        break;
    case MQTT_STATE_SETTINGS + 19: // температура окончания дистилляции 1
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_Tem1P, Temp1P);
        break;
    case MQTT_STATE_SETTINGS + 20: // температура окончания режима разгона
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_TDeflBegDistil, TempDeflBegDistil);
        break;
    case MQTT_STATE_SETTINGS + 21: // какое U надо поддерживать для защиты клапанов от перегрева.
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_UPeregrev, NaprPeregrev);
        break;
    case MQTT_STATE_SETTINGS + 22: // Признак того, что используется автономная система охлаждения
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_AvtonomHLD, FlAvtonom);
        break;
    case MQTT_STATE_SETTINGS + 23: // Признак того, как надо использовать датчик asc712
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_iCorrectASC712, CorrectASC712);
        break;
    case MQTT_STATE_SETTINGS + 24: // Давление при котором надо выдавать сингал тревоги.
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PAlarmMPX5010, AlarmMPX5010);
        break;
    case MQTT_STATE_SETTINGS + 25: // Поправка для датчика давления
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PP_MPX5010, P_MPX5010 / 10);
        break;
    case MQTT_STATE_SETTINGS + 26: // Поправка для датчика температуры 0
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ds1820popr0, ds1820_popr[0]);
        break;
    case MQTT_STATE_SETTINGS + 27: // Поправка для датчика температуры 1
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ds1820popr1, ds1820_popr[1]);
        break;
    case MQTT_STATE_SETTINGS + 28: // Поправка для датчика температуры 2
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ds1820popr2, ds1820_popr[2]);
        break;
    case MQTT_STATE_SETTINGS + 29: // Поправка для датчика температуры 3
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ds1820popr3, ds1820_popr[3]);
        break;
    case MQTT_STATE_SETTINGS + 30: // Колличество точек при отборе по Т куба
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_CCHIM, CntCHIM);
        break;
    case MQTT_STATE_SETTINGS + 31:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_TempZSP, TempZSP);
        break;
    case MQTT_STATE_SETTINGS + 32:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_TempZSPSld, TempZSPSld);
        break;
    case MQTT_STATE_SETTINGS + 33:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_TempKipenZator, TempKipenZator);
        break;
    case MQTT_STATE_SETTINGS + 34:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_TempHLDZatorBrog1, TempHLDZatorBrog1);
        break;
    case MQTT_STATE_SETTINGS + 35:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PowerRazvZerno, PowerRazvZerno);
        break;
#if ENABLE_SENSOR_SORTING
    case MQTT_STATE_SETTINGS + 36:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_dsnums, ds1820_nums[0], ds1820_nums[1], ds1820_nums[2], ds1820_nums[3], ds1820_nums[4]);
        break;
        // Phisik 2020-03-07: Тут были настройки отбора по таблице, но они переехали вниз
    case MQTT_STATE_SETTINGS + 37 ... MQTT_STATE_SETTINGS + 40:
        nMqttStateMachine = MQTT_STATE_SETTINGS + 41;
        // break;
#else
    // Phisik 2020-03-07: Тут были настройки отбора по таблице, но они переехали вниз
    case MQTT_STATE_SETTINGS + 36 ... MQTT_STATE_SETTINGS + 40:
        nMqttStateMachine = MQTT_STATE_SETTINGS + 41;
        // break;
#endif

    case MQTT_STATE_SETTINGS + 41: // Автоматическое увеличение % ШИМ отбора
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_IncChim, IncrementCHIM);
        break;
    case MQTT_STATE_SETTINGS + 42: // Автоматическое уменьшение % ШИМ отбора
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_DecChim, DecrementCHIM);
        break;
    case MQTT_STATE_SETTINGS + 43: // Мощность регулятора мощности
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_UstPowReg, UstPowerReg);
        break;
    case MQTT_STATE_SETTINGS + 44: // Мощность варка зерно для термостата
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PowVarkaZerno, PowerVarkaZerno);
        break;
    case MQTT_STATE_SETTINGS + 45: // Задание t на термостат (уставка)
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_TmpTerm, TempTerm);
        break;
    case MQTT_STATE_SETTINGS + 46: // Дельта t на термостат
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_TmstDelta, Delta);
        break;
    case MQTT_STATE_SETTINGS + 47: // PID пропорциональный коэффициент
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PID_Temp0, PIDTemp[0]);
        break;
    case MQTT_STATE_SETTINGS + 48: // PID интегральный коэффициент
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PID_Temp1, PIDTemp[1]);
        break;
    case MQTT_STATE_SETTINGS + 49: // PID дифференциальный коэффициент
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PID_Temp2, PIDTemp[2]);
        break;
    case MQTT_STATE_SETTINGS + 50: // Мощность в режиме нбк
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_PowNBK, PowerNBK);
        break;
    case MQTT_STATE_SETTINGS + 51: // min давление нбк
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_minPrNBK, minPressNBK * 5);
        break;
    case MQTT_STATE_SETTINGS + 52: // Период корректировки скорости подачи
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_timePrNBK, timePressNBK * 5);
        break;
    case MQTT_STATE_SETTINGS + 53: // Управление насосом нвк
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_NasosNBK, UprNasosNBK);
        break;
    case MQTT_STATE_SETTINGS + 54: // дельта давления нбк
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_dPrNBK, deltaPressNBK);
        break;
    case MQTT_STATE_SETTINGS + 55: // % ШИМ отбора голов через царгу пастеризации
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_ProcChimOtbCP, ProcChimOtbCP);
        break;

    case MQTT_STATE_SETTINGS + 56 ... MQTT_STATE_SETTINGS + 65:
        // Температуры отбора по таблице
        {
            const int index = nMqttStateMachine - MQTT_STATE_SETTINGS - 56;
            snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_tempKCHIM, index, tempK[index]);
            // Serial.println(buf);
        }
        break;
    case MQTT_STATE_SETTINGS + 66 ... MQTT_STATE_SETTINGS + 75:
        // ШИМ отбора по таблице
        {
            const int index = nMqttStateMachine - MQTT_STATE_SETTINGS - 66;
            snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_CHIMTK, index, CHIM[index]);
            // Serial.println(buf);
        }
        break;

    // Leave some space here for the future
    case MQTT_STATE_SETTINGS + 76 ... MQTT_STATE_SETTINGS + 80:
        nMqttStateMachine = MQTT_STATE_SETTINGS + 81;

    case MQTT_STATE_SETTINGS + 81:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_CntPause, CntPause);
        break;

    case MQTT_STATE_SETTINGS + 82 ... MQTT_STATE_SETTINGS + 91:
        // Температуры отбора по таблице
        {
            const int index = nMqttStateMachine - MQTT_STATE_SETTINGS - 82;
            snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_BEERtempP, index, tempP[index + 1]);
            // Serial.println(buf);
        }
        break;

    case MQTT_STATE_SETTINGS + 92 ... MQTT_STATE_SETTINGS + 101:
        // ШИМ отбора по таблице
        {
            const int index = nMqttStateMachine - MQTT_STATE_SETTINGS - 92;
            snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_BEERtimeP, index, timeP[index + 1]);
            //Serial.println(buf);
        }
        break;

    case MQTT_STATE_SETTINGS + 102:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_MixerDuty, tempP[0]);
        break;
    case MQTT_STATE_SETTINGS + 103:
        snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_MixerPause, timeP[0]);
        break;

    default:
        nMqttStateMachine = 0;
    }

    // Отправляем данные в порт
    if (nMqttStateMachine > 1)
        mqttSerialPrint(buf);

    // Идем отправлять следующий пункт, если в режиме отправки
    if (nMqttStateMachine > 0)
        nMqttStateMachine++;
}

// Обработка полученных от ESP данных
void processRecievedData(const char* pub_topic, const char* val)
{
    long ret;
    //unsigned int u_ret;

    // звук кнопок
    if (strncmp_P(pub_topic, val_BeepKeyPress, MQTT_BUFFER_SIZE) == 0)
        BeepKeyPress = val[0] == '1';

    // звук окончания процесса
    else if (strncmp_P(pub_topic, val_BeepEndProc, MQTT_BUFFER_SIZE) == 0)
        BeepEndProcess = val[0] == '1';

    // звук смены этапа
    else if (strncmp_P(pub_topic, val_BeepStateProc, MQTT_BUFFER_SIZE) == 0)
        BeepStateProcess = val[0] == '1';

    // вывод в UART
    else if (strncmp_P(pub_topic, val_FlToUSART, MQTT_BUFFER_SIZE) == 0)
        FlToUSART = val[0] == '1';

    // нажатие кнопки дисплея
    else if (strncmp_P(pub_topic, val_keystrokes, MQTT_BUFFER_SIZE) == 0) { // изменено для уменьшения количества переменных
        cmdGPRS = val[0];
        // сохранить время получения команды
        SecCmdDisp = millis();
        // установить признак необходимости обновления показаний дисплея
        NeedRefresh = true;
    }

    // команды контроллеру
    else if (strncmp_P(pub_topic, val_commands, MQTT_BUFFER_SIZE) == 0) { // изменено для уменьшения количества переменных
        switch (val[0]) {
        case 'P': // previous stage
            DEBUG_SERIAL.println("MQTT command received: decreasing state machine");
            StateMachine--; // NB! Тут нет никаких проверок, что опасно!
            break;
        case 'N': // next stage
            DEBUG_SERIAL.println("MQTT command received: increasing state machine");
            StateMachine++; // NB! Тут нет никаких проверок, что опасно!
            break;
        case 'H': // halt surrent prosess
            DEBUG_SERIAL.println("MQTT command received: halting current process");
            UstPower = 0;
            StateMachine = 100; // Конечное состояние автомата
            break;
        case 'R': // reset controller immediatly
        {
            void (*resetFunc)(void) = 0; // Прошивка начинается с нулевого адреса ...
            resetFunc(); // ... начинаем выполнение программы с самого начала
        } break;
        default:
            break;
        }
    }

    // Переключатель этапов. В отличии от того, что сверху, все проверки проходят на сайте.
    else if (strncmp_P(pub_topic, val_StateMachine, MQTT_BUFFER_SIZE) == 0) {
        StateMachine = atoi(val);
    }

    // настройка обновления дисплея
    else if (strncmp_P(pub_topic, val_online, MQTT_BUFFER_SIZE) == 0) {
        FlagRefreshOnline = val[0] == '1';
    }

    // сохранение в EEPROM
    else if (strncmp_P(pub_topic, val_save, MQTT_BUFFER_SIZE) == 0)
        writeEEPROM();

    // считывание значений настроек
    else if (strncmp_P(pub_topic, val_refresh, MQTT_BUFFER_SIZE) == 0)
        nMqttStateMachine = MQTT_STATE_SETTINGS;

    // температура окончания разгона
    else if (strncmp_P(pub_topic, val_tEndRectRazgon, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret != 0 && abs(ret) < 1250)
            tEndRectRazgon = ret;
    }

    // температура окончания отбора голов
    else if (strncmp_P(pub_topic, val_tEndRectOtbGlv, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret < 1250)
            tEndRectOtbGlv = ret;
    }

    // температура окончания отбора тела
    else if (strncmp_P(pub_topic, val_tEndRectOtbSR, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret < 1250)
            tEndRectOtbSR = ret;
    }

    // температура окончания ректификации
    else if (strncmp_P(pub_topic, val_tEndRect, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret < 1250)
            tEndRect = ret;
    }

    // ШИМ отбора голов
    else if (strncmp_P(pub_topic, val_ShimGlv, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val) * 100;
        if (ret > 0)
            timeChimRectOtbGlv = ret;
    }

    // % ШИМ отбора голов
    else if (strncmp_P(pub_topic, val_ProcShimGlv, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret != 0 && ret <= 100)
            ProcChimOtbGlv = ret;
    }

    // % ШИМ отбора голов через ЦП
    else if (strncmp_P(pub_topic, val_ProcChimOtbCP, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret <= 100)
            ProcChimOtbCP = ret;
    }

    // ШИМ отбора тела
    else if (strncmp_P(pub_topic, val_ShimSR, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val) * 100;
        if (ret >= 0)
            timeChimRectOtbSR = ret;
    }
    // текущий % ШИМ отбора тела
    else if (strncmp_P(pub_topic, val_ProcShimSR, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret >= 0 && abs(ret) <= 100)
            ProcChimSR = ret;
    }

    // минимальный % ШИМ отбора тела
    else if (strncmp_P(pub_topic, val_MinProcShimSR, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret >= 0)
            minProcChimOtbSR = ret;
    }

    // начальный % ШИМ отбора тела
    else if (strncmp_P(pub_topic, val_BegProcShimSR, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && abs(ret) <= 100)
            begProcChimOtbSR = ret;
    }

    // Время стабилизации колонны TimeStabKolonna
    else if (strncmp_P(pub_topic, val_timeStabKolonna, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        TimeStabKolonna = ret;
    }

    // дельта ректификации
    else if (strncmp_P(pub_topic, val_tDeltaRect, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0)
            tDeltaRect = ret;
    }

    // настройка окончания отбора голов
    else if (strncmp_P(pub_topic, val_ProvodSR, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret != 0)
            UrovenProvodimostSR = ret;
    }
    // настройка мощности ректификации
    else if (strncmp_P(pub_topic, val_PowRect, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret <= Power)
            PowerRect = ret;
    }
    // мощность установленного ТЭНа
    else if (strncmp_P(pub_topic, val_PowerTEH, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0)
            Power = ret;
    }

    // настройка мощности дистилляции
    else if (strncmp_P(pub_topic, val_PowDistil, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret <= Power)
            PowerDistil = ret;
    }
    // температура окончания дистилляции 1
    else if (strncmp_P(pub_topic, val_Tem1P, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret < 1250)
            Temp1P = ret;
    }

    // температура окончания разгона
    else if (strncmp_P(pub_topic, val_TDeflBegDistil, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret != 0 && abs(ret) < 1250)
            TempDeflBegDistil = ret;
    }
    // какое U надо поддерживать для защиты клапанов от перегрева.
    else if (strncmp_P(pub_topic, val_UPeregrev, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret <= 300)
            NaprPeregrev = ret;
    }
    // Признак того, что используется автономная система охлаждения
    else if (strncmp_P(pub_topic, val_AvtonomHLD, MQTT_BUFFER_SIZE) == 0) {
        ret = val[0] == '1';
        FlAvtonom = ret;
    }
    // Признак того, как надо использовать датчик asc712
    else if (strncmp_P(pub_topic, val_iCorrectASC712, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret >= 0 && ret < 3)
            ;
        CorrectASC712 = ret;
    }
    // Давление при котором надо выдавать сингал тревоги.
    else if (strncmp_P(pub_topic, val_PAlarmMPX5010, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret >= 0)
            ;
        AlarmMPX5010 = ret;
    }

    // Поправка для датчика давления
    else if (strncmp_P(pub_topic, val_PP_MPX5010, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);

        P_MPX5010 = ret;
    }
    // Поправка для датчика температуры 0
    else if (strncmp_P(pub_topic, val_ds1820popr0, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);

        ds1820_popr[0] = ret;
    }
    // Поправка для датчика температуры 1
    else if (strncmp_P(pub_topic, val_ds1820popr1, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);

        ds1820_popr[1] = ret;
    }
    // Поправка для датчика температуры 2
    else if (strncmp_P(pub_topic, val_ds1820popr2, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);

        ds1820_popr[2] = ret;
    }
    // Поправка для датчика температуры 3
    else if (strncmp_P(pub_topic, val_ds1820popr3, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);

        ds1820_popr[3] = ret;
    }
    // Количество точек при отборе по Т куба
    else if (strncmp_P(pub_topic, fmt_CCHIM, 5) == 0) {
        ret = atol(val);
        if (ret >= -COUNT_CHIM && ret <= COUNT_CHIM)
            ;
        CntCHIM = ret;
    }

    // Температура в точках 1-10 при отборе по Т куба
    else if (strncmp_P(pub_topic, fmt_tempKCHIM, 9) == 0) {
        const int i = pub_topic[9] - '0';
        if (i >= 0 || i < 10) {
            ret = atol(val);
            if (ret >= 0 && ret < 1250)
                tempK[i] = ret;
        }
    }
    // % Шим в точке 1-10 при отборе по Т куба
    else if (strncmp_P(pub_topic, fmt_CHIMTK, 6) == 0) {
        const int i = pub_topic[6] - '0';
        if (i >= 0 || i < 10) {
            ret = atoi(val);
            if (ret > -2)
                CHIM[i] = ret;
        }
    }

    // Пивная мешалка
    else if (strncmp_P(pub_topic, fmt_MixerDuty, 9) == 0) {
        ret = atol(val);
        if (ret >= 0)
            tempP[0] = ret;
    } else if (strncmp_P(pub_topic, fmt_MixerPause, 9) == 0) {
        ret = atol(val);
        if (ret >= 0)
            timeP[0] = ret;
    }
    // Пивные паузы

    else if (strncmp_P(pub_topic, fmt_CntPause, 8) == 0) {
        ret = atol(val);
        if (ret >= 0)
            CntPause = ret;
    }

    else if (strncmp_P(pub_topic, fmt_BEERtempP, 9) == 0) {
        const int i = pub_topic[9] - '0';
        if (i >= 0 || i < 10) {
            ret = atol(val);
            if (ret >= 0 && ret < 1250)
                tempP[i + 1] = ret; // +1 здесь, поскольку tempP[0] - это мешалка
        }
    } else if (strncmp_P(pub_topic, fmt_BEERtimeP, 9) == 0) {
        const int i = pub_topic[9] - '0';
        if (i >= 0 || i < 10) {
            ret = atol(val);
            if (ret >= 0 && ret < 1250)
                timeP[i + 1] = ret; // +1 здесь, поскольку tempP[0] - это мешалка
        }
    }

    // Автоматическое увеличение % ШИМ отбора
    else if (strncmp_P(pub_topic, val_IncChim, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);

        IncrementCHIM = ret;
    }
    // Автоматическое уменьшение % ШИМ отбора
    else if (strncmp_P(pub_topic, val_DecChim, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);

        DecrementCHIM = ret;
    }

    // limon: 2018-07-17

    //44 Мощность регулятора мощности
    else if (strncmp_P(pub_topic, val_UstPowReg, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0)
            UstPowerReg = ret;
    }
    //45 Мощность варка зерно для термостата
    else if (strncmp_P(pub_topic, val_PowVarkaZerno, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0)
            PowerVarkaZerno = ret;
    }
    //46 Задание t на термостат (уставка)
    else if (strncmp_P(pub_topic, val_TmpTerm, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret <= 1250)
            TempTerm = ret;
    }
    //47 Дельта t на термостат
    else if (strncmp_P(pub_topic, val_TmstDelta, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0)
            Delta = ret;
    }
    //48 PID пропорциональный коэффициент
    else if (strncmp_P(pub_topic, val_PID_Temp0, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        PIDTemp[0] = ret;
    }
    //49 PID интегральный коэффициент
    else if (strncmp_P(pub_topic, val_PID_Temp1, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        PIDTemp[1] = ret;
    }
    //50 PID дифференциальный коэффициент
    else if (strncmp_P(pub_topic, val_PID_Temp2, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        PIDTemp[2] = ret;
    }
    //51 Скорость насоса НБК
    else if (strncmp_P(pub_topic, val_SpdNBK, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret >= 0 && ret <= 256)
            SpeedNBKDst = ret;
    }
    //52 Мощность в режиме нбк
    else if (strncmp_P(pub_topic, val_PowNBK, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0)
            PowerNBK = ret;
    }
    //53 min давление нбк
    else if (strncmp_P(pub_topic, val_minPrNBK, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        minPressNBK = ret / 5;
    }
    //54 Период корректировки скорости подачи
    else if (strncmp_P(pub_topic, val_timePrNBK, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0)
            timePressNBK = ret / 5;
    }
    //55 Управление насосом нвк
    else if (strncmp_P(pub_topic, val_NasosNBK, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        UprNasosNBK = ret;
    }
    //56 дельта давления нбк
    else if (strncmp_P(pub_topic, val_dPrNBK, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        deltaPressNBK = ret;
    }

    // Phisik: limon предложил сделать переключатель режимов, вполне удобно получилось
    //57 Текущий режим
    else if (strncmp_P(pub_topic, val_IspReg, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        switch (ret) {
        case 101: // Мониторинг
        case 102: // Термостат
        case 103: // Регулятор мощности
        case 104: // Первый (недробный) отбор
        case 105: // Отбор голов
        case 106: // Второй дробный отбор
        case 107: // Третий дробный отбор
        case 108: // Затор зерно
        case 109: // Ректификация
        case 110: // Дистилляция с дефлегматором
        case 111: // НДРФ
        case 112: // НБК
        case 113: // Разваривание мучно-солодового затора (без варки).
        case 114: // Разваривание с чиллером и миксером
        case 115: // Таймер + регулятор мощности
        case 116: // Пивоварня - клон браумастера
        case 117: // Фракционная перегонка
        case 118: // Ректификация Фракционная
        case 129: // Тест клапанов // Не отключается!
            // case 130:  Внешнее управление
            IspReg = ret;

            DEBUG_SERIAL.print("MQTT command received: change process. New process is ");
            DEBUG_SERIAL.println(IspReg);

            PrepareProcess();

            // Внимание! Процесс запустится автоматически!
            StateMachine = 1;

            // Сохраним состояние в еепром
            writeEEPROM();
            break;
        default:
            // wrong number received, just skip it
            break;
        }
    }

    // температура стабилизации колонны
    else if (strncmp_P(pub_topic, val_tStabSR, MQTT_BUFFER_SIZE) == 0) {
        ret = atol(val);
        if (ret > 0 && ret <= 1000)
            tStabSR = ret;
#if ADJUST_COLUMN_STAB_TEMP
        lastStableT = 0; // Обнуляем нашу температуру, чтобы в Case 6 подхватило текущую
        SecTempPrev2 = 0; // Обнуляем счетчик времени
#endif // ADJUST_COLUMN_STAB_TEMP
    }

    //case 12: // состояние клапана отбора голов
    //	snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_klpGLV_HVS, KlState[KLP_GLV_HVS]);
    //	break;
    //case 13: // состояние клапана отбора спирта
    //	snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_klpSR, KlState[KLP_SR]);
    //	break;
    //case 14: // состояние клапана холодильника
    //	snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_klpHLD, KlState[KLP_HLD]);
    //	break;
    //case 15: // состояние клапана для подачи воды в дефлегматор
    //	snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_klpDEFL, KlState[KLP_DEFL]);
    //	break;

    // управление клапанами
    else if (strncmp_P(pub_topic, PSTR("klpGLV_HVS"), MQTT_BUFFER_SIZE) == 0) {
        if (val[0] == '1') {
            KlOpen[KLP_GLV_HVS] = 1000;
            KlClose[KLP_GLV_HVS] = 0;
            KlReg[KLP_GLV_HVS] = PEREGREV_ON;
        } else {
            KlOpen[KLP_GLV_HVS] = 0;
            KlClose[KLP_GLV_HVS] = 1000;
            KlReg[KLP_GLV_HVS] = 0;
        }
        DEBUG_SERIAL.print("MQTT command received: changing KLP_GLV_HVS state to ");
        DEBUG_SERIAL.println(val);
    } else if (strncmp_P(pub_topic, PSTR("klpSR"), MQTT_BUFFER_SIZE) == 0) {
        if (val[0] == '1') {
            KlOpen[KLP_SR] = 1000;
            KlClose[KLP_SR] = 0;
            KlReg[KLP_SR] = PEREGREV_ON;
        } else {
            KlOpen[KLP_SR] = 0;
            KlClose[KLP_SR] = 1000;
            KlReg[KLP_SR] = 0;
        }
        DEBUG_SERIAL.print("MQTT command received: changing KLP_SR state to ");
        DEBUG_SERIAL.println(val);
    } else if (strncmp_P(pub_topic, PSTR("klpHLD"), MQTT_BUFFER_SIZE) == 0) {
        if (val[0] == '1') {
            KlOpen[KLP_HLD] = 1000;
            KlClose[KLP_HLD] = 0;
            KlReg[KLP_HLD] = PEREGREV_ON;
        } else {
            KlOpen[KLP_HLD] = 0;
            KlClose[KLP_HLD] = 1000;
            KlReg[KLP_HLD] = 0;
        }
        DEBUG_SERIAL.print("MQTT command received: changing KLP_HLD state to ");
        DEBUG_SERIAL.println(val);
    } else if (strncmp_P(pub_topic, PSTR("klpDEFL"), MQTT_BUFFER_SIZE) == 0) {
        if (val[0] == '1') {
            KlOpen[KLP_DEFL] = 1000;
            KlClose[KLP_DEFL] = 0;
            KlReg[KLP_DEFL] = PEREGREV_ON;
        } else {
            KlOpen[KLP_DEFL] = 0;
            KlClose[KLP_DEFL] = 1000;
            KlReg[KLP_DEFL] = 0;
        }
        DEBUG_SERIAL.print("MQTT command received: changing KLP_DEFL state to ");
        DEBUG_SERIAL.println(val);
    } else if (strncmp_P(pub_topic, fmt_TempZSP, 7) == 0) {
        ret = atol(val);
        TempZSP = ret;
    } else if (strncmp_P(pub_topic, fmt_TempZSPSld, 10) == 0) {
        ret = atol(val);
        TempZSPSld = ret;
    } else if (strncmp_P(pub_topic, fmt_TempKipenZator, 14) == 0) {
        ret = atol(val);
        TempKipenZator = ret;
    } else if (strncmp_P(pub_topic, fmt_TempHLDZatorBrog1, 17) == 0) {
        ret = atol(val);
        TempHLDZatorBrog1 = ret;
    } else if (strncmp_P(pub_topic, fmt_PowerRazvZerno, 14) == 0) {
        ret = atol(val);
        PowerRazvZerno = ret;
    }
#if ENABLE_SENSOR_SORTING
    // snprintf_P(buf, MQTT_BUFFER_SIZE, fmt_razgonTEH, digitalRead(PIN_RZG_ON) == RELAY_HIGH);
    else if (strncmp_P(pub_topic, fmt_dsnums, 6) == 0) {
        int n[5];
        int nval = sscanf(val, "%d,%d,%d,%d,%d", n, n + 1, n + 2, n + 3, n + 4);
        if (nval == 5)
            for (int i = 0; i < 5 && i < MAX_DS1820; i++)
                if (n[i] > -1 && n[i] < 5)
                    ds1820_nums[i] = n[i];
    }
#endif
} // ProcessRecievedData()

const short maxTopicNameLength = 20;
int currentIndex = 0;
char uartBuffer[MQTT_BUFFER_SIZE] = { 0 };

// Получение данных от ESP
void handleMqttSerial()
{
#if MQTT_SERIAL_PING_CHECK
    // Phisik: добавим пинг для UART, чтобы можно было перезапустить/переподключить esp и не дергать сам контроллер
    const int pingPeriod = 14444; // выберем число некратное 2 и 30 сек
    static uint32_t lastUartSendPingTime = millis();
    static uint32_t lastUartRcvPingTime = millis();
#endif

    char* value;

    // Phisik: Не будем блокировать исполнение остального кода
    // Прочитаем сколько есть, остальное на следующей итерации
    int n = MQTT_SERIAL.available();

    // Если буфер пустой, оправляем данные на ESP
    if (n < 1)
        mqttSendStatus();

    // В противном случае смотрим, что нам пришло от ESP
    while (n-- > 0) {
        switch (uartBuffer[currentIndex] = MQTT_SERIAL.read()) {

        case '\r':
            // skip all unwanted symbols
            break;
        case '\n':
            // replace new line with zero symbol to provide correct work of strchr() function
            uartBuffer[currentIndex] = 0;

#if MQTT_SERIAL_PING_CHECK
            // answer to ping from ESP
            if (strncmp_P(uartBuffer, PSTR("ping"), MQTT_BUFFER_SIZE) == 0) {
#if MQTT_DEBUG
                DEBUG_SERIAL.println("[INFO][MEGA] Ping was received. Sending pong...");
#endif // MQTT_DEBUG
                lastUartRcvPingTime = millis();
                MQTT_SERIAL.println("pong");
                currentIndex = 0;
                break;
            }
#endif
            if (value = strchr(uartBuffer, '=')) {
                // don't copy name & value to new buffer, just replace '=' with zero symbol, increment pointer and use both parts as usual strings
                const char* topic = uartBuffer;
                *value++ = 0;

#if MQTT_DEBUG
                DEBUG_SERIAL.print("[MQTT][MEGA] Rcvd: ");
                DEBUG_SERIAL.print(topic);
                DEBUG_SERIAL.print(F("="));
                DEBUG_SERIAL.println(value);
#endif

                // check for empty strings
                if (topic[0] > 0 && value[0] > 0) {
                    processRecievedData(topic, value);
                } else {
#if MQTT_DEBUG
                    DEBUG_SERIAL.println(F("[MQTT][MEGA] No valid topic was found in received message"));
#endif
                }
            } else {
#if MQTT_DEBUG
                DEBUG_SERIAL.println(F("[MQTT][MEGA] No value was found in received message"));
#endif
            }
            currentIndex = 0;
            break;
        default:
            if (++currentIndex > MQTT_BUFFER_SIZE - 2) {
                currentIndex = 0;
            }
        }
    }

#if MQTT_SERIAL_PING_CHECK
    // Phisik: send ping to esp
    if (millis() - lastUartSendPingTime > pingPeriod) {
#if MQTT_DEBUG
        DEBUG_SERIAL.println(F("[INFO][MEGA] Starting ping-pong UART test. Sending ping..."));
#endif // MQTT_DEBUG
        lastUartSendPingTime = millis();
        MQTT_SERIAL.println("ping");
    }

    // Phisik: reinitialize UART if there was no ping in 2 minutes
    if (millis() - lastUartRcvPingTime > 123456L) {
        lastUartRcvPingTime = millis();
        MQTT_SERIAL.begin(MQTT_SERIAL_BAUDRATE, MQTT_SERIAL_MODE);
        DEBUG_SERIAL.println(F("[WARN][MEGA] MQTT UART seems to lost connection. Reinitializing UART..."));
    }
#endif
}

#endif // #
//#################################################################################################################