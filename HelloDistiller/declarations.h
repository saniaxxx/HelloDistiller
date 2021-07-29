// Последнее обновление 2018-02-21 by Phisik
// Описание переменных разделяемых между файлами

// HD.ino
#define MY_TX_BUFFER_SIZE 70
#define MY_RX_BUFFER_SIZE 70
#define MAX_TABLE_T 200
#define MAX_TABLE_SQRT 26
#define MAX_CNT_PAUSE 16
#define MAX_CNT_FRACTION_RECT 5
#define MAX_CNT_FRACTION_DIST 8
#define COUNT_CHIM 15

extern unsigned char hour, minute, second;

extern unsigned long TekPowerKLP;
extern unsigned long TekPower;
extern unsigned long SqNaprT, SqNaprPrevT;
extern unsigned int timeChimRectOtbSR;
extern unsigned int timeChimRectOtbGlv;
extern unsigned int TimeOpenTriacFact;
extern unsigned int TimeOpenTriac;
extern unsigned int TimeOpenKLP;
extern unsigned int TicZero;
extern unsigned int KlOpen[MAX_KLP];
extern unsigned int KlClose[MAX_KLP];
extern unsigned int BeepTime;
extern unsigned int KlCount[MAX_KLP];
extern unsigned int ipPort;
// extern unsigned int tableSQ[MAX_TABLE_SQRT];
extern unsigned char zPSOut;
extern unsigned char zPS;
extern unsigned char timeP[MAX_CNT_PAUSE];
extern unsigned char timeBRD;
extern unsigned char tableS10[MAX_TABLE_T];
extern unsigned char nPopr;
extern unsigned char flCrossZero;
extern unsigned char StateVolts;
extern unsigned char SpeedNBKDst;
extern unsigned char SpeedNBK;
extern unsigned char PIDTime;
extern unsigned char KtPhase[3];
extern unsigned char DeltaDefl;
extern unsigned char Counter1;
extern unsigned char CountAlarmVoda;
extern unsigned char CountAlarmUroven;
extern unsigned char AngleFractionRect[MAX_CNT_FRACTION_RECT];
extern unsigned char AngleFractionDist[MAX_CNT_FRACTION_DIST];
extern unsigned char AddPressKeys;
extern unsigned char AddPressKeys10;
extern unsigned char timeWaitGPRS;
extern unsigned char timeRefrServer;
extern unsigned char timeGPRS;
extern unsigned char ip[4];
extern unsigned char idDevice[11];
extern unsigned char flGPRSState;
extern unsigned char WiFiPass[12];
extern unsigned char WiFiAP[12];
extern unsigned char PeriodRefrServer;
extern unsigned char IspReg;
// extern unsigned char tableSQRT[MAX_TABLE_SQRT];
extern uint8_t ds1820_rom_codes[MAX_DS1820][9];
extern uint8_t ds1820_devices;
extern unsigned char CntErrDs18;
extern long SecondsEnd;
extern long Seconds;
extern long SecTempPrev;
extern long SecTempPrev1;
extern long SecOstatok;
extern int timerMinute;
extern int timeNBK;
extern int timeMIXER;
extern int time3;
extern int time2;
extern int time1;
extern int temps[MAX_DS1820];
extern int tempK[COUNT_CHIM];
extern int tStabSR;
extern int tEndRectRazgon;
extern int tEndRectOtbSR;
extern int tEndRectOtbGlv;
extern int tEndRect;
extern int tEndDistDefl;
#if ADJUST_COLUMN_STAB_TEMP
extern float lastStableT;
extern long SecTempPrev2;
#endif
extern int resultU;
extern int prev_index_input;
extern int index_input;
extern int indexOut;
extern int UstPwrPH3;
extern int UstPwrPH2;
extern int UstPwrPH1;
extern int UstPowerReg;
extern int UstPower;
extern int UrovenProvodimostSR;
extern int UrovenBarda;
extern int U_VODA;
extern int TimeStabKolonna;
extern int TempPrev;
extern int TempFractionRect[MAX_CNT_FRACTION_RECT];
extern int TempFractionDist[MAX_CNT_FRACTION_DIST];
extern int TempDeflBegDistil;
extern int TempDefl;
extern int R_TEN20;
extern int PowerVarkaZerno;
extern int PowerRect;
extern int PowerRazvZerno;
extern int PowerPhase[3];
extern int PowerNBK;
extern int PowerMinute;
extern int PowerGlvDistil;
extern int PowerFractionDist[MAX_CNT_FRACTION_DIST];
extern int PowerDistil;
extern int Power;
extern int OldErrTempOut;
extern int OldErrTemp;
extern int OldErrOut;
extern int OldErr;
extern int NewErrTemp;
extern int NewErr;
extern int NaprPeregrev;
extern int MaxTemps[MAX_DS1820];
extern float MaxVoltsOut;
extern float MaxIOut;
extern int KtT;
extern int ItTemp;
extern int It;
extern int FactPower;
extern int DtTemp;
extern int Dt;
extern int Angle;
extern int U_MPX5010;
extern int TimeRestabKolonna;
extern int TimeAutoIncCHIM;
extern int P_MPX5010;
extern int MaxPressByPeriod;
extern int FindKt;
extern int AlarmMPX5010;
extern int TempTerm;
extern int TempKipenZator;
extern int Temp3P;
extern int Temp2P;
extern int Temp1P;
extern int Delta;
extern int FlState;
extern int TekTemp;
extern const char sms_stop[];
extern const char sms_start[];
extern const char call_sms[];
extern const char call_gprs[];
extern char timePressNBK;
extern char timeOpenBRD;
extern char tempP[MAX_CNT_PAUSE];
extern char tDeltaRect;
extern char secPressKeys;
extern char pos_rx_buffer;
extern char my_tx_buffer[MY_TX_BUFFER_SIZE];
extern char my_rx_buffer[MY_RX_BUFFER_SIZE];
extern char my_gprs_buffer[MY_TX_BUFFER_SIZE];
extern char minProcChimOtbSR;
extern char minPressNBK;
extern char lcd_buffer[LCD_BUFFER_SIZE];
extern char flScanKbd;
extern char flRing;
extern char flPopr;
extern char flNoPhase;
extern char flNoAlarmLowPower;
extern char flNeedTemp;
extern char flNeedScanKbd;
extern char flNeedReadAlarm;
extern char flNeedAnalyse;
extern char flCorrASC712;
extern char flAutoDetectPowerTEN;
extern char flAllOff;
extern char flAlarmUroven;
extern char flAlarmMPX5010;
extern char deltaPressNBK;
extern char countAlrmNPG;
extern char begProcChimOtbSR;
extern char UprNasosNBK;
extern char TempZSPSld;
extern char TempZSP;
extern char TempHLDZatorBrog1;
extern char TekFraction;
extern char StepOut;
extern char StateNPG;
extern char StateMachine;
extern char SlaveON;
extern char ProcChimSR;
extern int ProcChimOtbGlv;
extern int ProcChimOtbCP;
extern char PIDTemp[3];
extern char NpgDt;
extern char NeedGPRS;
extern char NeedDisplaying;
extern char KlTek;
extern char KlState[MAX_KLP];
extern char KlSelect;
extern char KlReg[MAX_KLP];
extern char KeyCode;
extern char IncrementCHIM;
extern char FlUsart;
extern char DispDopInfo;
extern char DecrementCHIM;
extern char CountState;
extern char CountKeys;
extern char CountGasSensor;
extern char CountFractionRect;
extern char CountFractionDist;
extern char CountAlarmMPX5010;
extern char CorrectASC712;
extern char CntPeriod;
extern char CntPause;
extern char CntCHIM;
extern char CHIM[COUNT_CHIM];
extern char FlToUSART;
extern char FlToGSM;
extern char FlAvtonom;
extern char BeepStateProcess;
extern char BeepKeyPress;
extern char BeepEndProcess;
extern char DispPage;
extern char pdu_phone[14];
extern char my_phone[13];
extern char lastSMSState;
extern char SMSOnCall;
extern char stateAfterDelete;
extern char flNeedRefrServer;
extern char flNeedCall;
extern char cmdGPRSIsp;
extern char cmdGPRS;
extern char ErrGPRSConnectServer;
extern char ErrGPRSConnectInernet;
extern char ds1820_popr[MAX_DS1820];
extern char ds1820_flread[MAX_DS1820];
extern char NumErrDs18;
extern char CntErrPower;
extern int deltaPower;
extern const char my_version[];
extern int U_VODA, U_UROVEN, U_GAS, U_NPG, U_GLV; // Уровни воды, газа, НПГ, голов

