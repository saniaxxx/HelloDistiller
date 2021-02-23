// Последнее обновление 2018-07-25 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам
// Обработка клавиатуры здесь

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Очень длинная и "нудная" функция обработки клавиатуры в разных режимах
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "configuration.h"
#include "declarations.h"

char KeyCode; // Код нажатой клавиши

#define PRESS_SELECT     (KeyCode==1)  // Phisik: Скобочки здесь нужно всегда ставить, на всякий случай,
#define PRESS_RIGHT      (KeyCode==2)  // т.к. могут быть очень трудноуловимые ошибки
#define PRESS_LEFT       (KeyCode==3)
#define PRESS_UP         (KeyCode==4)
#define PRESS_DOWN       (KeyCode==5)

#define KEYCODE_INIT_LCD	10
#define KEYCODE_NEXT_STAGE  11

void ScanKbd()
{
	int menuFlagNumber;

	char i;
#ifdef DEBUG
	if (DEBUG_SERIAL.available()) return;
#endif 
	// Если режим защиты от дребезга уже выключен
	if (CountKeys == 0)
	{
		flScanKbd = 1; // Установим признак сканирования клавиатуры.
		
		KeyCode = 0; // Reset keycode

		switch (cmdGPRS) {
			case 'R': KeyCode = 2; break;
			case 'U': KeyCode = 4; break;
			case 'D': KeyCode = 5; break;
			case 'L': KeyCode = 3; break;
			case 'S': KeyCode = 1; break;
		}
#ifdef TESTGSM
		DEBUG_SERIAL.print(F("Go cmd="));
		DEBUG_SERIAL.print(cmdGPRS);
		DEBUG_SERIAL.print(F("="));
		DEBUG_SERIAL.println((int)KeyCode);
#endif
		cmdGPRS = 0;  // clear server command

#if USE_LCD_KEYPAD_SHIELD
		// Skip hardware keyboard reading if recieved online command
		if (KeyCode == 0) {
			const int key = analogRead(PIN_LCD_KEYPAD);
			
			if (key >= 0)    KeyCode = 2;	  // 0
      if (key >= 130)  KeyCode = 5;   // 300
			if (key >= 300)   KeyCode = 4;  // 130
			if (key >= 400)  KeyCode = 3;   // 485
			if (key >= 600)  KeyCode = 1;   // 720
			if (key >= 850) KeyCode = 0;   
			
		#if PRINT_ADC_VALUES
			if(KeyCode>0) {
				DEBUG_SERIAL.print("ADC value = ");
				DEBUG_SERIAL.println(key);
			}
		#endif
		}
#else
		if(digitalRead(PIN_SELECT)==0) KeyCode = 1;
		if(digitalRead(PIN_RIGHT)==0)  KeyCode = 2;
		if(digitalRead(PIN_LEFT)==0)   KeyCode = 3;
		if(digitalRead(PIN_UP)==0)     KeyCode = 4;
		if(digitalRead(PIN_DOWN)==0)   KeyCode = 5;

		// Reinit lcd combination
		if (digitalRead(PIN_RIGHT) == 0 && digitalRead(PIN_LEFT) == 0)  KeyCode = KEYCODE_INIT_LCD;
		if (digitalRead(PIN_UP) == 0 && digitalRead(PIN_DOWN) == 0)     KeyCode = KEYCODE_NEXT_STAGE;
#endif

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// PRESS_LEFT && PRESS_RIGHT
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		if (KeyCode == KEYCODE_INIT_LCD)
		{
			if (BeepKeyPress) my_beep(2);

#if USE_I2C_LCD
			// Phisik:  пробуйте по разному, моя библиотека этих строчек не требует
			//lcd.begin(LCD_WIDTH, LCD_HEIGHT);
			//lcd.init();

			lcd.begin();
			lcd.backlight();
#else
			lcd.begin(LCD_WIDTH, LCD_HEIGHT);
#endif // USE_I2C_LCD

			
			lcd.clear();
			DisplayData();
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//  PRESS_UP && PRESS_DOWN
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		// Нажатие кнопок одновременно ввехр и вниз позволяет перейти к следующему этапу (отладочные кнопки)
		if (KeyCode == KEYCODE_NEXT_STAGE)
		{
			if (BeepKeyPress) my_beep(2);
			StateMachine++; // Состояние конечного автомата процесса ректификации
			CountKeys = 10; // Большая защита от дребезга, чтоб случайно не нажали.
			DisplayData();
			timeNBK = 0;
			timeMIXER = 0;
			time1 = 0;
			time2 = 0;
			time3 = 0;

		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//  PRESS_SELECT
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (PRESS_SELECT)
		{
			if (BeepKeyPress) my_beep(1);
			switch (FlState)
			{
			case 0:

				FlState = IspReg;
				break;
			case 100:
				menuFlagNumber = -1;


				// Edited by Phisik on 2017-08-16
				// Обновил алгоритм зацикливания. Теперь не надо задавать LAST_ITEM/FIRST_ITEM
				// Просто расставить  1/0

				// ищем следующий ненулевой индекс в menuFlagNumber
				while (!settingsEnableFlag[++menuFlagNumber])
					if (menuFlagNumber >= SETTINGS_ITEMS) {
						menuFlagNumber = 0;
						break;  // если кругом нули, показываем первый пункт
					}

				FlState = 200 + menuFlagNumber;   // Возвращаемся к стандартной нумерации 


				break;
			case 101:
				FlState = 0;
				IspReg = 101;
				writeEEPROM();
				PrepareProcess(); // Phisik: Убираем все ТЕНы и клапана!!!
				break;
			case 102:
				if (IspReg != 102)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 102;
				writeEEPROM();
				FlState = 200;
				break;
			case 200:
				FlState = 0;
				writeEEPROM();
				break;
			case 103:
				IspReg = 103;
				writeEEPROM();
				FlState = 202;
				break;
			case 201:
				writeEEPROM();
				FlState = 0;
				break;
			case 202:
				writeEEPROM();
				FlState = 0;
				break;
			case 104:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 104 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 104;
				writeEEPROM();
				FlState = 206;
				break;
			case 105:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 105 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 105;
				writeEEPROM();
				FlState = 221;
				break;
			case 106:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 106 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 106;
				writeEEPROM();
				FlState = 207;
				break;
			case 107:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 107 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 107;
				writeEEPROM();
				FlState = 208;
				break;
			case 108:
				if (IspReg != 108 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 108;
				FlState = 259;
				writeEEPROM();
				break;
			case 109:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 109 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;

				IspReg = 109;
				FlState = 209;
				writeEEPROM();
				break;
			case 110:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 110 || !StateMachine) // Если состояние автомата - не запущено, запускаем процесс
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;

				IspReg = 110;
				writeEEPROM();
				FlState = 220;
				break;
			case 111:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 111 || !StateMachine) // Если состояние автомата - не запущено, запускаем процесс
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;

				IspReg = 111;
				writeEEPROM();
				FlState = 209;
				break;
			case 112:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 112 || !StateMachine) // Если состояние автомата - не запущено, запускаем процесс
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;

				IspReg = 112;
				writeEEPROM();
				FlState = 264;
				break;
			case 113:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 113 || !StateMachine) // Если состояние автомата - не запущено, запускаем процесс
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;

				IspReg = 113;
				writeEEPROM();
				FlState = 259;
				break;
			case 114:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 114 || !StateMachine) // Если состояние автомата - не запущено, запускаем процесс
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;

				IspReg = 114;
				writeEEPROM();
				FlState = 261;
				break;
			case 115:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 115 || !StateMachine) // Если состояние автомата - не запущено, запускаем процесс
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;

				IspReg = 115;
				writeEEPROM();
				FlState = 0;
				break;
			case 116:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 116 || !StateMachine) // Если состояние автомата - не запущено, запускаем процесс
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;

				IspReg = 116;
				writeEEPROM();
				FlState = 240;
				break;
			case 117:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 117 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 117;
				writeEEPROM();
				FlState = 257;
				break;
			case 118:
				// Если ранее был выбран другой тип отбора, сбрасываем состояние автомата в начальное.
				if (IspReg != 118 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
					flAlarmMPX5010 = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 118;
				writeEEPROM();
				FlState = 258;
				break;
			case 130:
				if (IspReg != 130 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 130;
				writeEEPROM();
				FlState = 0;
				break;
			case 129:
				if (IspReg != 129 || !StateMachine)
				{
					Seconds = 0;
					flAlarmUroven = 0;
					StateMachine = 0;
				}
				if (!StateMachine) StateMachine = 1;
				IspReg = 129;
				writeEEPROM();
				FlState = 231;
				break;
			case 203:
			case 204:
			case 205:
			case 206:
			case 207:
			case 208:
			case 209:
			case 210:
				//      case 211:             
			case 212:
			case 213:
			case 214:
			case 215:
			case 216:
			case 217:
			case 218:
			case 219:
			case 220:
			case 221:
			case 222:
			case 223:
			case 224:
			case 225:
			case 226:
			case 227:
			case 228:
			case 229:
			case 230:
			case 231:
			case 232:
			case 233:
			case 234:
			case 236:
			case 237:
			case 238:
			case 239:
			case 241:
			case 243:
			case 246:
			case 247:
			case 248:
			case 250:
			case 251:
			case 252:
			case 253:
			case 254:
			case 259:
			case 260:
			case 261:
			case 264:
			case 265:
			case 266:
			case 267:
			case 268:
		#if USE_BMP280_SENSOR
			case 270:
		#endif
				writeEEPROM();
				FlState = 0;
				break;
			case 211:
				FlState = 300;
				nPopr = 0;
				break;
			case 235:
				FlState = 301;
				nPopr = 0;
				flPopr = 0;
				break;
			case 240:
				FlState = 302;
				nPopr = 0;
				flPopr = 0;
				break;
			case 242:
				FlState = 303;
				nPopr = 0;
				flPopr = 0;
				break;
			case 244:
				FlState = 304;
				nPopr = 0;
				flPopr = 0;
				break;
			case 245:
				FlState = 305;
				nPopr = 0;
				flPopr = 0;
				break;
			case 249:
				FlState = 309;
				nPopr = 0;
				flPopr = 0;
				break;
			case 255:
				FlState = 310;
				nPopr = 0;
				flPopr = 0;
				break;
			case 256:
				FlState = 311;
				nPopr = 0;
				flPopr = 0;
				break;
			case 257:
				FlState = 312;
				nPopr = 0;
				flPopr = 0;
				break;
			case 258:
				FlState = 313;
				nPopr = 0;
				flPopr = 0;
				break;
			case 262:
				FlState = 314;
				nPopr = 0;
				flPopr = 0;
				break;
			case 263:
				FlState = 315;
				nPopr = 0;
				flPopr = 0;
				break;
#if ENABLE_SENSOR_SORTING
			case 269:     
				FlState=316;
				nPopr=0;
				break;   
#endif
			case 300:
				writeEEPROM();
				FlState = 211;
				break;
			case 301:
				writeEEPROM();
				FlState = 235;
				break;
			case 302:
				writeEEPROM();
				if (IspReg != 116) FlState = 240;
				else FlState = 0;
				break;
			case 303:
				writeEEPROM();
				FlState = 242;
				break;
			case 304:
				writeEEPROM();
				FlState = 244;
				break;
			case 305:
				writeEEPROM();
				FlState = 245;
				break;
			case 309:
				writeEEPROM();
				FlState = 249;
				break;
			case 310:
				writeEEPROM();
				FlState = 255;
				break;
			case 311:
				writeEEPROM();
				FlState = 256;
				break;
			case 312:
				writeEEPROM();
				if (IspReg != 117) FlState = 257;
				else FlState = 0;
				break;
			case 313:
				writeEEPROM();
				if (IspReg != 118) FlState = 258;
				else FlState = 0;
				break;
			case 314:
				writeEEPROM();
				Power = PowerPhase[0] + PowerPhase[1] + PowerPhase[2];
				FlState = 262;
				break;
			case 315:
				writeEEPROM();
				FlState = 263;
				break;
#if ENABLE_SENSOR_SORTING
			case 316:       
				writeEEPROM();
				FlState=269;
				break;    
#endif
			}

			DisplayData();
			
			if (CountKeys == 0) CountKeys = 6;
			CountState = MENU_DELAY_SEC;

		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//  PRESS_RIGHT
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (PRESS_RIGHT)
		{
			if (BeepKeyPress) my_beep(1);

			int menuFlagNumber = 0;

			switch (FlState)
			{
			case 0:
				
				do {
					if(++DispPage > SCREEN_ITEMS-1)	DispPage = 0;  // зацикливаем поиск
				} while (!screenEnableFlag[DispPage]);


#if !defined(NO_PAGE_BEEP) || NO_PAGE_BEEP!=1
				// Если перешли на третью страницу, то переинициализируем дисплей
				if (DispPage == 4 || DispPage == 10)
				{
					my_beep(BEEP_LONG * 5);
#if USE_I2C_LCD
					// Phisik:  пробуйте по разному, моя библиотека этих строчек не требует
					//lcd.begin(LCD_WIDTH, LCD_HEIGHT);
					//lcd.init();

					lcd.begin();
					lcd.backlight();
#else
					lcd.begin(LCD_WIDTH, LCD_HEIGHT);
#endif // USE_I2C_LCD
					lcd.clear();
					DisplayData();
				}
#endif
				CountKeys = 4;
				break;
			case 100:
			case 101:
			case 102:
			case 103:
			case 104:
			case 105:
			case 106:
			case 107:
			case 108:
			case 109:
			case 110:
			case 111:
			case 112:
			case 113:
			case 114:
			case 115:
			case 116:
			case 117:
			case 118:
				// Edited by Phisik on 2017-08-15
				// Вычисляем текущий индекс в menuEnableFlag[]
				menuFlagNumber = FlState % 100;

				// ищем следующий ненулевой индекс в menuFlagNumber
				while (!menuEnableFlag[++menuFlagNumber] && menuFlagNumber < MENU_ITEMS - 2)
					;  // здесь ничего не делаем, просто ищем флажок

				if (menuFlagNumber >= MENU_ITEMS - 2)
					FlState = 129;   // При необходимости, перескакиваем на тест клапанов
				else
					FlState = 100 + menuFlagNumber;   // Или просто возвращаемся к стандартной нумерации  
				break;
			case 129:
				// Если нужно внешнее управление, переходим к нему
				if (menuEnableFlag[MENU_ITEMS - 1])
					FlState++;
				else // Если нет - идем в начало
					FlState = 100;
				break;
			case 130:
				FlState = 100;
				break;
			case 200:
			case 201:
			case 202:
			case 203:
			case 204:
			case 205:
			case 206:
			case 207:
			case 208:
			case 209:
			case 210:
			case 211:
			case 212:
			case 213:
			case 214:
			case 215:
			case 216:
			case 217:
			case 218:
			case 219:
			case 220:
			case 221:
			case 222:
			case 223:
			case 224:
			case 225:
			case 226:
			case 227:
			case 228:
			case 229:
			case 230:
			case 231:
			case 232:
			case 233:
			case 234:
			case 235:
			case 236:
			case 237:
			case 238:
			case 239:
			case 240:
			case 241:
			case 242:
			case 243:
			case 244:
			case 245:
			case 246:
			case 247:
			case 248:
			case 249:
			case 250:
			case 251:
			case 252:
			case 253:
			case 254:
			case 255:
			case 256:
			case 257:
			case 258:
			case 259:
			case 260:
			case 261:
			case 262:
			case 263:
			case 264:
			case 265:
			case 266:
			case 267:
			case 268:
#if ENABLE_SENSOR_SORTING
			case 269:
#endif
#if USE_BMP280_SENSOR
			case 270:
#endif
				// Edited by Phisik on 2017-08-16
				// Вычисляем текущий индекс в menuEnableFlag[]
				menuFlagNumber = FlState % 200;


				// Edited by Phisik on 2018-07-03
				// Обновил алгоритм зацикливания. Теперь не надо задавать LAST_ITEM/FIRST_ITEM
				// Просто расставить  1/0

				// ищем следующий ненулевой индекс в menuFlagNumber
				do { 
					if (++menuFlagNumber > SETTINGS_ITEMS - 1)
						menuFlagNumber = 0;  // зацикливаем поиск
				} while (!settingsEnableFlag[menuFlagNumber]);

				FlState = 200 + menuFlagNumber;   // Возвращаемся к стандартной нумерации  

				break;

			case 300:
				if (nPopr < MAX_DS1820 - 1) nPopr++;
				break;
			case 301:
				if (flPopr == 0) flPopr = 1;
				else
				{
					if (nPopr < COUNT_CHIM - 1) nPopr++;
					flPopr = 0;

				}
				break;
			case 302:
				if (flPopr == 0) flPopr = 1;
				else
				{
					if (nPopr < MAX_CNT_PAUSE - 1) nPopr++;
					flPopr = 0;

				}
				break;
			case 303:
				if (nPopr < 3) nPopr++;
				break;
			case 304:
				if (nPopr < 9) nPopr++;
				break;
			case 305:
				if (nPopr < 11) nPopr++;
				break;
			case 309:
				if (nPopr < 2) nPopr++;
				break;
			case 310:
			case 311:
				if (nPopr < 10) nPopr++;
				break;
			case 312:
				if (flPopr < 2) flPopr++;
				else
				{
					if (nPopr < MAX_CNT_FRACTION_DIST) nPopr++;
					flPopr = 0;
				}
				break;
			case 313:
				if (flPopr < 1) flPopr++;
				else
				{
					if (nPopr < MAX_CNT_FRACTION_RECT) nPopr++;
					flPopr = 0;
				}
				break;
			case 314:
			case 315:
				if (nPopr < 2) nPopr++;
				break;
#if ENABLE_SENSOR_SORTING
			case 316:
				if (nPopr<MAX_DS1820-1) nPopr++;
				break;
#endif

			}

			//NeedDisplaying=1;
			DisplayData();
			if (CountKeys == 0) CountKeys = 6;

			CountState = MENU_DELAY_SEC;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//  PRESS_LEFT
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////   

		if (PRESS_LEFT)
		{
			if (BeepKeyPress) my_beep(1);

			int menuFlagNumber = 0;
			switch (FlState)
			{

			case 0:

				do {
					if(--DispPage < 0) 	DispPage = SCREEN_ITEMS-1;  // зацикливаем поиск
				} while (!screenEnableFlag[DispPage]);
				
#if !defined(NO_PAGE_BEEP) || NO_PAGE_BEEP!=1
				if (DispPage==0){
					// Если на нулевой странице нажали кнопку влево, то тоже переинициализируем дисплей. 
					my_beep(BEEP_LONG * 2);

#if USE_I2C_LCD
					// Phisik:  пробуйте по разному, моя библиотека этих строчек не требует
					//lcd.begin(LCD_WIDTH, LCD_HEIGHT);
					//lcd.init();

					lcd.begin();
					lcd.backlight();
#else
					lcd.begin(LCD_WIDTH, LCD_HEIGHT);
#endif // USE_I2C_LCD
					lcd.clear();
					DisplayData();
				}
#endif
				CountKeys = 4;
				break;
			case 100:
				if (menuEnableFlag[MENU_ITEMS - 1])
					FlState = 130;  // идем на внешнее управление
				else
					FlState = 129;  // или сразу на тест клапанов
				break;
			case 101:
			case 102:
			case 103:
			case 104:
			case 105:
			case 106:
			case 107:
			case 108:
			case 109:
			case 110:
			case 111:
			case 112:
			case 113:
			case 114:
			case 115:
			case 116:
			case 117:
			case 118:
			case 129:
				// Edited by Phisik on 2017-08-15

				// Вычисляем текущий индекс в menuEnableFlag[]
				menuFlagNumber = FlState % 100;

				// Тут затычка из-за непоследовательной нумерации пунктов
				if (menuFlagNumber == 29) menuFlagNumber = MENU_ITEMS - 2;

				// Ищем предыдущий ненулевой индекс в menuFlagNumber
				while (!menuEnableFlag[--menuFlagNumber] && menuFlagNumber > 0)
					;  // здесь ничего не делаем, просто ищем флажок

				 // Возвращаемся к стандартной нумерации 
				FlState = 100 + menuFlagNumber;

				break;
			case 130:
				FlState--;
				break;

			case 200:
			case 201:
			case 202:
			case 203:
			case 204:
			case 205:
			case 206:
			case 207:
			case 208:
			case 209:
			case 210:
			case 211:
			case 212:
			case 213:
			case 214:
			case 215:
			case 216:
			case 217:
			case 218:
			case 219:
			case 220:
			case 221:
			case 222:
			case 223:
			case 224:
			case 225:
			case 226:
			case 227:
			case 228:
			case 229:
			case 230:
			case 231:
			case 232:
			case 233:
			case 234:
			case 235:
			case 236:
			case 237:
			case 238:
			case 239:
			case 240:
			case 241:
			case 242:
			case 243:
			case 244:
			case 245:
			case 246:
			case 247:
			case 248:
			case 249:
			case 250:
			case 251:
			case 252:
			case 253:
			case 254:
			case 255:
			case 256:
			case 257:
			case 258:
			case 259:
			case 260:
			case 261:
			case 262:
			case 263:
			case 264:
			case 265:
			case 266:
			case 267:
			case 268:
		#if ENABLE_SENSOR_SORTING
			case 269:
		#endif
		#if ENABLE_SENSOR_SORTING
			case 270:
		#endif
				// Edited by Phisik on 2017-08-16
				// Вычисляем текущий индекс в menuEnableFlag[]
				menuFlagNumber = FlState % 200;

				// ищем следующий ненулевой индекс в menuFlagNumber
				do { 
					if (--menuFlagNumber < 0)
						menuFlagNumber = SETTINGS_ITEMS-1;  // зацикливаем поиск
				} while (!settingsEnableFlag[menuFlagNumber]);

				FlState = 200 + menuFlagNumber;   // Возвращаемся к стандартной нумерации  

				break;
			case 300:
				if (nPopr > 0) nPopr--;
				break;
			case 301:
				if (flPopr == 1) flPopr = 0;
				else
				{
					if (nPopr > 0) nPopr--;
					flPopr = 1;
				}
				break;
			case 302:
				if (flPopr == 1) flPopr = 0;
				else
				{
					if (nPopr > 0) nPopr--;
					flPopr = 1;
				}
				break;
			case 303:
			case 304:
			case 305:
			case 309:
			case 310:
			case 311:
			case 314:
			case 315:
				if (nPopr > 0) nPopr--;
				break;
			case 312:
				if (flPopr > 0) flPopr--;
				else
				{
					if (nPopr > 0) nPopr--;
					flPopr = 2;
				}
				break;
			case 313:
				if (flPopr > 0) flPopr--;
				else
				{
					if (nPopr > 0) nPopr--;
					flPopr = 1;
				}
				break;
#if ENABLE_SENSOR_SORTING
			case 316:
				if (nPopr>0) nPopr--;
				break;
#endif
		}

			//NeedDisplaying=1;
			DisplayData();
			flNeedAnalyse = 1;
			CountState = MENU_DELAY_SEC;
			if (CountKeys == 0) CountKeys = 6;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  PRESS_UP
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (PRESS_UP)
		{
			if (BeepKeyPress) my_beep(1);
			CountState = 0;
			switch (FlState)
			{
			case 0:
				// В сотоянии 0 обработка клавиш происходит за выходом модели этого конечного автомата.
				if (DispPage == 7) // На 8 странице вручную можно состояние процесса поменять
				{
					StateMachine++;
					break;
				}

				if (DispPage == 5) // На 6 странице можно мощность внучную привбавить
				{
					UstPower += 10;
					break;
				}
			#if USE_BMP280_SENSOR
				if (DispPage==4) // На 4 странице можно вручную изменить давление
				{
					PressAtm++;
					CountKeys=1;
					break;
				}
			#endif // USE_BMP280_SENSOR				

				// Ректификация
				if (IspReg == 109 || IspReg == 118)
				{
					switch (StateMachine)
					{
					case 2:
						tEndRectRazgon++;
						CountKeys = 1;
						break;
					case 3:
						PowerRect += 10;
						CountKeys = 2;
						break;
					case 4:
						if (UrovenProvodimostSR == 0) tEndRectOtbGlv++;
						else SecTempPrev += 600;


						break;

					case 5:
						tStabSR++;
#if ADJUST_COLUMN_STAB_TEMP
						lastStableT = 0;	// Обнуляем нашу температуру, чтобы в Case 6 подхватило текущую
						SecTempPrev2 = 0;   // Обнуляем счетчик времени
#endif // ADJUST_COLUMN_STAB_TEMP
						break;
					case 6:
						ProcChimSR++;
						break;
					case 7:
						tEndRect++;
						break;
					}
				}

				// Регулятор мощности
				if (IspReg == 103)
				{
					if (SlaveON == 0)
					{
						if (DispPage == 1)
						{
							// На второй странице нажатик кнопки "вверх" приводи к авто-детекции мощности ТЭНа
							flAutoDetectPowerTEN = 1;
						}
						else
						{
							UstPowerReg += 10;
							flAutoDetectPowerTEN = 0;
						}
					}
					if (SlaveON == 2) PowerGlvDistil += 10;
					if (SlaveON == 3) PowerRect += 10;
					if (SlaveON == 4) PowerDistil += 10;

					//TekPower=(unsigned long) UstPower*220*220/ Power;
					CountKeys = 1;
				}

				if (IspReg == 104)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil++;
					else
					{
						if (DispPage == 0)  Temp1P++;
						else  PowerDistil += 10;
					}
				}
				if (IspReg == 106)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil++;
					else
					{
						if (DispPage == 0)  Temp2P++;
						else PowerDistil += 10;
					}
				}

				if (IspReg == 107)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil++;
					else
					{
						if (DispPage == 0)  Temp3P++;
						else  PowerDistil += 10;
					}
				}

				if (IspReg == 117)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil++;
					else
					{
						if (TekFraction < CountFractionDist)
						{
							if (DispPage == 0)  TempFractionDist[TekFraction]++;
							else PowerFractionDist[TekFraction] += 10;
						}
					}
				}

				if (IspReg == 105)
				{
					CountKeys = 1;
					if (StateMachine == 3) PowerGlvDistil += 10;
					if (StateMachine == 2) TempDeflBegDistil++;
				}

				if (IspReg == 114)
				{
					TempHLDZatorBrog1++;
				}

				if (IspReg == 129)
				{
					CloseAllKLP();
					ProcChimSR++;
				}

				if (IspReg == 115)
				{
					CountKeys = 2;
					if (DispPage == 0) timerMinute += AddPressKeys;
					else  PowerMinute += AddPressKeys10;
				}

				if (IspReg == 116)
				{
					if (DispPage == 0 && StateMachine == 3 && timeP[KlTek] == 0) timeP[KlTek] = 255;
					else
					{
						// На второй странице можно включить насос.
						if (DispPage == 1)
						{
							time1 = 0;
							KlOpen[KLP_HLD] = 0;
						}
					}
				}

				if (IspReg == 108 || IspReg == 113)
				{
					CountKeys = 1;
					switch (StateMachine)
					{
					case 0: //Не запущено
						time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
						timeMIXER = (int)tempP[0] * 60;
						break;
					case 1: //Нагрев до температуры 50 градусов
						if (DispPage == 0) TempZSP++;
						else
						{
							time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
							timeMIXER = (int)tempP[0] * 60;
						}
						break;
					case 3: //Нагрев до температуры 62 градуса
						TempZSPSld++;
						break;
					case 4: //Ожидание 15 минут, поддержка температуры
						time2 += 60;
					case 2: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
					case 8: //ожидание засыпки солода, поддержка температуры
					  // На этих этапах нажатие кнопки вверх включает мешалку, вниз - отключает.
						time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
						timeMIXER = (int)tempP[0] * 60;
						break;

					case 6: //Варка
					case 9: //Размешивание после засыпания солшода
					case 10: // Осахаривание
						if (DispPage == 0) time2 += 60;
						else TempZSPSld++;
						break;
					case 5: //  Нагрев до закипания
						TempKipenZator++;
						break;
					case 7: //охлаждение до температуры осахаривания.
						TempZSPSld++;
						break;
					case 12: //охлаждение до температуры осахаривания.
						TempHLDZatorBrog1++;
						break;
					case 14: //охлаждение до температуры осахаривания.
					case 13: //охлаждение до температуры осахаривания.
						if (DispPage == 0) TempHLDZatorBrog1++;
						else
						{
							time1 = ((int)tempP[0] + (int)timeP[0]) * 60; // Взводим насос на десять минут работы
							timeMIXER = (int)tempP[0] * 60;
						}
						break;
					case 100: //Окончание
						break;
					}
				}

				if (IspReg == 110)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil++;
					else
						if (DispPage == 0) TempDefl++;
						else deltaPower += 10;
				}

				// НДРФ
				if (IspReg == 111)
				{
					CountKeys = 2;
					switch (StateMachine)
					{
					case 2:
						tEndRectRazgon++;
						CountKeys = 1;
						break;
					case 3:
						if (DispPage == 0) SecOstatok += 60;
						else PowerRect += 10;
						CountKeys = 3;
						break;
					case 4:
						tEndRectOtbGlv++;
						break;
					case 5:
						tStabSR++;
#if ADJUST_COLUMN_STAB_TEMP
						lastStableT = 0;	// Обнуляем нашу температуру, чтобы в Case 6 подхватило текущую
						SecTempPrev2 = 0;   // Обнуляем счетчик времени
#endif // ADJUST_COLUMN_STAB_TEMP
						break;
					case 6:
						ProcChimSR++;
						break;
					case 7:
						tEndRect++;
						break;
					}
				}
				// NBK
				if (IspReg == 112)
				{
					CountKeys = 1;
					if (DispPage == 0)
					{
						if (StateMachine == 3) StateMachine = 4;
						else
						{
							if (StateMachine == 5) time2 += 60;
							else
							{
								SpeedNBKDst += 1;
								SpeedNBK = SpeedNBKDst;
								CountKeys = 1;
							}
						}
					}
					else
					{
						PowerNBK += AddPressKeys10;
						UstPower = PowerNBK;
					}
				}
				if (IspReg == 130) {
					Angle += 5;
					SetAngle(Angle);
				}

				break;
			case 200:
				TempTerm += AddPressKeys;
				CountKeys = 2;
				break;
			case 201:
				Power += AddPressKeys10;
				PowerPhase[0] = Power;
				CountKeys = 3;
				break;
			case 202:
				UstPowerReg += AddPressKeys10;
				CountKeys = 3;
				break;
			case 203:
				FlToUSART = 1;
				break;
			case 204:

				if (FlToGSM < 20) FlToGSM++;
				// Переинициализируем сотовый
				break;
			case 205:
				Delta++;
				CountKeys = 3;
				break;
			case 206:
				Temp1P += AddPressKeys;
				CountKeys = 2;
				break;
			case 207:
				Temp2P += AddPressKeys;
				CountKeys = 2;
				break;
			case 208:
				Temp3P += AddPressKeys;
				CountKeys = 2;
				break;
			case 209:
				tEndRectRazgon += AddPressKeys;
				CountKeys = 3;
				break;
			case 210:
				PowerRect += AddPressKeys10;
				CountKeys = 3;
				break;
			case 212:
				tEndRectOtbGlv++;
				CountKeys = 3;
				break;
			case 213:
				timeChimRectOtbGlv += AddPressKeys10;
				CountKeys = 3;
				break;
			case 214:
				ProcChimOtbGlv++;
				CountKeys = 4;
				break;
			case 250:
				minProcChimOtbSR++;
				CountKeys = 4;
				break;
			case 251:
				tStabSR++;
