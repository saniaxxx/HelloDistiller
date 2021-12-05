// Последнее обновление 2017-05-31 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам
// Тут мы работаем с EEPROM

#include "configuration.h"
#include "declarations.h"

unsigned int ee_addr; // Переменная для хранения текущего адреса чтения/записи в EEPROM
unsigned char eeReadChar()
{
    unsigned char lByte = EEPROM.read(ee_addr);
    ee_addr++;
    return lByte;
}

void eeWriteChar(char p_value)
{
    if (EEPROM.read(ee_addr) != p_value)
        EEPROM.write(ee_addr, p_value);
    ee_addr++;
}

void eeWriteInt(int p_value)
{
    unsigned char lByte = (unsigned char)(p_value);
    unsigned char hByte = (unsigned char)(p_value >> 8);

    // Для экономии ресурса флеш памяти, сначала проверяем ее содержимое и запись производим только если значение отличается.
    eeWriteChar(lByte);
    eeWriteChar(hByte);
}

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int eeReadInt()
{
    unsigned char lByte = eeReadChar();
    unsigned char hByte = eeReadChar();
    return (unsigned int)(lByte) | (unsigned int)(hByte << 8);
}

void writeEEPROM()
{
    int i;
    flAutoDetectPowerTEN = 0;
    ee_addr = 0;
    eeWriteChar(PR_REWRITE_EEPROM);
    eeWriteChar(IspReg);
    eeWriteInt(Power);
    eeWriteInt(UstPowerReg);
    eeWriteInt(TempTerm);
    eeWriteInt(Delta);
    eeWriteChar(FlToUSART);
    eeWriteChar(FlToGSM);
    eeWriteInt(Temp1P);
    eeWriteInt(Temp2P);
    eeWriteInt(Temp3P);
    for (i = 0; i < MAX_DS1820; i++)
        eeWriteChar(ds1820_popr[i]);

    eeWriteInt(tEndRectRazgon);
    eeWriteInt(PowerRect);

    eeWriteInt(tEndRectOtbGlv);
    eeWriteInt(timeChimRectOtbGlv);
    eeWriteInt(ProcChimOtbGlv);
    eeWriteInt(timeChimRectOtbSR);
    eeWriteChar(tDeltaRect);
    eeWriteInt(tEndRectOtbSR);
    eeWriteInt(tEndRect);
    eeWriteInt(PowerGlvDistil);
    eeWriteInt(PowerDistil);

    eeWriteInt(TempDeflBegDistil);
    eeWriteInt(TempDefl);
    eeWriteChar(DeltaDefl);

    eeWriteChar(BeepEndProcess);
    eeWriteChar(BeepStateProcess);
    eeWriteChar(BeepKeyPress);

    eeWriteInt(PowerRazvZerno);

    eeWriteInt(tEndDistDefl);
    eeWriteChar(minProcChimOtbSR);
    eeWriteInt(PowerVarkaZerno);
    eeWriteInt(TempKipenZator);
    eeWriteChar(TempZSPSld);
    eeWriteChar(TempZSP);
    eeWriteInt(PowerNBK);
    eeWriteChar(SpeedNBKDst);
    eeWriteInt(PowerMinute);
    eeWriteChar(begProcChimOtbSR);
    eeWriteInt(P_MPX5010);
    eeWriteChar(PeriodRefrServer);
    eeWriteInt(NaprPeregrev);
    eeWriteInt(UrovenBarda);
    eeWriteInt(UrovenProvodimostSR);
    eeWriteInt(TimeStabKolonna);
    eeWriteChar(DecrementCHIM);
    eeWriteChar(IncrementCHIM);
    eeWriteInt(TimeAutoIncCHIM);
    eeWriteChar(CntCHIM);
    for (i = 0; i < COUNT_CHIM; i++) {
        eeWriteInt(tempK[i]);
        eeWriteChar(CHIM[i]);
    }
    eeWriteInt(TimeRestabKolonna);
    eeWriteChar(CntPause);
    for (i = 0; i < 10; i++) {
        eeWriteChar(tempP[i]);
        eeWriteChar(timeP[i]);
    }
    eeWriteChar(CorrectASC712);
    eeWriteChar(ip[0]);
    eeWriteChar(ip[1]);
    eeWriteChar(ip[2]);
    eeWriteChar(ip[3]);
    eeWriteInt(ipPort);
    for (i = 0; i < 10; i++)
        eeWriteChar(idDevice[i]);
    for (i = 0; i < 12; i++)
        eeWriteChar(my_phone[i]);
    eeWriteInt(AlarmMPX5010);
    eeWriteChar(FlAvtonom);
    eeWriteChar(timeOpenBRD);
    eeWriteChar(PIDTemp[0]);
    eeWriteChar(PIDTemp[1]);
    eeWriteChar(PIDTemp[2]);
    for (i = 0; i < 11; i++)
        eeWriteChar(WiFiAP[i]);
    for (i = 0; i < 11; i++)
        eeWriteChar(WiFiPass[i]);

    eeWriteChar(CountFractionDist);
    for (i = 0; i < MAX_CNT_FRACTION_DIST; i++) {
        eeWriteInt(TempFractionDist[i]);
        eeWriteInt(PowerFractionDist[i]);
        eeWriteChar(AngleFractionDist[i]);
    }

    eeWriteChar(CountFractionRect);
    for (i = 0; i < MAX_CNT_FRACTION_RECT; i++) {
        eeWriteInt(TempFractionRect[i]);
        eeWriteChar(AngleFractionRect[i]);
    }

    eeWriteChar(TempHLDZatorBrog1);
    eeWriteInt(PowerPhase[0]);
    eeWriteInt(PowerPhase[1]);
    eeWriteInt(PowerPhase[2]);
    eeWriteChar(KtPhase[0]);
    eeWriteChar(KtPhase[1]);
    eeWriteChar(KtPhase[2]);

    for (i = 10; i < MAX_CNT_PAUSE; i++) {
        eeWriteChar(tempP[i]);
        eeWriteChar(timeP[i]);
    }
    eeWriteChar(minPressNBK);
    eeWriteChar(deltaPressNBK);
    eeWriteChar(timePressNBK);
    eeWriteChar(UprNasosNBK);
    eeWriteInt(ProcChimOtbCP);
    eeWriteChar(PIDTime);

    // Phisik: поддержка сортировки сенсоров
#if ENABLE_SENSOR_SORTING
    for (i = 0; i < MAX_DS1820; i++) {
        eeWriteChar(ds1820_nums[i]);
    }
#endif
#if USE_BMP280_SENSOR
    eeWriteChar(timePressAtm);
#endif
}