extern volatile bool bLCDclearFlag;

#if USE_I2C_LCD
extern LiquidCrystal_I2C lcd;
#else
extern LiquidCrystal lcd;
#endif // USE_I2C_LCD

extern OneWire ds;

void DisplayData();
void my_beep(unsigned int mBeepTime);
void CloseAllKLP();
void SetAngle(unsigned char Angl);
void zero_cross_int();
void my_lcdprint(char* s);
void GetPhonePDU();
void ReadAlarm();
void OpenKLP();
void GetStateSerialErr();
void PrepareProcess();
void ProcessTermostat();
void ProcessRazvarZerno();
void ProcessHLDZatorByChiller();
void ProcessTimerMaxPower();
void ProcessBeerCloneBrau();
void ProcessDistillFractional();
void ProcessRectif();
void ProcessSimpleDistill();
void ProcessSimpleGlv();
void ProcessDistilDefl();
void ProcessNDRF();
void ProcessTestKLP();
void StateToSMS();
void GetStateSerial();
void ProcessGSM();
void ProcessNBK();
void RaspredPowerByPhase();
void ProcessPIDTemp(int NeedTemp, int FactTemp);
void ProcessPIDPress(int NeedTemp, int FactTemp);
char GetCHIMOtbor();
float my_sqrt(unsigned long tsQT);
void ScanDS18b20Slave();
void fillTableData();