#if ADJUST_COLUMN_STAB_TEMP
				lastStableT = 0;	// Обнуляем нашу температуру, чтобы в Case 6 подхватило текущую
				SecTempPrev2 = 0;   // Обнуляем счетчик времени
#endif // ADJUST_COLUMN_STAB_TEMP
				CountKeys = 3;
				break;
			case 252:
				begProcChimOtbSR++;
				CountKeys = 4;
				break;
			case 253:
				P_MPX5010++;
				CountKeys = 4;
				break;
			case 254:
				PowerNBK += AddPressKeys10;
				CountKeys = 3;
				break;
			case 215:
				timeChimRectOtbSR += AddPressKeys10;
				CountKeys = 3;
				break;
			case 216:
				tDeltaRect++;
				CountKeys = 4;
				break;
			case 217:
				tEndRectOtbSR++;
				CountKeys = 3;
				break;
			case 218:
				tEndRect++;
				CountKeys = 3;
				break;
			case 219:
				PowerGlvDistil += AddPressKeys10;
				CountKeys = 3;
				break;
			case 220:
				PowerDistil += AddPressKeys10;
				CountKeys = 3;
				break;
			case 221:
				TempDeflBegDistil += AddPressKeys;
				CountKeys = 3;
				break;
			case 222:
				TempDefl++;
				CountKeys = 3;
				break;
			case 223:
				DeltaDefl++;
				CountKeys = 3;
				break;
			case 224:
				tEndDistDefl++;
				CountKeys = 3;
				break;
			case 225:
				BeepEndProcess = 1;
				break;
			case 226:
				BeepStateProcess++;
				CountKeys = 5;
				break;
			case 227:
				BeepKeyPress = 1;
				break;
			case 228:
				PowerRazvZerno += AddPressKeys10;
				CountKeys = 3;
				break;
			case 229:
				PowerVarkaZerno += AddPressKeys10;
				CountKeys = 3;
				break;
			case 230:
				PeriodRefrServer += 5;
				CountKeys = 3;
				break;
			case 231:
				NaprPeregrev += 5;
				CountKeys = 3;
				break;
			case 232:
				UrovenBarda += 5;
				CountKeys = 3;
				break;
			case 233:
				UrovenProvodimostSR++;
				CountKeys = 3;
				break;
			case 234:
				TimeStabKolonna += 60;
				CountKeys = 3;
				break;
			case 235:
				CntCHIM++;
				CountKeys = 3;
				break;
			case 236:
				DecrementCHIM++;
				CountKeys = 3;
				break;
			case 237:
				IncrementCHIM++;
				CountKeys = 3;
				break;
			case 238:
				TimeAutoIncCHIM += 60;
				CountKeys = 3;
				break;
			case 239:
				TimeRestabKolonna += 60;
				CountKeys = 3;
				break;
			case 240:
				if (CntPause < MAX_CNT_PAUSE - 1) CntPause++;
				CountKeys = 3;
				break;
			case 241:
				CorrectASC712++;
				CountKeys = 3;
				break;
			case 243:
				ipPort += AddPressKeys;
				CountKeys = 2;
				break;
			case 246:
				AlarmMPX5010 += AddPressKeys;
				CountKeys = 2;
				break;
			case 247:
				FlAvtonom++;
				CountKeys = 5;
				break;
			case 248:
				timeOpenBRD++;
				CountKeys = 2;
				break;
			case 249:
				PIDTemp[0] += AddPressKeys;
				CountKeys = 2;
				break;
			case 257:
				if (CountFractionDist < MAX_CNT_FRACTION_DIST) CountFractionDist++;
				CountKeys = 3;
				break;
			case 258:
				if (CountFractionRect < MAX_CNT_FRACTION_RECT) CountFractionRect++;
				CountKeys = 3;
				break;
			case 259:
				TempZSP++;
				CountKeys = 3;
				break;
			case 260:
				TempZSPSld++;
				CountKeys = 3;
				break;
			case 261:
				TempHLDZatorBrog1++;
				CountKeys = 3;
				break;
			case 262:
				Power += AddPressKeys10;
				CountKeys = 3;
				break;
			case 264:
				minPressNBK++;
				CountKeys = 1;
				break;
			case 265:
				deltaPressNBK++;
				CountKeys = 1;
				break;
			case 266:
				timePressNBK++;
				CountKeys = 1;
				break;
			case 267:
				UprNasosNBK++;
				CountKeys = 3;
				break;
			case 268:
				ProcChimOtbCP++;
				CountKeys = 1;
				break;
		#if USE_BMP280_SENSOR
			case 270:
				if (timePressAtm<250) 
					timePressAtm+=5;
				else 
					timePressAtm=250;

				CountKeys=2;
				break;
		#endif // USE_BMP280_SENSOR

			case 300:
				ds1820_popr[nPopr]++;
				CountState = 8; // При вводе поправок сохраняем состояние 4 секунды, а не две, тут спешка не нужна.                
				CountKeys = 4;
				break;
			case 301:
				if (flPopr == 0) {
					tempK[nPopr] += AddPressKeys;
					CountKeys = 1;
				}
				else CHIM[nPopr]++;
				break;
			case 302:
				if (flPopr == 0) tempP[nPopr]++;
				else
				{
					timeP[nPopr]++;
					if (timeP[nPopr] == 255) timeP[nPopr] = 0;
				}
				CountKeys = 1;
				break;
			case 303:
				ip[nPopr]++;
				CountKeys = 1;
				break;
			case 304:
				idDevice[nPopr]++;
				CountKeys = 1;
				break;
			case 305:
				my_phone[nPopr]++;
				CountKeys = 1;
				break;
			case 309:
				PIDTemp[nPopr] += AddPressKeys;
				CountKeys = 2;
				break;
			case 310:
				WiFiAP[nPopr]++;
				CountKeys = 1;
				break;
			case 311:
				WiFiPass[nPopr]++;
				CountKeys = 1;
				break;
			case 312:
				if (nPopr == MAX_CNT_FRACTION_DIST)
				{

					if (TekFraction < MAX_CNT_FRACTION_DIST - 1)
					{
						TekFraction++;
						SecTempPrev = Seconds;
						SetAngle(AngleFractionDist[TekFraction]);
					}
					CountKeys = 3;

				}
				else
				{
					CountKeys = 2;
					if (flPopr == 0) TempFractionDist[nPopr] += AddPressKeys;
					if (flPopr == 2) PowerFractionDist[nPopr] += AddPressKeys10;
					if (flPopr == 1) {
						AngleFractionDist[nPopr]++;
						CountKeys = 1;
					}
				}
				break;
			case 313:
				if (nPopr == MAX_CNT_FRACTION_RECT)
				{

					if (TekFraction < MAX_CNT_FRACTION_RECT - 1)
					{
						TekFraction++;
						SecTempPrev1 = Seconds;
						SetAngle(AngleFractionRect[TekFraction]);
					}
					CountKeys = 3;
				}
				else
				{
					CountKeys = 2;
					if (flPopr == 0) TempFractionRect[nPopr] += AddPressKeys;
					if (flPopr == 1) {
						AngleFractionRect[nPopr]++;
						CountKeys = 1;
					}
				}
				break;
			case 314:
				PowerPhase[nPopr] += AddPressKeys10;
				CountKeys = 2;
				break;
			case 315:
				KtPhase[nPopr] += AddPressKeys;
				CountKeys = 2;
				break;
