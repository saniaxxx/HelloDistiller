// Последнее обновление 2018-07-25 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам
// тут собственно void setup() 

#include "configuration.h"
#include "declarations.h"

void setup() 
{
    #if USE_WDT
      wdt_enable(WDTO_8S);
    #endif
    
    
    unsigned char i;
    //my_beep(5);
    // set up the LCD's number of columns and rows: 

    DEBUG_SERIAL.begin(DEBUG_SERIAL_BAUDRATE);
    DEBUG_SERIAL.println("Starting up...");
	DEBUG_SERIAL.flush();
    
#if USE_I2C_LCD
	// Phisik:  пробуйте по разному, моя библиотека этих строчек не требует
	//lcd.begin(LCD_WIDTH, LCD_HEIGHT);
	//lcd.init();

	lcd.begin(); 
	lcd.backlight();
#else
	lcd.begin(LCD_WIDTH, LCD_HEIGHT);
#endif // USE_I2C_LCD
    
    #if USE_GSM_WIFI
        GSM_SERIAL.begin(GSM_SERIAL_BAUDRATE);
    #endif
      
    // MQTT code by max506 & limon
    #if USE_MQTT_BROKER
       initMqtt(); 
    #endif  // MQTT_BROKER
      
    
    #if NUM_PHASE>0
      Serial1.begin(9600);
    #endif
    
      StateVolts=6;
    

    
      // Print a message to the LCD.
      //  my_lcdprint(F("hello, world!"));
      //  analogReference(DEFAULT);
    
      //  #if USE_LCD_KEYPAD_SHIELD==0
      //    analogReference(INTERNAL2V56);
      //  #else
      // теперь опорно напряжение всегда делаем +5В, чтобы работа программы была одинаковой, как с использованием дисплей шильда, так и без него
      analogReference(DEFAULT);
      // #endif
    
      pinMode(PIN_TRIAC, OUTPUT);
      pinMode(PIN_ALL_OFF, OUTPUT);
      pinMode(PIN_RST_WDT, OUTPUT);
    
      pinMode(PIN_RZG_ON, OUTPUT);
      digitalWrite(PIN_RZG_ON,!RELAY_HIGH);
    
      pinMode(PIN_START_1, OUTPUT);
      digitalWrite(PIN_START_1,!RELAY_HIGH);
      pinMode(PIN_START_2, OUTPUT);
      digitalWrite(PIN_START_2,!RELAY_HIGH);
    
      pinMode(PIN_NASOS_NBK, OUTPUT);
      pinMode(PIN_NASOS_NBK_BRD, OUTPUT);
      pinMode(PIN_MIXER, OUTPUT);
      digitalWrite(PIN_MIXER,!RELAY_HIGH);
      digitalWrite(PIN_NASOS_NBK, LOW);  
      pinMode(PIN_TERMOSTAT_ON, OUTPUT);
      pinMode(PIN_TERMOSTAT_OFF, OUTPUT);
    
      if (UROVEN_ALARM==1)
      {
        pinMode(PIN_ALARM_VODA, INPUT);
        pinMode(PIN_ALARM_UROVEN, INPUT);
      }
    
    
      if (UrovenProvodimostSR==1) 
      {
        pinMode(PIN_PROVODIMOST_SR, INPUT);
      }
    
    #ifdef NPG_UROVEN_PIN_MIN
      //    pinMode(NPG_UROVEN_PIN_MIN, INPUT);           // назначить выводу порт ввода
      //    digitalWrite(NPG_UROVEN_PIN_MIN, HIGH);       // включить подтягивающий резистор
      //    pinMode(NPG_UROVEN_PIN_WORK, INPUT);           // назначить выводу порт ввода
      //    digitalWrite(NPG_UROVEN_PIN_WORK, HIGH);       // включить подтягивающий резистор
    
    #endif
    
    
    #if !USE_LCD_KEYPAD_SHIELD
      pinMode(PIN_SELECT, INPUT);           // назначить выводу порт ввода
      digitalWrite(PIN_SELECT, HIGH);       // включить подтягивающий резистор
    
      pinMode(PIN_RIGHT, INPUT);           // назначить выводу порт ввода
      digitalWrite(PIN_RIGHT, HIGH);       // включить подтягивающий резистор
    
      pinMode(PIN_LEFT, INPUT);           // назначить выводу порт ввода
      digitalWrite(PIN_LEFT, HIGH);       // включить подтягивающий резистор
    
      pinMode(PIN_UP, INPUT);           // назначить выводу порт ввода
      digitalWrite(PIN_UP, HIGH);       // включить подтягивающий резистор
    
      pinMode(PIN_DOWN, INPUT);           // назначить выводу порт ввода
      digitalWrite(PIN_DOWN, HIGH);       // включить подтягивающий резистор
    #endif
    
      // Проходим по клапанам и устанавливаем им ШИМ и настраиваем пины
      for(i=0;i<MAX_KLP;i++)
      {
    #if SIMPLED_VERSION<30
        pinMode(PIN_KLP_BEG+i, OUTPUT);
        digitalWrite(PIN_KLP_BEG+i, !KLP_HIGH);  
    #else
        pinMode(PIN_KLP_BEG+i+i, OUTPUT);
        digitalWrite(PIN_KLP_BEG+i+i, !KLP_HIGH);  
    #endif
        KlOpen[i]=0;    
        KlClose[i]=10;   
        KlState[i]=0;          // Начальное состояние - клапан закрыт, счетчик 0
        KlCount[i]=0;  
      }
    #if NUM_PHASE>1
      for(i=0;i<NUM_PHASE;i++)
      {
        digitalWrite(PIN_KLP_BEG,HIGH);
        digitalWrite(PIN_KLP_BEG+4,HIGH);
        delay(1000);
        digitalWrite(PIN_KLP_BEG+4,LOW);
        digitalWrite(PIN_KLP_BEG,LOW);
        delay(1000); 
      }
    #endif
    
    
      // Устанавливаем на 1-е прерывание функцию (это pin 3) обработку от детектора нулюя сетевого напряжения
      // Сейчас всегда устанавливаем обработку на переход с ненуля на ноль, чтобы было немного больше времени на программу в прерывании нуля.
      attachInterrupt(digitalPinToInterrupt(PIN_ZERO_DETECT), zero_cross_int, FALLING);
            
      
      // Таймер 2 используется для подсчета числа секунд и, возможно, программного небыстрого ШИМа
      // Прерываение по этому таймеру будет вызываться 125 раз в секунду.
      // Timer/Counter 2 initialization
      // Clock source: System Clock
      // Clock value: 62,500 kHz
      // Mode: Normal top=0xFF
      // OC2A output: Disconnected
      // OC2B output: Disconnected
      TCCR2A=0x00;
      TCCR2B=0x07;
      TCNT2=0x00;
      OCR2A=0x7D;
      OCR2B=0x00;  
      //  // Timer/Counter 1 Interrupt(s) initialization
      TIMSK2=0x02;
    
    
      // Таймер 3 будет использоваться для такой, обычно очень несвойственной для него процедуры, как "замена" аппаратного прерывания АЦП.
      // (сейчас таймер свободен, так что может быть задействован для других целей)
    #if SIMPLED_VERSION==20
      // Timer/Counter 3 initialization
      // Clock source: System Clock
      // Clock value: 250,000 kHz
      // Mode: Normal top=0xFFFF
      // OC3A output: Discon.
      // OC3B output: Discon.
      // OC3C output: Discon.
      // Noise Canceler: Off
      // Input Capture on Falling Edge
      // Timer3 Overflow Interrupt: Off
      // Input Capture Interrupt: Off
      // Compare A Match Interrupt: On
      // Compare B Match Interrupt: Off
      // Compare C Match Interrupt: Off
      TCCR3A=0x00;
      TCCR3B=0x03;
      TCNT3H=0x00;
      TCNT3L=0x00;
      ICR3H=0x00;
      ICR3L=0x00;
      OCR3AH=0x00;
      OCR3AL=0x00;
      OCR3BH=0x00;
      OCR3BL=0x00;
      OCR3CH=0x00;
      OCR3CL=0x00;
      // Timer/Counter 3 Interrupt(s) initialization
      TIMSK3=0x02;
    #endif
    
    
      // Таймер 4 будет использоваться для подсчета числа тиков при вызове АЦП в расчете среднеквадратичного 
      // (поскольку непонятно, чего и сколько это занимает в Ардуино - исходный код разбирать неохота)
      // Timer/Counter 4 initialization
      // Clock source: System Clock
      // Clock value: 250,000 kHz
      // Mode: Normal top=0xFFFF
      // OC4A output: Discon.
      // OC4B output: Discon.
      // OC4C output: Discon.
      // Noise Canceler: Off
      // Input Capture on Falling Edge
      // Timer4 Overflow Interrupt: Off
      // Input Capture Interrupt: Off
      // Compare A Match Interrupt: Off
      // Compare B Match Interrupt: Off
      // Compare C Match Interrupt: Off
      TCCR4A=0x00;
      TCCR4B=0x03;
      TCNT4H=0x00;
      TCNT4L=0x00;
      ICR4H=0x00;
      ICR4L=0x00;
      OCR4AH=0x00;
      OCR4AL=0x00;
      OCR4BH=0x00;
      OCR4BL=0x00;
      OCR4CH=0x00;
      OCR4CL=0x00;
      // Timer/Counter 4 Interrupt(s) initialization
      TIMSK4=0x00;
    
    #if SIMPLED_VERSION==20
      // Timer/Counter 4 Interrupt(s) initialization
      TIMSK4=0x02;
    #endif  
    
    
      // Таймер 5 будет использоваться для управления симистором регулятора мощности по прерыванию COMPA.
      // Таймер 5 будет использоваться для управления симистороми клапанов по прерыванию COMPB.
      // Таймер 5 будет использоваться для управления симистором регулятора оборотов двигателя прерыванию COMPС.
      // Timer/Counter 5 initialization
      // Clock source: System Clock
      // Clock source: System Clock
      // Clock value: 250,000 kHz
      // Mode: Normal top=0xFFFF
      // OC5A output: Discon.
      // OC5B output: Discon.
      // OC5C output: Discon.
      // Noise Canceler: Off
      // Input Capture on Falling Edge
      // Timer5 Overflow Interrupt: Off
      // Input Capture Interrupt: Off
      // Compare A Match Interrupt: On
      // Compare B Match Interrupt: On
      // Compare C Match Interrupt: On
    
      TCCR5A=0x00;
      TCCR5B=0x03;
      TCNT5H=0x00;
      TCNT5L=0x00;
      ICR5H=0x00;
      ICR5L=0x00;
      OCR5AH=0x00;
      OCR5AL=0x02;
      OCR5BH=0x00;
      OCR5BL=0x00;
      OCR5CH=0x00;
      OCR5CL=0x00;
      // Timer/Counter 5 Interrupt(s) initialization
      //  TIMSK5=0x06;
      TIMSK5=0x0E;// Инициализируем прерывания таймера
    
    
      //  TekPower=(unsigned long) UstPower*220*220/ Power;
      nPopr=1;
      ds1820_devices=0;
      while(ds.search(ds1820_rom_codes[ds1820_devices])) {
        ds1820_devices++;
        if (ds1820_devices>=MAX_DS1820) break;
      }
      ds.reset_search();

    #if SIMPLED_VERSION>=30 || SIMPLED_VERSION==5
      if (ds1820_devices<3 && NUM_PHASE<=1) ScanDS18b20Slave();
    #endif

#if ENABLE_DS18B20_PRESET
	  // Updated by Phisik on 2018-09-15
	  // Сейчас мы отсортируем датчики по готовым пресетам
	  // Алгоритм такой, берем первый адрес из ds1820_rom_codes и перебираем все наши пресеты
	  // до первого совпадения, если совпадение найдено, то берем оставльные датчики, и ищем их в пресете,
	  // если все датчики найдены, то заполняем ds1820_rom_codes адресами из пресета.

	  // NB! фактически мы адреса не сортируем, мы используем порядок заданный пользователем
	  // Поэтому внимательно забиваем ВСЕ адреса в пресеты, иначе работать правильно не будет!

	  if (DS18B20_PRESET_NUM>0) {
		  // Это будет номер найденного пресета
		  int nPreset;
		  
		  // Перебираем каждый пресет
		  for (nPreset = 0; nPreset < DS18B20_PRESET_NUM; nPreset++) {
			  bool bMatchFound = false;

			  // Ищем в пресете все наши датчики
			  for(int i=0; i<ds1820_devices; i++) {
				  bMatchFound = false;

				  // Перебираем каждый датчик в пресете и сравниваем его с нашим
				  for(int j=0; j<MAX_DS1820; j++) {  
					  int mismatch = 0;
					  for (int k = 0; k < 8 && mismatch == 0; k++)
						  mismatch += ds1820_rom_codes[i][k] - dsSensorPreset[nPreset][j][k];

					  // Если нашли сенсор, выходим из цикла
					  if (mismatch == 0)  {
						  bMatchFound = true;
						  break;
					  }
				  }

				  // Если не нашли сенсор, идем к следующему пресету
				  if (!bMatchFound) break;
			  }

			  // Если нашли все сенсоры, то выходим из цикла
			  if (bMatchFound) break;
		  } 

		  // Если мы нашли пресет, в котором есть все наши датчики, 
		  // то заполняем ds1820_rom_codes[][] данными из пресета
		  if(nPreset<DS18B20_PRESET_NUM) {
			  for(int j=0; j<ds1820_devices; j++)
				  for(int k=0; k<8; k++)
					  ds1820_rom_codes[j][k] = dsSensorPreset[nPreset][j][k];
		  }

#ifdef DEBUG
		  sprintf_P(lcd_buffer,PSTR("Found match with %n preset for DS18B20 sensors"), nPreset);
		  DEBUG_SERIAL.println(lcd_buffer);
#endif

	  } // if (DS18B20_PRESET_NUM>0)
#endif      
      
    lcd.clear();     
    #ifdef DEBUG
      ds1820_devices=3;
      sprintf_P(lcd_buffer,PSTR("O\277\273a\343\272a=%u"),ds1820_devices);                                         //Отладка=
    #else
      sprintf_P(lcd_buffer,PSTR("ds18B20 \276o\343\272\273\306\300e\275o-%u"),(int) ds1820_devices);               //подключено-
    #endif
    
      my_lcdprint(lcd_buffer); 
      sprintf_P(lcd_buffer,PSTR("\251po\263\275\307(%i),Po\267\273\270\263a(%i)"), USE_ALARM_UROVEN, USE_ALARM_VODA); //Уровня(), Розлива()                     
      my_lcdprint(lcd_buffer); 
      lcd.setCursor(0, 3);                     
      sprintf_P(lcd_buffer,PSTR("\250po\263o\343.(%i),\250o\276p.(%1i)"), UrovenProvodimostSR, (int)nPopr);        //Провод.(), Попр.()
      my_lcdprint(lcd_buffer);  
      lcd.setCursor(0, 2);                          
      sprintf_P(lcd_buffer,PSTR("\245c\276o\273\304-\275\270e \343a\277\300\270\272o\263:"), USE_ALARM_UROVEN);    //Исполь-ние датчиков:  
      nPopr=0;                    
      lcd.setCursor(0, 1);
      my_lcdprint(lcd_buffer);  
      delay(4000);
      
      lcd.clear();
      sprintf_P(lcd_buffer,PSTR("Bepc\270\307(%s) RWR=%i"),my_version,PR_REWRITE_EEPROM);                           //Версия() RWR=
      my_lcdprint(lcd_buffer);                 
      lcd.setCursor(0, 1);
      
    #ifndef USE_SLAVE
      my_lcdprint(lcd_buffer);
      lcd.setCursor(0, 3);    
      sprintf_P(lcd_buffer,PSTR(" <<HelloDistiller>> "));                                                           //<<HelloDistiller>>
      my_lcdprint(lcd_buffer);
      lcd.setCursor(0, 1);
      sprintf_P(lcd_buffer,PSTR("Ko\275\344\270\264\171pa\345\270\307(%u)"),SIMPLED_VERSION);                       //Конфигурация()
        
    #else
      my_lcdprint(lcd_buffer);
      lcd.setCursor(0, 3);    
      sprintf_P(lcd_buffer,PSTR(" <<HelloDistiller>> "));                                                           //<<HelloDistiller>>
      my_lcdprint(lcd_buffer);
      lcd.setCursor(0, 1);
      sprintf_P(lcd_buffer,PSTR("Ko\275\344\270\264\171pa-\307(%u) Slave"),SIMPLED_VERSION);                        //Конфигура-я() Slave
      
    #endif
      my_lcdprint(lcd_buffer);                      
      delay(3000);
      lcd.clear();      
      flNeedTemp=0;
      StepOut=0;
    
    
      // Читаем ранее сохраненные значения из энергонезависимой памяти.
      if (EEPROM.read(0)!=PR_REWRITE_EEPROM)
      { 
        writeEEPROM();
      }
    
      for(i=0;i<ds1820_devices;i++)
      {
        temps[i]=0;
        MaxTemps[i]=0;
        ds1820_popr[i]=0;    // Поправки к температуре датчиков устанавливаем в ноль (в дальнейшем они сами считаются из eeprom, если были ранее записаны).
        // Выдадим номера датчиков.
        if (FlToUSART)
        {
          sprintf_P(my_tx_buffer,PSTR("ds18b20\t%1X\t%02X%02X%02X%02X%02X%02X%02X%02X"),i,ds1820_rom_codes[i][0],ds1820_rom_codes[i][1],ds1820_rom_codes[i][2],
          ds1820_rom_codes[i][3],ds1820_rom_codes[i][4],ds1820_rom_codes[i][5],ds1820_rom_codes[i][6],ds1820_rom_codes[i][7]);
          DEBUG_SERIAL.println(my_tx_buffer);
        }
      }
    
      readEEPROM();

  #if USE_BMP280_SENSOR
	  ScanKbd();
	  // Если нажата кнопка "Вверх", то переключаем на прямое управление.
	  //if  (KeyCode==4) { FlToGSM=12; WiFiChl=1; }
	  // Если нажата кнопка "Вниз", то отключаем датчик атмосферного давления
	  
	  if  (KeyCode==5) timePressAtm=25;

	  if (timePressAtm>=30)
	  {
		  lcd.clear();
		  sprintf_P(lcd_buffer,PSTR("O\276poc bmp280..."));                  //Опрос bmp280...
		  lcd.setCursor(0, 0);
		  my_lcdprint(lcd_buffer);  

		  if (!bmp.begin()) {  
			  //Serial.println("Could not find a valid BMP280 sensor, check wiring!");
			  //Serial.println(bmp);
			  timePressAtm=25;
			  sprintf_P(lcd_buffer,PSTR("O\301\270\262\272a!"));               //Ошибка!
		  }
		  else
		  {
			  sprintf_P(lcd_buffer,PSTR("OK"));                                //OK
		  }
		  lcd.setCursor(0, 1);
		  my_lcdprint(lcd_buffer);  
		  delay(1500);
		  lcd.clear();
  }
  #endif // USE_BPM

    
    #ifdef USE_SLAVE
      IspReg=103; // Для Slave режима всегда устанавливаем регулятор мощности при перезагрузке
      pinMode(PIN_SLAVE_0, INPUT);
      pinMode(PIN_SLAVE_1, INPUT);
      pinMode(PIN_SLAVE_2, INPUT);
      pinMode(PIN_SLAVE_3, INPUT);
    #else
      pinMode(PIN_SLAVE_0, OUTPUT);
      pinMode(PIN_SLAVE_1, OUTPUT);
      pinMode(PIN_SLAVE_2, OUTPUT);
      pinMode(PIN_SLAVE_3, OUTPUT);
      // Отключаем подчиненные контроллеры
      digitalWrite(PIN_SLAVE_0, 0);
      digitalWrite(PIN_SLAVE_1, 0);
      digitalWrite(PIN_SLAVE_2, 0);
      digitalWrite(PIN_SLAVE_3, 0);
    #endif
    
    #if SIMPLED_VERSION==20 || NUM_PHASE>1
      IspReg=103; // Для режима регулятора всегда устанавливаем регулятор мощности при перезагрузке
      Power=1000;
      UstPowerReg=0;
      UstPower=0;
      UstPwrPH1=0;
    #endif
    
    #ifndef DEBUG
      my_beep(10);
    #endif
    
      if (IspReg>=240) IspReg=101;
      // Выдаем тестовую команду
      // InitGSM();
    
      if (IspReg>=250) IspReg=101;

    #if USE_GSM_WIFI==1    
          GetPhonePDU();
    #endif



   
    
      CloseAllKLP();
      //  digitalWrite(PIN_REG_ON,LOW);
      digitalWrite(PIN_RZG_ON,!RELAY_HIGH);
      digitalWrite(PIN_ALL_OFF,ALL_OFF_HIGH);
      delay(100);
      digitalWrite(PIN_ALL_OFF,!ALL_OFF_HIGH);
      flAllOff=0;
    
      digitalWrite(PIN_TERMOSTAT_ON,LOW);
      digitalWrite(PIN_TERMOSTAT_OFF,LOW);
      UstPower=0;
      StateNPG=0;
      StateMachine=0;
    
      bresenham_Init(0,200);
      bresenham_Init(1,125);
      bresenham_Init(2,125);
    
      b_value[0]=0;
    
    #ifdef TEST
      DS_TEMP(TEMP_KUB)=799;
      GetCHIMOtbor();
      DS_TEMP(TEMP_KUB)=880;
      GetCHIMOtbor();
      DS_TEMP(TEMP_KUB)=890;
      GetCHIMOtbor();
      DS_TEMP(TEMP_KUB)=900;
      GetCHIMOtbor();
      DS_TEMP(TEMP_KUB)=960;
      GetCHIMOtbor();
      DS_TEMP(TEMP_KUB)=980;
      GetCHIMOtbor();
    #endif
    
      
      fillTableData();
      
      StateVolts=0;
      
      #if USE_GSM_WIFI==1
        // Состояние модема - инициализация   
        flGPRSState=98;
        // После инициализации переходим в нулевое состояние.
        stateAfterDelete=0;
        // На инициализации даем максимум 120 секунд
        timeGPRS=120;
      #endif

      MaxVoltsOut=0;
      MaxIOut=0;
}   // setup()