void readEEPROM()
{
    int i;
    unsigned char ctmp;
    ee_addr = 0;
    eeReadChar();
    IspReg = eeReadChar();
    Power = eeReadInt();
    UstPowerReg = eeReadInt();
    TempTerm = eeReadInt();
    Delta = eeReadInt();
    FlToUSART = eeReadChar();
    FlToGSM = eeReadChar();
    Temp1P = eeReadInt();
    Temp2P = eeReadInt();
    Temp3P = eeReadInt();
    for (i = 0; i < MAX_DS1820; i++)
        ds1820_popr[i] = eeReadChar();
    tEndRectRazgon = eeReadInt();
    PowerRect = eeReadInt();

    tEndRectOtbGlv = eeReadInt();
    timeChimRectOtbGlv = eeReadInt();
    ProcChimOtbGlv = eeReadInt();
    timeChimRectOtbSR = eeReadInt();
    tDeltaRect = eeReadChar();
    tEndRectOtbSR = eeReadInt();
    tEndRect = eeReadInt();
    PowerGlvDistil = eeReadInt();
    PowerDistil = eeReadInt();

    TempDeflBegDistil = eeReadInt();
    TempDefl = eeReadInt();
    DeltaDefl = eeReadChar();

    BeepEndProcess = eeReadChar();
    BeepStateProcess = eeReadChar();
    BeepKeyPress = eeReadChar();

    PowerRazvZerno = eeReadInt();

    tEndDistDefl = eeReadInt();
    minProcChimOtbSR = eeReadChar();
    PowerVarkaZerno = eeReadInt();
    TempKipenZator = eeReadInt();
    if (TempKipenZator > 1050 || TempKipenZator < 0)
        TempKipenZator = 800;
    TempZSPSld = eeReadChar();
    if (TempZSPSld > 100 || TempZSPSld < 0)
        TempZSPSld = 68;
    TempZSP = eeReadChar();
    if (TempZSP > 100 || TempZSP < 0)
        TempZSP = 68;
    PowerNBK = eeReadInt();
    SpeedNBKDst = eeReadChar();
    if (PowerNBK > 10000 || PowerNBK < 0)
        PowerNBK = 2400;
    if (SpeedNBKDst > 254)
        SpeedNBKDst = 0;
    PowerMinute = eeReadInt();
    if (PowerMinute > 10000 || PowerMinute < 0)
        PowerMinute = Power;
    begProcChimOtbSR = eeReadChar();
    if (begProcChimOtbSR > 100)
        begProcChimOtbSR = 40;
    P_MPX5010 = eeReadInt();
    if (P_MPX5010 > 800 || P_MPX5010 < -800)
        P_MPX5010 = 0;
    PeriodRefrServer = eeReadChar();
    timeRefrServer = PeriodRefrServer; // Таймер для отсчета ответа соединения
    NaprPeregrev = eeReadInt();
    if (NaprPeregrev > 1000)
        NaprPeregrev = 300;
    UrovenBarda = eeReadInt();
    //if (UrovenBarda>2000 || UrovenBarda<0) UrovenBarda=800;
    UrovenProvodimostSR = eeReadInt();
    if (UrovenProvodimostSR > 2000 || UrovenProvodimostSR == -1) {
        UrovenProvodimostSR = 0;
    }
    TimeStabKolonna = eeReadInt();
    if (TimeStabKolonna == -1)
        TimeStabKolonna = 900;

    DecrementCHIM = eeReadChar();
    if (DecrementCHIM == -1)
        DecrementCHIM = 10;
    IncrementCHIM = eeReadChar();
    if (IncrementCHIM == -1)
        IncrementCHIM = 0;
    TimeAutoIncCHIM = eeReadInt();
    if (TimeAutoIncCHIM == -1)
        TimeAutoIncCHIM = 600;

    CntCHIM = eeReadChar();
    for (i = 0; i < COUNT_CHIM; i++) {
        tempK[i] = eeReadInt();
        CHIM[i] = eeReadChar();
    }
    TimeRestabKolonna = eeReadInt();
    if (TimeRestabKolonna == -1)
        TimeRestabKolonna = 3600;

    CntPause = eeReadChar();
    for (i = 0; i < 10; i++) {
        // Если число пауз больше нуля, то считываем их
        if (CntPause > 0) {
            tempP[i] = eeReadChar();
            timeP[i] = eeReadChar();
        } else {
            // Иначе просто читаем чтобы счетчик чтений увеличился.
            eeReadChar();
            eeReadChar();
        }
        if (timeP[i] == 255)
            timeP[i] = 0;
    }

    if (CntPause == -1)
        CntPause = 8;

    CorrectASC712 = eeReadChar();
    if (CorrectASC712 == -1)
        CorrectASC712 = 0;

    ip[0] = eeReadChar();
    ip[1] = eeReadChar();
    ip[2] = eeReadChar();
    ip[3] = eeReadChar();
    ipPort = eeReadInt();

    for (i = 0; i < 10; i++) {
        ctmp = eeReadChar();
        if (ctmp != 255)
            idDevice[i] = ctmp;
    }
    idDevice[10] = 0;

    for (i = 0; i < 12; i++) {
        ctmp = eeReadChar();
        if (ctmp != 255)
            my_phone[i] = ctmp;
    }

    my_phone[12] = 0;
    AlarmMPX5010 = eeReadInt();
    if (AlarmMPX5010 == -1)
        AlarmMPX5010 = 0;

    FlAvtonom = eeReadChar();
    if (FlAvtonom == -1)
        FlAvtonom = 0;
    timeOpenBRD = eeReadChar();
    if (timeOpenBRD == 255)
        timeOpenBRD = 2;

    PIDTemp[0] = eeReadChar();
    PIDTemp[1] = eeReadChar();
    PIDTemp[2] = eeReadChar();
    if (PIDTemp[0] == -1) {
        PIDTemp[0] = 20;
        PIDTemp[1] = 10;
        PIDTemp[2] = 30;
    }

    for (i = 0; i < 11; i++) {
        ctmp = eeReadChar();
        if (ctmp != 255)
            WiFiAP[i] = ctmp;
    }
    WiFiAP[11] = 0;
    for (i = 0; i < 11; i++) {
        ctmp = eeReadChar();
        if (ctmp != 255)
            WiFiPass[i] = ctmp;
    }
    WiFiPass[11] = 0;
    CountFractionDist = eeReadChar();
    for (i = 0; i < MAX_CNT_FRACTION_DIST; i++) {
        TempFractionDist[i] = eeReadInt();
        if (TempFractionDist[i] == -1)
            TempFractionDist[i] = 0;
        PowerFractionDist[i] = eeReadInt();
        if (PowerFractionDist[i] == -1)
            PowerFractionDist[i] = 0;
        AngleFractionDist[i] = eeReadChar();
        if (AngleFractionDist[i] == 255)
            AngleFractionDist[i] = 0;
    }

    CountFractionRect = eeReadChar();
    for (i = 0; i < MAX_CNT_FRACTION_RECT; i++) {
        TempFractionRect[i] = eeReadInt();
        if (TempFractionRect[i] == -1)
            TempFractionRect[i] = 0;
        AngleFractionRect[i] = eeReadChar();
        if (AngleFractionRect[i] == 255)
            AngleFractionRect[i] = 0;
    }

    TempHLDZatorBrog1 = eeReadChar();
    if (TempHLDZatorBrog1 > 100 || TempHLDZatorBrog1 < 0)
        TempHLDZatorBrog1 = 36;

    PowerPhase[0] = eeReadInt();
    if (PowerPhase[0] < 0 || PowerPhase[0] > 10000)
        PowerPhase[0] = Power;
    PowerPhase[1] = eeReadInt();
    if (PowerPhase[1] < 0 || PowerPhase[1] > 10000)
        PowerPhase[1] = 0;
    PowerPhase[2] = eeReadInt();
    if (PowerPhase[2] < 0 || PowerPhase[2] > 10000)
        PowerPhase[2] = 0;
    if ((PowerPhase[0] + PowerPhase[1] + PowerPhase[2]) <= 0)
        PowerPhase[0] = Power;

    KtPhase[0] = eeReadChar();
    if (KtPhase[0] > 250)
        KtPhase[0] = 100;
    KtPhase[1] = eeReadChar();
    if (KtPhase[1] > 250)
        KtPhase[1] = 100;
    KtPhase[2] = eeReadChar();
    if (KtPhase[2] > 250)
        KtPhase[2] = 100;

    for (i = 10; i < MAX_CNT_PAUSE; i++) {
        // Если число пауз больше нуля, то считываем их
        if (CntPause > 0) {
            tempP[i] = eeReadChar();
            timeP[i] = eeReadChar();
        } else {
            // Иначе просто читаем чтобы счетчик чтений увеличился.
            eeReadChar();
            eeReadChar();
        }
        if (timeP[i] == 255)
            timeP[i] = 0;
    }
    minPressNBK = eeReadChar();
    if (minPressNBK == -1)
        minPressNBK = 26;
    deltaPressNBK = eeReadChar();
    if (deltaPressNBK == -1)
        deltaPressNBK = 25;
    timePressNBK = eeReadChar();
    if (timePressNBK == -1)
        timePressNBK = 36;

    UprNasosNBK = eeReadChar();
    if (UprNasosNBK == -1)
        UprNasosNBK = 1;
    ProcChimOtbCP = eeReadInt();
    if (ProcChimOtbCP == -1)
        ProcChimOtbCP = 0;
    PIDTime == eeReadChar();
    if (PIDTime == 255)
        PIDTime = 15;

        // Phisik: поддержка сортировки сенсоров
#if ENABLE_SENSOR_SORTING
    for (i = 0; i < MAX_DS1820; i++) {
        ds1820_nums[i] = eeReadChar();
        // DEBUG_SERIAL.println(ds1820_nums[i]);
        ds1820_nums[i] = min(ds1820_nums[i], MAX_DS1820 - 1);
    }
#endif

#if USE_BMP280_SENSOR
    timePressAtm = eeReadChar();
    if (timePressAtm == 255)
        timePressAtm = 25;
#endif

#ifdef TEST
    DEBUG_SERIAL.print(F("maxaddr:"));
    DEBUG_SERIAL.println(ee_addr);
#endif
}