extern unsigned char b_size[3]; // Размер брезинхема
extern unsigned char b_value[3]; // Текущее значение алгоритма
extern int b_error[3]; // Переменная
extern unsigned char b_stepNumber[3]; // Номер шага

void bresenham_Init(char n, unsigned char sz);
boolean bresenham_getNext(char n);

void ProcessNPG();

// eeprom.cpp
extern unsigned int ee_addr;
unsigned char eeReadChar();
void eeWriteChar(char p_value);
void eeWriteInt(int p_value);
unsigned int eeReadInt();
void readEEPROM();
void writeEEPROM();

// keyboard.cpp
void ScanKbd();

// mqtt.cpp
#if USE_MQTT_BROKER

#define MQTT_BUFFER_SIZE 50

extern char lcd_mqtt_buf1[LCD_BUFFER_SIZE];
extern char lcd_mqtt_buf2[LCD_BUFFER_SIZE];
extern char buf_pmem[LCD_BUFFER_SIZE];
extern PROGMEM const char fmt_lcd1[];
extern PROGMEM const char fmt_lcd2[];

void mqttSerialPrint(char*);
bool mqttSendStatus();
void handleMqttSerial();
void initMqtt();
#endif

#ifdef DEBUG
void my_debug();
#endif

#if ENABLE_SENSOR_SORTING
extern uint8_t ds1820_nums[MAX_DS1820];
#define DS_TEMP(i) temps[ds1820_nums[i]]
#define MAX_DS_TEMP(i) MaxTemps[ds1820_nums[i]]
#else
#define DS_TEMP(i) temps[i]
#define MAX_DS_TEMP(i) MaxTemps[i]
#endif

#if USE_BMP280_SENSOR
extern Adafruit_BMP280 bmp; // I2C

extern int PressAtm;
extern unsigned char flReadPress;
extern unsigned char timePressAtm;
extern char ds1820_poprPress[MAX_DS1820]; // Поправки к температуре датчиков
#endif