#if ENABLE_SENSOR_SORTING
			case 316:
				if (ds1820_nums[nPopr]<MAX_DS1820-1) ds1820_nums[nPopr]++;
				CountKeys=4;
				break;   
#endif
			}

			//NeedDisplaying=1;
			DisplayData();
			flNeedAnalyse = 1;
			if (CountState == 0) CountState = 4;
			if (CountKeys == 0) CountKeys = 6;
			CountState = MENU_DELAY_SEC;
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//  PRESS_DOWN
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (PRESS_DOWN)
		{
			if (BeepKeyPress) my_beep(1);
			CountState = 0;
			switch (FlState)
			{
			case 0:

				// В сотоянии 0 обработка клавиш происходит за выходом модели этого конечного автомата.
				if (DispPage == 2) // На третьей странице сбрасываем датчик потока
				{
					break;
				}

				if (DispPage == 7) // На 8 странице можно состояние процесс поменять.
				{
					StateMachine--;
					break;
				}

				if (DispPage == 5) // На 6 странице можно мощность вручную изменить.
				{
					UstPower -= 10;
					break;
				}

			#if USE_BMP280_SENSOR
				if (DispPage==4) // На 4 странице можно вручную изменить давление
				{
					PressAtm--;
					CountKeys=1;
					break;
				}
			#endif // USE_BMP280_SENSOR


				// Ректификация
				if (IspReg == 109 || IspReg == 118)
				{
					switch (StateMachine)
					{
					case 2:
						tEndRectRazgon--;
						CountKeys = 1;
						break;
					case 3:
						PowerRect -= 10;
						CountKeys = 3;
						break;
					case 4:
						if (UrovenProvodimostSR == 0) tEndRectOtbGlv--;
						else SecTempPrev -= 600;
						break;
					case 5:
						tStabSR--;
#if ADJUST_COLUMN_STAB_TEMP
						lastStableT = 0;	// Обнуляем нашу температуру, чтобы в Case 6 подхватило текущую
						SecTempPrev2 = 0;   // Обнуляем счетчик времени
#endif // ADJUST_COLUMN_STAB_TEMP
						break;
					case 6:
						ProcChimSR--;
						break;
					case 7:
						tEndRect--;
						break;
					}
				}

				// Регулятор мощности
				if (IspReg == 103)
				{
					if (SlaveON == 0) UstPowerReg -= 10;
					if (SlaveON == 2) PowerGlvDistil -= 10;
					if (SlaveON == 3) PowerRect -= 10;
					if (SlaveON == 4) PowerDistil -= 10;

					//TekPower=(unsigned long) UstPower*220*220/ Power;
					CountKeys = 1;
				}
				if (IspReg == 104)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil--;
					else
						if (DispPage == 0)  Temp1P--;
						else  PowerDistil -= 10;
				}
				if (IspReg == 106)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil--;
					else
						if (DispPage == 0)  Temp2P--;
						else  PowerDistil -= 10;
				}

				if (IspReg == 107)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil--;
					else
						if (DispPage == 0)  Temp3P--;
						else  PowerDistil -= 10;
				}

				if (IspReg == 117)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil--;
					else
					{
						if (TekFraction < CountFractionDist)
						{
							if (DispPage == 0)  TempFractionDist[TekFraction]--;
							else PowerFractionDist[TekFraction] -= 10;
						}
					}
				}

				if (IspReg == 105)
				{
					CountKeys = 1;
					if (StateMachine == 3) PowerGlvDistil -= 10;
					if (StateMachine == 2) TempDeflBegDistil--;
				}

				if (IspReg == 114)
				{
					CountKeys = 1;
					TempHLDZatorBrog1--;
				}

				if (IspReg == 129)
				{
					CloseAllKLP();
					ProcChimSR--;
				}

				if (IspReg == 115)
				{
					CountKeys = 2;
					if (DispPage == 0) timerMinute -= AddPressKeys;
					else  PowerMinute -= AddPressKeys10;
				}
				if (IspReg == 108 || IspReg == 113)
				{
					CountKeys = 1;
					switch (StateMachine)
					{
					case 0: //Не запущено
						timeMIXER = 0;
						time1 = 0;
						break;
					case 1: //Нагрев до температуры 50 градусов
						if (DispPage == 0) TempZSP--;
						else timeMIXER = 0;
						break;
					case 2: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
					  // Переход к следующему этапу
						StateMachine = 3;
						CountKeys = 3;
						break;
					case 3: //Нагрев до температуры 62 градуса
						TempZSPSld--;
						break;
					case 4: //Ожидание 15 минут, поддержка температуры
						time2 -= 60;
						// На этих этапах нажатие кнопки вверх включает мешалку, вниз - отключает.
						timeMIXER = 0;
						break;

					case 6: //Варка
					case 9: //Размешивание после засыпания солшода
					case 10: // Осахаривание
						if (DispPage == 0)
						{
							if (time2 >= 65) time2 -= 60;
							else time2 = 0;

						}
						else TempZSPSld--;

						break;
					case 5:
						TempKipenZator--;
						break;
					case 12: //охлаждение до температуры осахаривания.
						TempHLDZatorBrog1--;
						break;
					case 14: //охлаждение до температуры осахаривания.
					case 13: //охлаждение до температуры осахаривания.
						if (DispPage == 0) TempHLDZatorBrog1--;
						else
						{
							timeMIXER = 0;
						}
						break;

					case 8:
						// Нажатие кнопки вниз переводит на следующий этап 
						StateMachine = 9;
						CountKeys = 6;
						break;
					case 7: //охлаждение до температуры осахаривания.
						TempZSPSld--;
						break;
					case 100: //Окончание
						break;
					}
				}

				if (IspReg == 110)
				{
					CountKeys = 1;
					if (StateMachine == 2)
						TempDeflBegDistil--;
					else
						if (DispPage == 0) TempDefl--;
						else deltaPower -= 10;

				}

				// НДРФ
				if (IspReg == 111)
				{
					CountKeys = 2;
					switch (StateMachine)
					{
					case 2:
						tEndRectRazgon--;
						CountKeys = 1;
						break;
					case 3:

						if (DispPage == 0) SecOstatok -= 60;
						else PowerRect -= 10;
						CountKeys = 3;
						break;
					case 4:
						tEndRectOtbGlv--;
						break;
					case 5:
						tStabSR--;
#if ADJUST_COLUMN_STAB_TEMP
						lastStableT = 0;	// Обнуляем нашу температуру, чтобы в Case 6 подхватило текущую
						SecTempPrev2 = 0;   // Обнуляем счетчик времени
#endif // ADJUST_COLUMN_STAB_TEMP
						break;
					case 6:
						ProcChimSR--;
						break;
					case 7:
						tEndRect--;
						break;
					}
				}
				// NBK
				if (IspReg == 112)
				{
					CountKeys = 1;
					if (DispPage == 0)
					{
						if (StateMachine == 5) time2 -= 60;
						else
						{
							SpeedNBKDst -= 1;
							SpeedNBK = SpeedNBKDst;
						}
					}
					else
					{
						PowerNBK -= AddPressKeys10;
						UstPower = PowerNBK;

					}
				}

				if (IspReg == 116)
				{
					// На второй странице можно выключить насос.
					if (DispPage == 1)
					{
						time1 = 0;
						KlOpen[KLP_HLD] = 40;
					}
				}

				if (IspReg == 130) {
					Angle -= 5;
					SetAngle(Angle);
				}

				break;
			case 200:
				TempTerm -= AddPressKeys;
				CountKeys = 2;
				break;
			case 201:
				Power -= AddPressKeys10;
				PowerPhase[0] = Power;
				CountKeys = 3;
				break;
			case 202:
				UstPowerReg -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 203:
				FlToUSART = 0;
				break;
			case 204:
				if (FlToGSM > 0) FlToGSM--;
				break;
			case 205:
				Delta--;
				CountKeys = 3;
				break;
			case 206:
				Temp1P -= AddPressKeys;
				CountKeys = 2;
				break;
			case 207:
				Temp2P -= AddPressKeys;
				CountKeys = 2;
				break;
			case 208:
				Temp3P -= AddPressKeys;
				CountKeys = 2;
				break;
			case 209:
				tEndRectRazgon -= AddPressKeys;
				CountKeys = 3;
				break;
			case 210:
				PowerRect -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 212:
				tEndRectOtbGlv--;
				CountKeys = 3;
				break;
			case 213:
				timeChimRectOtbGlv -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 214:
				ProcChimOtbGlv--;
				CountKeys = 4;
				break;
			case 250:
				minProcChimOtbSR--;
				CountKeys = 4;
				break;
			case 251:
				tStabSR--;