void fillTableData(){
  // Устанавливаем значение таблицы открытия симистора
      tableS10[ 0]= 217;//tableK[ 0]= 5;
      tableS10[ 1]= 217;//tableK[ 1]= 10;
      tableS10[ 2]= 217;//tableK[ 2]= 15;
      tableS10[ 3]= 214;//tableK[ 3]= 20;
      tableS10[ 4]= 211;//tableK[ 4]= 25;
      tableS10[ 5]= 208;//tableK[ 5]= 30;
      tableS10[ 6]= 206;//tableK[ 6]= 35;
      tableS10[ 7]= 204;//tableK[ 7]= 40;
      tableS10[ 8]= 202;//tableK[ 8]= 45;
      tableS10[ 9]= 200;//tableK[ 9]= 50;
      tableS10[ 10]= 198;//tableK[ 10]= 55;
      tableS10[ 11]= 196;//tableK[ 11]= 60;
      tableS10[ 12]= 195;//tableK[ 12]= 65;
      tableS10[ 13]= 193;//tableK[ 13]= 70;
      tableS10[ 14]= 192;//tableK[ 14]= 75;
      tableS10[ 15]= 190;//tableK[ 15]= 80;
      tableS10[ 16]= 189;//tableK[ 16]= 85;
      tableS10[ 17]= 188;//tableK[ 17]= 90;
      tableS10[ 18]= 186;//tableK[ 18]= 95;
      tableS10[ 19]= 185;//tableK[ 19]= 100;
      tableS10[ 20]= 184;//tableK[ 20]= 105;
      tableS10[ 21]= 183;//tableK[ 21]= 110;
      tableS10[ 22]= 182;//tableK[ 22]= 115;
      tableS10[ 23]= 181;//tableK[ 23]= 120;
      tableS10[ 24]= 180;//tableK[ 24]= 125;
      tableS10[ 25]= 179;//tableK[ 25]= 130;
      tableS10[ 26]= 178;//tableK[ 26]= 135;
      tableS10[ 27]= 177;//tableK[ 27]= 140;
      tableS10[ 28]= 176;//tableK[ 28]= 145;
      tableS10[ 29]= 175;//tableK[ 29]= 150;
      tableS10[ 30]= 174;//tableK[ 30]= 155;
      tableS10[ 31]= 173;//tableK[ 31]= 160;
      tableS10[ 32]= 172;//tableK[ 32]= 165;
      tableS10[ 33]= 171;//tableK[ 33]= 170;
      tableS10[ 34]= 170;//tableK[ 34]= 175;
      tableS10[ 35]= 169;//tableK[ 35]= 180;
      tableS10[ 36]= 168;//tableK[ 36]= 185;
      tableS10[ 37]= 168;//tableK[ 37]= 190;
      tableS10[ 38]= 167;//tableK[ 38]= 195;
      tableS10[ 39]= 166;//tableK[ 39]= 200;
      tableS10[ 40]= 165;//tableK[ 40]= 205;
      tableS10[ 41]= 164;//tableK[ 41]= 210;
      tableS10[ 42]= 164;//tableK[ 42]= 215;
      tableS10[ 43]= 163;//tableK[ 43]= 220;
      tableS10[ 44]= 162;//tableK[ 44]= 225;
      tableS10[ 45]= 161;//tableK[ 45]= 230;
      tableS10[ 46]= 160;//tableK[ 46]= 235;
      tableS10[ 47]= 160;//tableK[ 47]= 240;
      tableS10[ 48]= 159;//tableK[ 48]= 245;
      tableS10[ 49]= 158;//tableK[ 49]= 250;
      tableS10[ 50]= 157;//tableK[ 50]= 255;
      tableS10[ 51]= 157;//tableK[ 51]= 260;
      tableS10[ 52]= 156;//tableK[ 52]= 265;
      tableS10[ 53]= 155;//tableK[ 53]= 270;
      tableS10[ 54]= 154;//tableK[ 54]= 275;
      tableS10[ 55]= 154;//tableK[ 55]= 280;
      tableS10[ 56]= 153;//tableK[ 56]= 285;
      tableS10[ 57]= 152;//tableK[ 57]= 290;
      tableS10[ 58]= 152;//tableK[ 58]= 295;
      tableS10[ 59]= 151;//tableK[ 59]= 300;
      tableS10[ 60]= 150;//tableK[ 60]= 305;
      tableS10[ 61]= 150;//tableK[ 61]= 310;
      tableS10[ 62]= 149;//tableK[ 62]= 315;
      tableS10[ 63]= 148;//tableK[ 63]= 320;
      tableS10[ 64]= 148;//tableK[ 64]= 325;
      tableS10[ 65]= 147;//tableK[ 65]= 330;
      tableS10[ 66]= 146;//tableK[ 66]= 335;
      tableS10[ 67]= 146;//tableK[ 67]= 340;
      tableS10[ 68]= 145;//tableK[ 68]= 345;
      tableS10[ 69]= 144;//tableK[ 69]= 350;
      tableS10[ 70]= 144;//tableK[ 70]= 355;
      tableS10[ 71]= 143;//tableK[ 71]= 360;
      tableS10[ 72]= 142;//tableK[ 72]= 365;
      tableS10[ 73]= 142;//tableK[ 73]= 370;
      tableS10[ 74]= 141;//tableK[ 74]= 375;
      tableS10[ 75]= 140;//tableK[ 75]= 380;
      tableS10[ 76]= 140;//tableK[ 76]= 385;
      tableS10[ 77]= 139;//tableK[ 77]= 390;
      tableS10[ 78]= 138;//tableK[ 78]= 395;
      tableS10[ 79]= 138;//tableK[ 79]= 400;
      tableS10[ 80]= 137;//tableK[ 80]= 405;
      tableS10[ 81]= 136;//tableK[ 81]= 410;
      tableS10[ 82]= 136;//tableK[ 82]= 415;
      tableS10[ 83]= 135;//tableK[ 83]= 420;
      tableS10[ 84]= 134;//tableK[ 84]= 425;
      tableS10[ 85]= 134;//tableK[ 85]= 430;
      tableS10[ 86]= 133;//tableK[ 86]= 435;
      tableS10[ 87]= 132;//tableK[ 87]= 440;
      tableS10[ 88]= 132;//tableK[ 88]= 445;
      tableS10[ 89]= 131;//tableK[ 89]= 450;
      tableS10[ 90]= 131;//tableK[ 90]= 455;
      tableS10[ 91]= 130;//tableK[ 91]= 460;
      tableS10[ 92]= 129;//tableK[ 92]= 465;
      tableS10[ 93]= 129;//tableK[ 93]= 470;
      tableS10[ 94]= 128;//tableK[ 94]= 475;
      tableS10[ 95]= 128;//tableK[ 95]= 480;
      tableS10[ 96]= 127;//tableK[ 96]= 485;
      tableS10[ 97]= 126;//tableK[ 97]= 490;
      tableS10[ 98]= 126;//tableK[ 98]= 495;
      tableS10[ 99]= 125;//tableK[ 99]= 500;
      tableS10[ 100]= 124;//tableK[ 100]= 505;
      tableS10[ 101]= 124;//tableK[ 101]= 510;
      tableS10[ 102]= 123;//tableK[ 102]= 515;
      tableS10[ 103]= 122;//tableK[ 103]= 520;
      tableS10[ 104]= 122;//tableK[ 104]= 525;
      tableS10[ 105]= 121;//tableK[ 105]= 530;
      tableS10[ 106]= 121;//tableK[ 106]= 535;
      tableS10[ 107]= 120;//tableK[ 107]= 540;
      tableS10[ 108]= 119;//tableK[ 108]= 545;
      tableS10[ 109]= 119;//tableK[ 109]= 550;
      tableS10[ 110]= 118;//tableK[ 110]= 555;
      tableS10[ 111]= 118;//tableK[ 111]= 560;
      tableS10[ 112]= 117;//tableK[ 112]= 565;
      tableS10[ 113]= 116;//tableK[ 113]= 570;
      tableS10[ 114]= 116;//tableK[ 114]= 575;
      tableS10[ 115]= 115;//tableK[ 115]= 580;
      tableS10[ 116]= 114;//tableK[ 116]= 585;
      tableS10[ 117]= 114;//tableK[ 117]= 590;
      tableS10[ 118]= 113;//tableK[ 118]= 595;
      tableS10[ 119]= 112;//tableK[ 119]= 600;
      tableS10[ 120]= 112;//tableK[ 120]= 605;
      tableS10[ 121]= 111;//tableK[ 121]= 610;
      tableS10[ 122]= 110;//tableK[ 122]= 615;
      tableS10[ 123]= 110;//tableK[ 123]= 620;
      tableS10[ 124]= 109;//tableK[ 124]= 625;
      tableS10[ 125]= 108;//tableK[ 125]= 630;
      tableS10[ 126]= 108;//tableK[ 126]= 635;
      tableS10[ 127]= 107;//tableK[ 127]= 640;
      tableS10[ 128]= 107;//tableK[ 128]= 645;
      tableS10[ 129]= 106;//tableK[ 129]= 650;
      tableS10[ 130]= 105;//tableK[ 130]= 655;
      tableS10[ 131]= 105;//tableK[ 131]= 660;
      tableS10[ 132]= 104;//tableK[ 132]= 665;
      tableS10[ 133]= 103;//tableK[ 133]= 670;
      tableS10[ 134]= 103;//tableK[ 134]= 675;
      tableS10[ 135]= 102;//tableK[ 135]= 680;
      tableS10[ 136]= 101;//tableK[ 136]= 685;
      tableS10[ 137]= 100;//tableK[ 137]= 690;
      tableS10[ 138]= 100;//tableK[ 138]= 695;
      tableS10[ 139]= 99;//tableK[ 139]= 700;
      tableS10[ 140]= 98;//tableK[ 140]= 705;
      tableS10[ 141]= 98;//tableK[ 141]= 710;
      tableS10[ 142]= 97;//tableK[ 142]= 715;
      tableS10[ 143]= 96;//tableK[ 143]= 720;
      tableS10[ 144]= 96;//tableK[ 144]= 725;
      tableS10[ 145]= 95;//tableK[ 145]= 730;
      tableS10[ 146]= 94;//tableK[ 146]= 735;
      tableS10[ 147]= 93;//tableK[ 147]= 740;
      tableS10[ 148]= 93;//tableK[ 148]= 745;
      tableS10[ 149]= 92;//tableK[ 149]= 750;
      tableS10[ 150]= 91;//tableK[ 150]= 755;
      tableS10[ 151]= 90;//tableK[ 151]= 760;
      tableS10[ 152]= 90;//tableK[ 152]= 765;
      tableS10[ 153]= 89;//tableK[ 153]= 770;
      tableS10[ 154]= 88;//tableK[ 154]= 775;
      tableS10[ 155]= 87;//tableK[ 155]= 780;
      tableS10[ 156]= 86;//tableK[ 156]= 785;
      tableS10[ 157]= 86;//tableK[ 157]= 790;
      tableS10[ 158]= 85;//tableK[ 158]= 795;
      tableS10[ 159]= 84;//tableK[ 159]= 800;
      tableS10[ 160]= 83;//tableK[ 160]= 805;
      tableS10[ 161]= 82;//tableK[ 161]= 810;
      tableS10[ 162]= 82;//tableK[ 162]= 815;
      tableS10[ 163]= 81;//tableK[ 163]= 820;
      tableS10[ 164]= 80;//tableK[ 164]= 825;
      tableS10[ 165]= 79;//tableK[ 165]= 830;
      tableS10[ 166]= 78;//tableK[ 166]= 835;
      tableS10[ 167]= 77;//tableK[ 167]= 840;
      tableS10[ 168]= 76;//tableK[ 168]= 845;
      tableS10[ 169]= 75;//tableK[ 169]= 850;
      tableS10[ 170]= 74;//tableK[ 170]= 855;
      tableS10[ 171]= 73;//tableK[ 171]= 860;
      tableS10[ 172]= 72;//tableK[ 172]= 865;
      tableS10[ 173]= 71;//tableK[ 173]= 870;
      tableS10[ 174]= 70;//tableK[ 174]= 875;
      tableS10[ 175]= 69;//tableK[ 175]= 880;
      tableS10[ 176]= 68;//tableK[ 176]= 885;
      tableS10[ 177]= 67;//tableK[ 177]= 890;
      tableS10[ 178]= 66;//tableK[ 178]= 895;
      tableS10[ 179]= 65;//tableK[ 179]= 900;
      tableS10[ 180]= 64;//tableK[ 180]= 905;
      tableS10[ 181]= 62;//tableK[ 181]= 910;
      tableS10[ 182]= 61;//tableK[ 182]= 915;
      tableS10[ 183]= 60;//tableK[ 183]= 920;
      tableS10[ 184]= 58;//tableK[ 184]= 925;
      tableS10[ 185]= 57;//tableK[ 185]= 930;
      tableS10[ 186]= 56;//tableK[ 186]= 935;
      tableS10[ 187]= 54;//tableK[ 187]= 940;
      tableS10[ 188]= 52;//tableK[ 188]= 945;
      tableS10[ 189]= 51;//tableK[ 189]= 950;
      tableS10[ 190]= 49;//tableK[ 190]= 955;
      tableS10[ 191]= 47;//tableK[ 191]= 960;
      tableS10[ 192]= 45;//tableK[ 192]= 965;
      tableS10[ 193]= 42;//tableK[ 193]= 970;
      tableS10[ 194]= 40;//tableK[ 194]= 975;
      tableS10[ 195]= 37;//tableK[ 195]= 980;
      tableS10[ 196]= 34;//tableK[ 196]= 985;
      tableS10[ 197]= 29;//tableK[ 197]= 990;
      tableS10[ 198]= 24;//tableK[ 198]= 995;
      tableS10[ 199]= 11;//tableK[ 199]= 1000;
    
      // Заполняем таблицу квадратов от 90 - минимаальное значени напряжение при котором работает блок питания контролера до
      // 255 - ну просто максимальное среднеквадратичное значение напряжения в сети.
    #ifdef TESTRM
      DEBUG_SERIAL.println(F("TABLESQRT"));
    #endif
     /* for(byte i=0;i<MAX_TABLE_SQRT;i++)
      {  
        tableSQRT[i]=(int) i*5+125;
        tableSQ[i]=(unsigned int) tableSQRT[i]*tableSQRT[i];
    #ifdef TESTRM
        DEBUG_SERIAL.print((unsigned int)i);
        DEBUG_SERIAL.write(9);
        DEBUG_SERIAL.print((unsigned int)tableSQRT[i]);
        DEBUG_SERIAL.write(9);
        DEBUG_SERIAL.println((unsigned int)tableSQ[i]);
    #endif
      }*/
    #ifdef TESTRM
      DEBUG_SERIAL.println(F("END TABLESQRT"));
    #endif
    
    
    #ifdef TEST
      DEBUG_SERIAL.println(F("TEST MY_SQRT"));
      for(int j=0;j<=600;j++)
      {
        DEBUG_SERIAL.print((unsigned int) j);
        DEBUG_SERIAL.write(9);
        DEBUG_SERIAL.print((unsigned long) j*j );
        DEBUG_SERIAL.write(9);
        DEBUG_SERIAL.print((unsigned int) sqrt((unsigned long) j*j) );
        DEBUG_SERIAL.write(9);
        DEBUG_SERIAL.print(my_sqrt((unsigned long) j*j) );
        DEBUG_SERIAL.write(9);
        DEBUG_SERIAL.println((long)my_sqrt((unsigned long) j*j)- (long) sqrt((unsigned long) j*j));
      }
      DEBUG_SERIAL.println(F("END TEST MY_SQRT"));
    #endif
}