#if ADJUST_COLUMN_STAB_TEMP
				lastStableT = 0;	// Обнуляем нашу температуру, чтобы в Case 6 подхватило текущую
				SecTempPrev2 = 0;   // Обнуляем счетчик времени
#endif // ADJUST_COLUMN_STAB_TEMP
				CountKeys = 3;
				break;
			case 252:
				begProcChimOtbSR--;
				CountKeys = 4;
				break;
			case 253:
				P_MPX5010--;
				CountKeys = 4;
				break;
			case 254:
				PowerNBK -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 215:
				timeChimRectOtbSR -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 216:
				tDeltaRect--;
				CountKeys = 4;
				break;
			case 217:
				tEndRectOtbSR--;
				CountKeys = 3;
				break;
			case 218:
				tEndRect--;
				CountKeys = 3;
				break;
			case 219:
				PowerGlvDistil -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 220:
				PowerDistil -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 221:
				TempDeflBegDistil -= AddPressKeys;
				CountKeys = 3;
				break;
			case 222:
				TempDefl--;
				CountKeys = 3;
				break;
			case 223:
				DeltaDefl--;
				CountKeys = 3;
				break;
			case 224:
				tEndDistDefl--;
				CountKeys = 3;
				break;
			case 225:
				BeepEndProcess = 0;
				break;
			case 226:
				BeepStateProcess = 0;
				break;
			case 227:
				BeepKeyPress = 0;
				break;
			case 228:
				PowerRazvZerno -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 229:
				if (PowerVarkaZerno > 10) PowerVarkaZerno -= AddPressKeys10;
				else PowerVarkaZerno = 0;

				CountKeys = 3;
				break;
			case 230:
				PeriodRefrServer -= 5;
				CountKeys = 3;
				break;
			case 231:
				NaprPeregrev -= 5;
				CountKeys = 3;
				break;
			case 232:
				UrovenBarda -= 5;
				CountKeys = 3;
				break;
			case 233:
				UrovenProvodimostSR--;
				CountKeys = 3;
				break;
			case 234:
				TimeStabKolonna -= 60;
				CountKeys = 3;
				break;
			case 235:
				CntCHIM--;
				CountKeys = 3;
				break;
			case 236:
				DecrementCHIM--;
				CountKeys = 3;
				break;
			case 237:
				IncrementCHIM--;
				CountKeys = 3;
				break;
			case 238:
				TimeAutoIncCHIM -= 60;
				CountKeys = 3;
				break;
			case 239:
				TimeRestabKolonna -= 60;
				CountKeys = 3;
				break;
			case 240:
				if (CntPause >= 0) CntPause--;
				CountKeys = 3;
				break;
			case 241:
				CorrectASC712--;
				CountKeys = 3;
				break;
			case 243:
				ipPort -= AddPressKeys;
				CountKeys = 2;
				break;
			case 246:
				AlarmMPX5010 -= AddPressKeys;
				CountKeys = 2;
				break;
			case 247:
				FlAvtonom--;
				if (FlAvtonom < 0) FlAvtonom = 0;
				CountKeys = 5;
				break;
			case 248:
				timeOpenBRD--;
				CountKeys = 2;
				break;
			case 249:
				PIDTemp[0] -= AddPressKeys;
				CountKeys = 2;
				break;
			case 257:
				if (CountFractionDist >= 0) CountFractionDist--;
				CountKeys = 3;
				break;
			case 258:
				if (CountFractionRect >= 0) CountFractionRect--;
				CountKeys = 3;
				break;
			case 259:
				TempZSP--;
				CountKeys = 3;
				break;
			case 260:
				TempZSPSld--;
				CountKeys = 3;
				break;
			case 261:
				TempHLDZatorBrog1--;
				CountKeys = 3;
				break;
			case 262:
				Power -= AddPressKeys10;
				CountKeys = 3;
				break;
			case 264:
				minPressNBK--;
				CountKeys = 1;
				break;
			case 265:
				deltaPressNBK--;
				CountKeys = 1;
				break;
			case 266:
				timePressNBK--;
				CountKeys = 1;
				break;
			case 267:
				UprNasosNBK--;
				CountKeys = 3;
				break;
			case 268:
				ProcChimOtbCP--;
				CountKeys = 1;
				break;
		#if USE_BMP280_SENSOR
			case 270:
				if (timePressAtm>5) timePressAtm-=5;
				else timePressAtm=0;
				CountKeys=2;
				break;
		#endif // USE_BMP280_SENSOR

			case 300:
				ds1820_popr[nPopr]--;
				CountState = 8; // При вводе поправок сохраняем состояние 4 секунды, а не две, тут спешка не нужна.                
				CountKeys = 4;
				break;
			case 301:
				if (flPopr == 0) {
					tempK[nPopr] -= AddPressKeys;
					CountKeys = 1;
				}
				else CHIM[nPopr]--;
				break;
			case 302:
				if (flPopr == 0) tempP[nPopr]--;
				else
				{
					timeP[nPopr]--;
					if (timeP[nPopr] == 255) timeP[nPopr] = 254;
				}
				CountKeys = 1;
				break;
			case 303:
				ip[nPopr]--;
				CountKeys = 1;
				break;
			case 304:
				idDevice[nPopr]--;
				CountKeys = 1;
				break;
			case 305:
				my_phone[nPopr]--;
				CountKeys = 1;
				break;
			case 309:
				PIDTemp[nPopr] -= AddPressKeys;
				CountKeys = 2;
				break;
			case 310:
				WiFiAP[nPopr]--;
				CountKeys = 1;
				break;
			case 311:
				WiFiPass[nPopr]--;
				CountKeys = 1;
				break;
			case 312:
				if (nPopr == MAX_CNT_FRACTION_DIST)
				{

					if (TekFraction > 0)
					{
						TekFraction--;
						SecTempPrev = Seconds;
						SetAngle(AngleFractionDist[TekFraction]);
					}
					CountKeys = 3;

				}
				else
				{
					CountKeys = 2;
					if (flPopr == 0) TempFractionDist[nPopr] -= AddPressKeys;
					if (flPopr == 2) PowerFractionDist[nPopr] -= AddPressKeys10;
					if (flPopr == 1) {
						AngleFractionDist[nPopr]--;
						CountKeys = 1;
					}
				}
				break;
			case 313:
				if (nPopr == MAX_CNT_FRACTION_RECT)
				{

					if (TekFraction > 0)
					{
						TekFraction--;
						SecTempPrev1 = Seconds;
						SetAngle(AngleFractionRect[TekFraction]);
					}
					CountKeys = 3;
				}
				else
				{
					CountKeys = 2;
					if (flPopr == 0) TempFractionRect[nPopr] -= AddPressKeys;
					if (flPopr == 1) {
						AngleFractionRect[nPopr]--;
						CountKeys = 1;
					}
				}
				break;
			case 314:
				PowerPhase[nPopr] -= AddPressKeys10;
				CountKeys = 2;
				break;
			case 315:
				KtPhase[nPopr] -= AddPressKeys;
				CountKeys = 2;
				break;
#if ENABLE_SENSOR_SORTING
			case 316:
				if (ds1820_nums[nPopr]>0) ds1820_nums[nPopr]--;
				CountKeys=4;
				break;   
#endif
			}

			//NeedDisplaying=1;
			DisplayData();
			flNeedAnalyse = 1;
			if (CountState == 0) CountState = 4;
			if (CountKeys == 0) CountKeys = 6;
			CountState = MENU_DELAY_SEC;
		}
		flNeedScanKbd = 0;
		flScanKbd = 0; // Убираем признак того, что сканируется клавиатура.
	}
}

