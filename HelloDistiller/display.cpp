// Последнее обновление 2018-07-25 by Phisik
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Здесь всяческий вывод на экран
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "configuration.h"
#include "declarations.h"

char flNumStrPrint = 0;

#if SHOW_FLOAT_TEMPERATURES
	#define PRINT_TEMPERATURES() { String msg = "T=" + String(0.1*DS_TEMP(TEMP_KUB), 1) + "," + String(0.1*DS_TEMP(TEMP_DEFL), 1) + "," + String(0.1*DS_TEMP(TEMP_TSA), 1);\
								   strcpy(lcd_buffer, msg.c_str());\
							       my_lcdprint(lcd_buffer);\
    }
#else
	#define PRINT_TEMPERATURES() { sprintf_P(lcd_buffer, PSTR("D2 T=%3i,%3i,%3i"), DS_TEMP(TEMP_KUB), DS_TEMP(TEMP_RK20), DS_TEMP(TEMP_TSA));\
								 my_lcdprint(lcd_buffer);\
	}
#endif


void my_lcdprint(char *s)
{
	char i;
	char len;
	len = strlen(lcd_buffer);
	for (i = len; i < LCD_WIDTH; i++)
		s[i] = ' ';
	s[LCD_WIDTH] = 0;

	if (IspReg == 111 && s[9] == 'R' && flNumStrPrint == 0) s[9] = 'N';

	lcd.print(s);

	// MQTT code by max506 & limon
#if USE_MQTT_BROKER

	if (flNumStrPrint == 0) {

		snprintf_P(lcd_mqtt_buf1, MQTT_BUFFER_SIZE, fmt_lcd1, s);

		// Убрать завершающие пробелы
		for (i = strlen(lcd_mqtt_buf1) - 1; ; i--) {
			if (lcd_mqtt_buf1[i] == ' ') lcd_mqtt_buf1[i] = '\0';
			else break;
		}
	}
	else if (flNumStrPrint == 1) {
		snprintf_P(lcd_mqtt_buf2, MQTT_BUFFER_SIZE, fmt_lcd2, s);

		// Убрать завершающие пробелы
		for (i = strlen(lcd_mqtt_buf2) - 1; ; i--) {
			if (lcd_mqtt_buf2[i] == ' ') lcd_mqtt_buf2[i] = '\0';
			else break;
		}
	}
#endif // USE_MQTT_BROKER

#if USE_GSM_WIFI==1

	// Если состояние отправки - надо отправить экран, тогда отправляем еще и на сервер.
	if (flGPRSState == 20 || flGPRSState == 164)
	{
		if (flNumStrPrint == 0)
		{   // Первой строчкой отправляем id устройства и первую строку экрана
			sprintf_P(my_gprs_buffer, PSTR("%s;%s"), idDevice, s);
			GSM_SERIAL.print(my_gprs_buffer);
#ifdef TESTGSM
			DEBUG_SERIAL.println(F("SendDTA1:"));
			DEBUG_SERIAL.println(my_gprs_buffer);
#endif

		}
		else
		{   // Второй строчкой отправляем нужную часть экрана, состояние контроллера и последнюю выполненную команду. 
			sprintf_P(my_gprs_buffer, PSTR("%s;%3i;%3i;%c#"), s, (int)IspReg, (int)StateMachine, cmdGPRSIsp);
			GSM_SERIAL.println(my_gprs_buffer);
#ifdef TESTGSM
			DEBUG_SERIAL.println(F("SendDTA2:"));
			DEBUG_SERIAL.println(my_gprs_buffer);
#endif
			flGPRSState++; // Переводим контроллер в состояние отправлено.
			//timeWaitGPRS=20;
			timeWaitGPRS = 2; // Две секунды достаточно для отправки
		}
	}
#endif

	flNumStrPrint++;

#ifdef TESTMEM
	DEBUG_SERIAL.println(F("\n[memCheck_print]"));
	DEBUG_SERIAL.println(freeRam());
#endif
}

unsigned char hour, minute, second;
int V1, V2, V3;

void formTRZGDistill()
{
	if (TempDeflBegDistil > 0) sprintf_P(lcd_buffer, PSTR("tD=%3i/%3i %4iW"), DS_TEMP(TEMP_DEFL), TempDeflBegDistil, Power);
	else  sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i %4iW"), DS_TEMP(TEMP_KUB), -TempDeflBegDistil, Power);

}

void formTSAErr()
{
	sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TSA!"), hour, minute, second);
	//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
	my_lcdprint(lcd_buffer); //вывод сz`одержимого буфера на LCD
	sprintf_P(lcd_buffer, PSTR("tT=%3i/%3i"), DS_TEMP(TEMP_TSA), MAX_TEMP_TSA);
}

void DisplayRectif()
{
	if (DispPage == 0)
	{
		switch (StateMachine)
		{
		case 0: //Не запущено
		case 1: //Не запущено
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u Rect"), hour, minute, second);

			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			//sprintf_P(lcd_buffer,PSTR("tK=%3i P=%4i"),DS_TEMP(TEMP_KUB),Power); 
			break;
		case 2: //Разгон
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u R RZG"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			if (tEndRectRazgon > 0) sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i %4iW"), DS_TEMP(TEMP_KUB), tEndRectRazgon, UstPower);
			else sprintf_P(lcd_buffer, PSTR("tN=%3i/%3i %4iW"), DS_TEMP(TEMP_RK20), -tEndRectRazgon, UstPower);
			break;
		case 3: //Стабилицация колонны
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u R NSB"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tN=%3i(%3i)%4iW"), DS_TEMP(TEMP_RK20), (IspReg == 111 ? (int)(SecOstatok) : (int)(Seconds - SecTempPrev)), PowerRect);
			break;
		case 4: //Отбор голов
			if (IspReg != 118)
			{
				sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u R GLV"), hour, minute, second);
				//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     

				my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				if (UrovenProvodimostSR == 0)
					sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i %4iW"), DS_TEMP(TEMP_KUB), tEndRectOtbGlv, PowerRect);
				else
					if (UrovenProvodimostSR > 0)
					{
						if (UrovenProvodimostSR == 2) sprintf_P(lcd_buffer, PSTR("Pr=%3i/%3i %4iW"), U_GLV, UROVEN_ALARM, PowerRect);
						else sprintf_P(lcd_buffer, PSTR("Pr=%3i/%3i %4iW"), U_GLV, UrovenProvodimostSR, PowerRect);
					}
					else
						sprintf_P(lcd_buffer, PSTR("Tm=%3i/%3i %4iW"), (int)SecOstatok, -UrovenProvodimostSR, PowerRect);
			}
			else
			{

				sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u RF GLV"), hour, minute, second);
				my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
				if (TekFraction < CountFractionRect) V2 = TempFractionRect[TekFraction];
				if (V2 >= 0) sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i %4iW"), DS_TEMP(TEMP_KUB), V2, UstPower);
				else sprintf_P(lcd_buffer, PSTR("t=%5imin %4iW"), (int)SecOstatok, UstPower);

			}
			break;
		case 5: //Стоп, ожидание возврата температуры
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u R Stop"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tN=%3i/%3i %2i%%"), DS_TEMP(TEMP_RK20), tStabSR, ProcChimSR);
			break;
		case 6: //Ректификация
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u RSt %2i%%"), hour, minute, second, ProcChimSR);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tN=%3i/%3i/d%2i"), DS_TEMP(TEMP_RK20), tStabSR + tDeltaRect, tDeltaRect);
			break;
		case 7: //Отбор хвостов
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u R Hvost"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i"), DS_TEMP(TEMP_KUB), IspReg != 118 ? tEndRect : TekTemp);
			break;
		case 8: //Отбор ожидание 3 минут
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u R Wait"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i"), DS_TEMP(TEMP_KUB), IspReg != 118 ? tEndRect : TekTemp);
			break;
		case 9: //Ожидание датчика уровня
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u R Wait"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("by Urov=%3i/%3i"), U_UROVEN, UROVEN_ALARM);
			break;
		case 100: //Окончание
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u Rec End"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i End"), DS_TEMP(TEMP_KUB), tEndRect);
			break;
		case 101: //Температура в ТСА превысила предельную
			formTSAErr();
			break;
		case 102: //Давление
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u Pres"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод сz`одержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("MPX5010=%3i/%3i"), U_MPX5010, AlarmMPX5010);
			break;

		}
	}
	else
	{
	PRINT_TEMPERATURES()
		if (StateMachine != 5)
		{
			if (IspReg != 118)  {
				// Phisik on 2018-07-06 I switched off the 3d screen and place power on the 2nd 
				sprintf_P(lcd_buffer, PSTR("%4iW,%3uV,%3uA"), FactPower, (uint16_t)MaxVoltsOut, (uint16_t)MaxIOut);
				// sprintf_P(lcd_buffer, PSTR("%3immV=%4u,%4u"), U_MPX5010, U_UROVEN, U_GLV);
			} else
				if (TekFraction < CountFractionRect)
				{
					if (TempFractionRect[TekFraction] > 0)
						sprintf_P(lcd_buffer, PSTR("%3imm t=%3i F%1i/%1i"), U_MPX5010, TempFractionRect[TekFraction], (int)TekFraction + 1, (int)CountFractionRect);
					else
						sprintf_P(lcd_buffer, PSTR("%3imm %4im F%1i/%1i"), U_MPX5010, (int)SecOstatok, (int)TekFraction + 1, (int)CountFractionRect);
				}
				else
					sprintf_P(lcd_buffer, PSTR("%3immV=%4u F%1i/%1i"), U_MPX5010, U_UROVEN, (int)TekFraction + 1, (int)CountFractionRect);
		}
		else sprintf_P(lcd_buffer, PSTR("%3imm reSt=%5i"), U_MPX5010, time1);
	}

}

void DisplayDistDefl()
{
	if (DispPage == 0)
	{
		switch (StateMachine)
		{
		case 0: //Не запущено
		case 1: //Нагрев до температуры 50 градусов
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DD"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			formTRZGDistill();
			break;
		case 2: //Ожидание закипания (прогреется дефлегматор)
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DD Rzg"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tD=%3i/%3i %4iW"), DS_TEMP(TEMP_DEFL), TempDeflBegDistil, UstPower);
			break;
		case 3: //Ожидание закипания (прогреется куб дефлегматор)
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DD Rzg"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i %4iW"), DS_TEMP(TEMP_KUB), TempDeflBegDistil, UstPower);
			break;
		case 4: //Работа без дефлегматора
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DD BezD"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tD=%3i/%3i %4iW"), DS_TEMP(TEMP_DEFL), TempDefl, UstPower);
			break;
		case 5: //Работа с 50% дефлегмацией
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DD 50%%"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tD=%3i/%3i %4iW"), DS_TEMP(TEMP_DEFL), TempDefl - DeltaDefl, UstPower);
			break;
		case 6: //Работа с 100% дефлегмацией
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DD 100%%"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tD=%3i/%3i %4iW"), DS_TEMP(TEMP_DEFL), TempDefl - DeltaDefl, UstPower);
			break;
		case 7: //Ожидание для охлаждения
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DD Wait"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i Wait=%4i"), DS_TEMP(TEMP_KUB), SecondsEnd);
			break;
		case 100: //Окончание
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DD end"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i end"), DS_TEMP(TEMP_KUB));
			break;
		}
	}
	else
	{
		PRINT_TEMPERATURES();
		sprintf_P(lcd_buffer, PSTR("V%4u,%4u-%4iW"), U_VODA, U_UROVEN, deltaPower);
	}

}
/*
void DisplaySimpleGLV()
 {

 if (DispPage==0)
 {
 switch (StateMachine)
 {
 case 0: //Не запущено
 case 1: //Нагрев до температуры 50 градусов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sGl Not"),hour,minute,second);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 formTRZGDistill();
 break;
 case 2: //Ожидание закипания (прогреется дефлегматор)
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sGl Rzg"),hour,minute,second);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 formTRZGDistill();
 break;
 case 3: //Ожидание закипания (прогреется куб дефлегматор)
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sGl Rzg"),hour,minute,second);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),TempDeflBegDistil,Power);
 break;
 case 4: //Отбор голов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sGl Otb"),hour,minute,second);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),Temp3P,PowerGlvDistil);
 break;
 case 5: //Ожидание 60 секунд для охлаждения
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sOtbGlv"),hour,minute,second);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tK=%3i Wait=%4i"),DS_TEMP(TEMP_KUB),SecondsEnd);
 break;
 case 100: //Окончание
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sOtbGlv"),hour,minute,second);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tK=%3i end"),DS_TEMP(TEMP_KUB));
 break;
 case 101: //Температура в ТСА превысила предельную
 formTSAErr();
 break;
 }
 }
 else
 {
 PRINT_TEMPERATURES();
 sprintf_P(lcd_buffer,PSTR("V=%4u,%4u,%4u"),U_VODA,U_UROVEN,U_GLV);
 }
 }*/

void DisplaySimpleDistill()
{
	if (DispPage == 0)
	{
		if (IspReg == 104)
		{
			V1 = 1;
			V2 = Temp1P;
		}
		if (IspReg == 106)
		{
			V1 = 2;
			V2 = Temp2P;
		}
		if (IspReg == 107 || IspReg == 105)
		{
			V1 = 3;
			V2 = Temp3P;
		}

		if (IspReg != 105)  sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u %1u-Otbor"), hour, minute, second, V1);
		else  sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u sGl"), hour, minute, second);

		switch (StateMachine)
		{
		case 0: //Не запущено
		case 1: //Нагрев до температуры 50 градусов
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			break;
		case 2: //Ожидание закипания (прогреется дефлегматор)
			if (IspReg != 105)  sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u %1u-O Rzg"), hour, minute, second, V1);
			else sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u sGl Rzg"), hour, minute, second, V1);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			formTRZGDistill();
			break;
		case 3: //Дистилляция
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i %4iW"), DS_TEMP(TEMP_KUB), V2, UstPower);
			break;
		case 4: //Ожидание 60 секунд для охлаждения
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i Wait=%4i"), DS_TEMP(TEMP_KUB), SecondsEnd);
			break;
		case 100: //Окончание
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i end"), DS_TEMP(TEMP_KUB));
			break;
		case 101: //Температура в ТСА превысила предельную
			formTSAErr();
			break;
		}
	}
	else
	{
		PRINT_TEMPERATURES();		
		sprintf_P(lcd_buffer, PSTR("%4iW,%3uV,%3uA"), FactPower, (uint16_t)MaxVoltsOut, (uint16_t)MaxIOut);
		//sprintf_P(lcd_buffer, PSTR("V%4u,%4u %4uW"), U_VODA, U_UROVEN, UstPower);
	}

}
void DisplayFracionDistill()
{
	if (DispPage == 0)
	{
		sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DsF %1i/%1i"), hour, minute, second, (int)(TekFraction < CountFractionDist) ? TekFraction + 1 : CountFractionDist, (int)CountFractionDist);

		switch (StateMachine)
		{
		case 0: //Не запущено
		case 1: //Нагрев до температуры 50 градусов
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			break;
		case 2: //Ожидание закипания (прогреется дефлегматор)
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DsF Rzg"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			formTRZGDistill();
			break;
		case 3: //Дистилляция
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

			if (TekFraction < CountFractionDist) V2 = TempFractionDist[TekFraction];
			if (V2 >= 0) sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i %4iW"), DS_TEMP(TEMP_KUB), V2, UstPower);
			else sprintf_P(lcd_buffer, PSTR("t=%5imin %4iW"), (int)SecOstatok, UstPower);

			break;
		case 4: //Ожидание 60 секунд для охлаждения
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i Wait=%4i"), DS_TEMP(TEMP_KUB), time3);
			break;
		case 100: //Окончание
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i end"), DS_TEMP(TEMP_KUB));
			break;
		case 101: //Температура в ТСА превысила предельную
			formTSAErr();
			break;
		}
	}
	else
	{
		PRINT_TEMPERATURES();
		sprintf_P(lcd_buffer, PSTR("V%4u,%4u %4uW"), U_VODA, U_UROVEN, UstPower);
	}

}
void DisplayRazvar()
{
	if (DispPage == 0)
	{
		if (IspReg == 114)  sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u HLD Ztr"), hour, minute, second);
		else sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u Razvar"), hour, minute, second);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

		switch (StateMachine)
		{
		case 0: //Не запущено
		  //            case 1: //Нагрев до температуры 50 градусов
		  //              sprintf_P(lcd_buffer,PSTR("t=%3i P=%4i"),DS_TEMP(TEMP_RAZVAR),UstPower); 
		  //              break;
		case 1: //Нагрев до температуры 50 градусов
			sprintf_P(lcd_buffer, PSTR("t=%3i/%3i %4iW"), DS_TEMP(TEMP_RAZVAR), (int)TempZSP * 10, UstPower);
			break;
		case 2: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
			sprintf_P(lcd_buffer, PSTR("t=%3i ZASYP & Dn"), DS_TEMP(TEMP_RAZVAR));
			break;
		case 3: //Нагрев до температуры 64 градуса
			sprintf_P(lcd_buffer, PSTR("t=%3i/%3i %4iW"), DS_TEMP(TEMP_RAZVAR), (int)TempZSPSld * 10, UstPower);
			break;
		case 4: //Ожидание 15 минут, поддержка температуры
			sprintf_P(lcd_buffer, PSTR("t=%3i %4iW %4i"), DS_TEMP(TEMP_RAZVAR), UstPower, time2);
			break;
		case 5: //Нагрев до кипения
			if (ds1820_devices < 2)
				sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i %4iW"), DS_TEMP(TEMP_RAZVAR), TempKipenZator, UstPower);
			else
				sprintf_P(lcd_buffer, PSTR("tD=%3i/%3i %4iW"), DS_TEMP(TEMP_DEFL), TempKipenZator, UstPower);
			break;
		case 6: //Варка
			sprintf_P(lcd_buffer, PSTR("t=%3i %4iW %4i"), DS_TEMP(TEMP_RAZVAR), PowerRazvZerno, time2);
			break;
		case 7: //Охлаждение до температыры осахаривания
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i HLD-1"), DS_TEMP(TEMP_RAZVAR), (int)TempZSPSld * 10);
			break;
		case 8: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
			sprintf_P(lcd_buffer, PSTR("t=%3i SOLOD & Dn"), DS_TEMP(TEMP_RAZVAR));
			break;
		case 9: //Мешаем 10 минут
			sprintf_P(lcd_buffer, PSTR("tK=%3i MIX %4i"), DS_TEMP(TEMP_RAZVAR), time2);
			break;
		case 10: //Осахаривание
			sprintf_P(lcd_buffer, PSTR("tK=%3i OSH %4i"), DS_TEMP(TEMP_RAZVAR), time2);
			break;
		case 11: //Охлаждение до температуры первичного внесения дрожжей осахаривания
			sprintf_P(lcd_buffer, PSTR("tK=%3i/400 HLD-2"), DS_TEMP(TEMP_RAZVAR));
			break;
		case 12: //Охлаждение до температыры осахаривания
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i HLD-3"), DS_TEMP(TEMP_RAZVAR), (int)TempHLDZatorBrog1 * 10);
			break;
		case 13: //Поддержка брожения, ничего не делаем, только мешаем периодически
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i BROG"), DS_TEMP(TEMP_RAZVAR), (int)TempHLDZatorBrog1 * 10);
			break;
		case 14: //Поддержка брожения - охлаждение
			sprintf_P(lcd_buffer, PSTR("tK=%3i/%3i BR-H"), DS_TEMP(TEMP_RAZVAR), (int)TempHLDZatorBrog1 * 10 - 5);
			break;
		case 100: //Окончание
			sprintf_P(lcd_buffer, PSTR("t=%3i end"), DS_TEMP(TEMP_RAZVAR));
			break;
		}
	}
	else
	{
		PRINT_TEMPERATURES();
		sprintf_P(lcd_buffer, PSTR("OSH=%3u NPG=%4u"), (int)TempZSPSld * 10, U_NPG);
	}

}
/*
void DisplayNDRF()
 {
 if (DispPage==0)
 {
 switch (StateMachine)
 {
 case 0: //Не запущено
 case 1: //Не запущено
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u NDRF"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tK=%3i P=%4i"),DS_TEMP(TEMP_KUB),Power);
 break;
 case 2: //Разгон
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND RZG"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 if (tEndRectRazgon>0) sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),tEndRectRazgon,UstPower);
 else sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i %4iW"),DS_TEMP(TEMP_RK20),-tEndRectRazgon,UstPower);
 break;
 case 3: //Стабилицация колонны
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND NSB"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("t=%3i(%4i)%4iW"),DS_TEMP(TEMP_RK20),(int)(SecOstatok),PowerRect);
 break;
 case 4: //Отбор голов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND GLV"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 if (UrovenProvodimostSR==0)
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),tEndRectOtbGlv,PowerRect);
 else
 if (UrovenProvodimostSR>0)
 sprintf_P(lcd_buffer,PSTR("Pr=%3i/%3i %4iW"),U_GLV,UrovenProvodimostSR,PowerRect);
 else
 sprintf_P(lcd_buffer,PSTR("Tm=%3i/%3i %4iW"),(int) SecOstatok,-UrovenProvodimostSR,PowerRect);
 break;
 case 5: //Стоп, ожидание возврата температуры
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND Stop"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i %2i%%"),DS_TEMP(TEMP_RK20),tStabSR,ProcChimSR);
 break;
 case 6: //Ректификация
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u NDst %2i%%"),hour,minute,second,ProcChimSR);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i/d%2i"),DS_TEMP(TEMP_RK20),tStabSR+tDeltaRect,tDeltaRect);
 break;
 case 7: //Отбор хвостов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND Hvst"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i"),DS_TEMP(TEMP_KUB),tEndRect);
 break;
 case 8: //Отбор ожидание 3 минут
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND Wait"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i"),DS_TEMP(TEMP_KUB),tEndRect);
 break;
 case 9: //Ожидание датчика уровня
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u R Wait"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("by Urov=%3i/%3i"),U_UROVEN,UROVEN_ALARM);
 break;
 case 100: //Окончание
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND End"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i"),DS_TEMP(TEMP_KUB),tEndRect);
 break;
 case 101: //Температура в ТСА превысила предельную
 formTSAErr();
 break;
 case 102: //Давление
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u Pres"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод сz`одержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("MPX5010=%3i/%3i"),U_MPX5010,AlarmMPX5010);
 break;

 }
 }
 else
 {
 PRINT_TEMPERATURES();
 sprintf_P(lcd_buffer,PSTR("%3immV=%4u,%4u"),U_MPX5010,U_UROVEN,U_GLV);
 }

 }*/

void DisplayNBK()
{
	if (DispPage == 0)
	{
		switch (StateMachine)
		{
		case 0: //Не запущено
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u NBK"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tK=%3i,%3imm"), DS_TEMP(TEMP_KUB), U_MPX5010);
			break;
		case 1: //Ожидание закипания (прогреется дефлегматор)
		case 2: //Ожидание закипания (прогреется дефлегматор)
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u NBK Rzg"), hour, minute, second);
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			formTRZGDistill();
			break;
		case 3: //Ожидание запуска
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u NBK Beg"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("UP to Beg %3imm"), U_MPX5010);
			break;
		case 4: //Запущено
		case 6: //Запущено, подача браги остановлена
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u NBK Wrk"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("Sp=%4i,%3imm"), (int)SpeedNBK, U_MPX5010);
			break;
		case 5: //Превышение температуры вверху
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u NBK tmp"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("tD=%3i/970,s%4i"), DS_TEMP(TEMP_DEFL), time2);
			break;
		case 100: //Окончание
			sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u NBK End"), hour, minute, second);
			//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("Sp=%4i,%3imm"), (int)SpeedNBK, U_MPX5010);
			break;
		}
	}
	else
	{
		PRINT_TEMPERATURES();
		sprintf_P(lcd_buffer, PSTR("%4uW,%3imm %3i"), UstPower, U_MPX5010, MaxPressByPeriod);
	}
}
void DisplayTestKLP()
{
	if (DispPage == 0)
	{
		sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u tst Dev"), hour, minute, second);
		//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("St=%3i CHIM=%3i"), StateMachine, ProcChimSR);
	}
	else
	{
		PRINT_TEMPERATURES();
		sprintf_P(lcd_buffer, PSTR("%3u,%3u Gas=%4u"), DS_TEMP(3), DS_TEMP(4), U_GAS);
	}

}

void DisplayExtContol()
{
	if (DispPage == 0)
	{
		sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ExtCtrl"), hour, minute, second);
		//sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);                     
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("T=%3u,%3u %4iW"), DS_TEMP(TEMP_KUB), DS_TEMP(TEMP_DEFL), UstPower);
	}
	else
	{
		PRINT_TEMPERATURES();
		sprintf_P(lcd_buffer, PSTR("%3u,%3u Gas=%4u"), DS_TEMP(3), DS_TEMP(4), U_GAS);
	}

}

void DisplayBeer()
{
	if (DispPage == 0)
	{
		sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u beer"), hour, minute, second);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		if (StateMachine == 2)   sprintf_P(lcd_buffer, PSTR("T=%3i/%3i P%2i/%2i"), DS_TEMP(TEMP_TERMOSTAT), (int)tempP[KlTek] * 10, (int)KlTek, (int)CntPause);
		if (StateMachine == 3)
		{
			if (timeP[KlTek] != 0)
				sprintf_P(lcd_buffer, PSTR("Ts=%5i P=%2i/%2i"), time2, (int)KlTek, (int)CntPause);
			else
				sprintf_P(lcd_buffer, PSTR("Press UP to%2i/%2i"), (int)KlTek + 1, (int)CntPause);

		}
		if (StateMachine == 100)   sprintf_P(lcd_buffer, PSTR("end"));
	}
	else
	{
		PRINT_TEMPERATURES();
		sprintf_P(lcd_buffer, PSTR("mix=%5i %4iW"), time1, UstPower);
	}
}

void DisplayData()
{
	int tic1;
	static int PrevState = 0;
	static char flDspDop = 0; // Флаг для отображения информации (через раз)
	char j; // Временная переменная для цикла
	flNumStrPrint = 0;
	flDspDop = !flDspDop;

	hour = Seconds / 3600;
	tic1 = Seconds % 3600;
	minute = tic1 / 60;
	second = tic1 % 60;

	if (FlState == 0)
		lcd.setCursor(0, 0);
	else
	{
		// Если предыдущее состояние было 0, то даем задерку в 1/10 секунды, чтобы не слишком часто обновлялся
		// дисплей.
		if (PrevState == 0)  delay(100);

		if (FlState != PrevState)
		{
			lcd.clear();  //очистка дисплея   
			PrevState = FlState;
		}

		lcd.setCursor(0, 0);
	}

#ifdef TESTMEM
	DEBUG_SERIAL.println(F("\n[memCheck_displ]"));
	DEBUG_SERIAL.println(freeRam());
#endif

	switch (FlState)
	{
	case 0:

		if (DispPage < 2) // Первые две страницы отображаются по-разному, остальные всегда одни и те же
		{
			switch (IspReg)
			{
			case 101: // Displaying
			  //sprintf_P(lcd_buffer,PSTR("S=%5u TT=%5u"),Seconds,MaxTimeTimer); 

			  //  Выводим сетевое напряжение по тому же принципу, что и работают дешевые вольтметры, то есть максимальное количество вольт в сети умножаем на 0,707
			  //  это напряжение нужно для калибровки нашего измерителя

				if (DispPage == 0)
				{
					sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u DSP 1/2"), hour, minute, second);
					my_lcdprint(lcd_buffer);
					sprintf_P(lcd_buffer, PSTR("Zr=%4u U=%3i"), TicZero, (uint16_t)MaxVoltsOut);
				}

				if (DispPage == 1)
				{
					PRINT_TEMPERATURES();
					sprintf_P(lcd_buffer, PSTR("V=%4u,%4u,%4u"), U_VODA, U_UROVEN, U_GLV);
				}
				//          lcd.setCursor(0, 1);
				//          my_lcdprint(lcd_buffer);                      

				break;
			case 102: // Термостат         
				if (DispPage == 0)
				{
					sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TermSt"), hour, minute, second, Delta);
					my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
					sprintf_P(lcd_buffer, PSTR("t=%3i/%3i(%2i) %1i"), DS_TEMP(TEMP_TERMOSTAT), TempTerm, Delta, StateMachine);
				}
				else
				{
					PRINT_TEMPERATURES();
					sprintf_P(lcd_buffer, PSTR("%3u,%3u P=%3imm"), DS_TEMP(3), DS_TEMP(4), U_MPX5010);
				}
				break;
			case 115: // Таймер
				if (DispPage == 0)
				{
					sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u Timer"), hour, minute, second, Delta);
					my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
					minute = Seconds / 60;
					sprintf_P(lcd_buffer, PSTR("tm=%3i/%3i %4iW"), minute, timerMinute, UstPower);
				}
				else
				{
					PRINT_TEMPERATURES();
					sprintf_P(lcd_buffer, PSTR("%3u,%3u %4iW"), DS_TEMP(3), DS_TEMP(4), PowerMinute);
				}
				break;
			case 116: // Пивоварня - клон браумастера
				DisplayBeer();
				break;
			case 103: // Регулятор мощности
				if (DispPage == 0)
				{
#ifndef USE_SLAVE
					sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u RMU=%3u"), hour, minute, second, (uint16_t)MaxVoltsOut);
					my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
#if SIMPLED_VERSION==20
					sprintf_P(lcd_buffer, PSTR("P=%3u/%3u%% c%3u"), UstPowerReg / 10, Power / 10, indexOut);
#else
					sprintf_P(lcd_buffer, PSTR("P=%4u/%4u c%3u"), FactPower, UstPowerReg, indexOut);
#endif
#ifdef TEST
					if (NUM_PHASE > 1) sprintf_P(lcd_buffer, PSTR("%s"), my_rx_buffer);
#endif
#else
					sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u RMU=%3u"), hour, minute, second, (uint16_t)MaxVoltsOut);
					my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
					sprintf_P(lcd_buffer, PSTR("PSlv=%4u/%4u"), UstPower, Power);
#endif
				}
				else
				{
					PRINT_TEMPERATURES();
#ifndef USE_SLAVE
					sprintf_P(lcd_buffer, PSTR("Up to auto=%4uW"), Power);
#else
					if (flAutoDetectPowerTEN)
						sprintf_P(lcd_buffer, PSTR("Up to auto=%4uW"), Power);
					else
						sprintf_P(lcd_buffer, PSTR("SlaveOn (Up)=%1i"), (int)SlaveON);
#endif
				}
				break;
			case 105: // Отбор голов33
			  //          DisplaySimpleGLV();
			  //          break;
			case 104: // Перевый (недробный) отбор
			case 106: // Второй дробный отбор
			case 107: // Третий дробный отбор
				DisplaySimpleDistill();
				break;

			case 117: // Фракционная перегонка
				DisplayFracionDistill();
				break;

			case 108: // Разваривание
			case 113: // Разваривание мучно-солодового затора (без варки).
			case 114: // Разваривание
				DisplayRazvar();
				break;
			case 109: // Ректификация
			case 118: // Ректификация
				DisplayRectif();
				break;
#ifndef TEST
#ifndef DEBUG // Это отображение информации выкусываем, если в режиме теста, потому что не хватает памяти
#ifndef TESTGSM
#ifndef TESTGSM1
			case 110: // Дистилляция с дефлегматором
				DisplayDistDefl();
				break;
			case 111: // НДРФ
				DisplayRectif();
				break;
#endif
#endif
#endif
#endif

			case 112: // NBK
				DisplayNBK();
				break;

			case 130: // Внешнее управление
				DisplayExtContol();
				break;
			case 129: // тест клапанов
				DisplayTestKLP();
				break;
			case 248:
				//          sprintf_P(lcd_buffer,PSTR("LOW LEVEL VOLTS!")); 
				//          my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				//          sprintf_P(lcd_buffer,PSTR("Umax=%i Sec=%3i"),(uint16_t)MaxVoltsOut,(int) ErrCountIndex*12); 
				break;
			case 249:
				sprintf_P(lcd_buffer, PSTR("NO DETECT ZERO!"));
				my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				sprintf_P(lcd_buffer, PSTR("Umax=%i"), (uint16_t)MaxVoltsOut);
				break;
			case 250:
				sprintf_P(lcd_buffer, PSTR("ALARM VODA !"));
				my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				break;
			case 251:
				sprintf_P(lcd_buffer, PSTR("ERR Ds18b20!"));
				my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				sprintf_P(lcd_buffer, PSTR("Num=%i"), (int)NumErrDs18);

				break;
			case 252:
				sprintf_P(lcd_buffer, PSTR("ALARM GAS SENS!"));
				my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				break;
			case 253:
				sprintf_P(lcd_buffer, PSTR("NPG OSUHENIE!"));
				my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				sprintf_P(lcd_buffer, PSTR("U_NPG=%i CNT=%i"), U_NPG, countAlrmNPG);
				break;
			case 254:
				sprintf_P(lcd_buffer, PSTR("NPG PEREPOLN!"));
				my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
				sprintf_P(lcd_buffer, PSTR("U_NPG=%i CNT=%i"), U_NPG, countAlrmNPG);
				break;

			}
			// Это обычное состояние
			//
			if (StateNPG == 1 && DispPage == 0) sprintf_P(lcd_buffer, PSTR("Napolnenie NPG.."));

		}
		// Третья страница отображения универсальна, там показываем напряжение, дистанцию и возможное число ошибок
		// расчета среднеквадратичного. 

		if (DispPage == 2)
		{
#if USE_GSM_WIFI==1
			sprintf_P(lcd_buffer, PSTR("D3 %3i,w%3u,i%3i"), (int)flGPRSState, (int)timeWaitGPRS, (int)timeGPRS);
			my_lcdprint(lcd_buffer);
#else
			sprintf_P(lcd_buffer, PSTR("D3 No GSM Supp. "));
			my_lcdprint(lcd_buffer);
#endif

			sprintf_P(lcd_buffer, PSTR("%4iW,%3uV,%3uA"), FactPower, (uint16_t)MaxVoltsOut, (uint16_t)MaxIOut);
		}
		// На четвертой странице показываем всю температуру и давление
		if (DispPage == 3)
		{
			sprintf_P(lcd_buffer, PSTR("D4 T=%3i,%3i,%3i"), DS_TEMP(TEMP_KUB), DS_TEMP(TEMP_DEFL), DS_TEMP(TEMP_TSA));
			my_lcdprint(lcd_buffer);
			sprintf_P(lcd_buffer, PSTR("%3i,%3i P=%3imm"), DS_TEMP(3), DS_TEMP(4), U_MPX5010);
		}

		// На пятой странице показываем состояние датчиков уровней
		if (DispPage == 4)
		{
			sprintf_P(lcd_buffer, PSTR("D5 VODA=%4u"), U_VODA);
			my_lcdprint(lcd_buffer);
		#if USE_BMP280_SENSOR
			sprintf(lcd_buffer,"GL=%4u,At=%3imm",U_GLV, PressAtm); //запись в буфер текста и значений температуры в подготовленном
		#else
			sprintf_P(lcd_buffer, PSTR("URV=%4u GL=%4u"), U_UROVEN, U_GLV);
		#endif // USE_BMP280_SENSOR			
		}

		// На шестой странице показываем состояние датчиков уровней
		if (DispPage == 5)
		{
			sprintf_P(lcd_buffer, PSTR("D6 GAS=%4u"), U_GAS);
			my_lcdprint(lcd_buffer);
			sprintf_P(lcd_buffer, PSTR("zPS=%3u,%4uW"), (int)zPSOut, UstPwrPH1);
		}

		// На седьмой странице показываем состояние регулятора мощности
		if (DispPage == 6)
		{
#if NUM_PHASE==1
			sprintf_P(lcd_buffer, PSTR(" %4i,%4i,%4iW"), UstPwrPH1, UstPwrPH2, UstPwrPH3);
			my_lcdprint(lcd_buffer);
			sprintf_P(lcd_buffer, PSTR("D7 %3i,%3i,%3i %%"), KtPhase[0], KtPhase[1], KtPhase[2]);
#else
			sprintf_P(lcd_buffer, PSTR("D7 T=%4u,Z=%4u"), TimeOpenTriac, TicZero);
			my_lcdprint(lcd_buffer);
			sprintf_P(lcd_buffer, PSTR("KLP=%4u,Bz=%3u"), TimeOpenKLP, (int)b_value[0]);
#endif
		}

		// На восьмой можно сменить состояние процесса.
		if (DispPage == 7)
		{
			sprintf_P(lcd_buffer, PSTR("D8 State Machine"));
			my_lcdprint(lcd_buffer);
			sprintf_P(lcd_buffer, PSTR("State = %i"), (int)StateMachine);
		}

		// На девятой состояние ПИД-регулятора
		if (DispPage == 8)
		{
			sprintf_P(lcd_buffer, PSTR("D9 N=%4i O=%4i"), NewErr, OldErrOut);
			my_lcdprint(lcd_buffer);
			sprintf_P(lcd_buffer, PSTR("Dt=%4i It=%4i"), Dt, It);
		}
		// На Десятой странице показываем максимальные температуры за процесс температуру и давление
		if (DispPage == 9)
		{
			sprintf_P(lcd_buffer, PSTR("D10 MaxT=%3i,%3i"), MAX_DS_TEMP(0), MAX_DS_TEMP(1));
			my_lcdprint(lcd_buffer);
			sprintf_P(lcd_buffer, PSTR("%3i,%3i,%3i"), MAX_DS_TEMP(2), MAX_DS_TEMP(3), MAX_DS_TEMP(4));
		}
		// На 11  странице показываем параметры расчета ПИД-регулятора
		if (DispPage == 10)
		{
			sprintf_P(lcd_buffer, PSTR("D11 N,O=%3i %3i"), NewErrTemp, OldErrTempOut);
			my_lcdprint(lcd_buffer);
			sprintf_P(lcd_buffer, PSTR("KId%4i %4i %3i"), KtT, ItTemp, DtTemp);
		}

		if (flDspDop && DispDopInfo > 0) // Если отображаем дополнительную информацию, тогда проверяем ее статус
		{

			if (BeepStateProcess > 1)
			{
				if (DispDopInfo == 3 || DispDopInfo == 4)
				{
					if (!(BeepStateProcess&B00000010) && DispDopInfo == 3) my_beep(BEEP_LONG); // Не пищим, если установлена маска в 2 разряде
					if (!(BeepStateProcess&B00000100) && DispDopInfo == 4) my_beep(BEEP_LONG); // Не пищим, если установлена маска в 3 разряде
				}
				else
					my_beep(BEEP_LONG); // Сначала пищим, предупреждая.
			}
			else
				my_beep(BEEP_LONG); // Сначала пищим, предупреждая.


			if (DispDopInfo == 1) sprintf_P(lcd_buffer, PSTR("Smena TARA!(%3i)"), (IspReg == 117 || IspReg == 118) ? COUNT_ALARM_UROVEN_FR - CountAlarmUroven : COUNT_ALARM_UROVEN - CountAlarmUroven); //Выводим на экран инфомацию о необходимости замены тары.
			if (DispDopInfo == 2) sprintf_P(lcd_buffer, PSTR("VODA! (%3i)"), COUNT_ALARM_VODA - CountAlarmVoda); //Выводим на экран инфомацию о необходимости замены тары.
			if (DispDopInfo == 3) sprintf_P(lcd_buffer, PSTR("Low Power! %3iV"), (uint16_t)MaxVoltsOut); //Выводим на экран инфомацию о необходимости замены тары.
			if (DispDopInfo == 6) sprintf_P(lcd_buffer, PSTR("HIGH Power! %3iV"), (uint16_t)MaxVoltsOut); //Выводим на экран инфомацию о необходимости замены тары.
			if (DispDopInfo == 4) sprintf_P(lcd_buffer, PSTR("Err temp! ds=%2i"), (int)NumErrDs18); //Выводим на экран инфомацию о ошибке чтения датчика.
			if (DispDopInfo == 5) sprintf_P(lcd_buffer, PSTR("!PRESS=%3i/%3i"), U_MPX5010, AlarmMPX5010); //Выводим на экран инфомацию о ошибке чтения датчика.

#if USE_GSM_WIFI==1
	  // Активизируем разовую отправку состояния на сервер
			if (FlToGSM > 1 && flGPRSState != 20 && flGPRSState != 164 && (timeGPRS == 0 || flNeedCall == 1))
			{
				if (flNeedCall == 1)
				{
					flGPRSState = 40; // Активируем звонок если в этом есть необходимость
				}
				else
				{   // Если в данный момент не производится дозвон, тогда активируем GPRS сессию. 
					if (flNeedCall != 2) flGPRSState = 2;
				}
				// Активируем длительную сессию GPRS
				flNeedRefrServer = 0;
				timeGPRS = 250;
			}
#endif
		}

		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

		break;
	case 100:
		sprintf_P(lcd_buffer, PSTR("Settings Menu"), FlState);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 101:
		sprintf_P(lcd_buffer, PSTR("Monitoring Mode "));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 102:
		sprintf_P(lcd_buffer, PSTR("Thermostat"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 103:
		sprintf_P(lcd_buffer, PSTR("Power Regulator"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 104:
		sprintf_P(lcd_buffer, PSTR("1st distillation"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("(non-fractional)"));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 105:
		sprintf_P(lcd_buffer, PSTR("IsR = otbor GLV"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на 
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 106:
		sprintf_P(lcd_buffer, PSTR("IsR = 2 DR otbor"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 107:
		sprintf_P(lcd_buffer, PSTR("IsR = 3 DR otbor"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 108:
		sprintf_P(lcd_buffer, PSTR("IsR = Zator"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Zerno"));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 109:
		sprintf_P(lcd_buffer, PSTR("Rectification"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 110:// Дистилляция с дефлегматором
		sprintf_P(lcd_buffer, PSTR("IsR = Dist Defl"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 111:// НДРФ
		sprintf_P(lcd_buffer, PSTR("IsR = NDRF"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 112:// НБК
		sprintf_P(lcd_buffer, PSTR("Continuous Still"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("      (NBK)     "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 113:// солодо-мучной затор (без варки
		sprintf_P(lcd_buffer, PSTR("IsR = Zator"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Solod - Muka"), PowerRect);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 114:// Охлаждение затора
		sprintf_P(lcd_buffer, PSTR("IsR = HLD Zator"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("by Chiller&Mixer"), PowerRect);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 115:// Таймер
		sprintf_P(lcd_buffer, PSTR("Power regulator"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("+ Shutdown Timer"));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 116:// Варка пива
		sprintf_P(lcd_buffer, PSTR("IsR = Brewing"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Beer"));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 117:// Фракционная дистилляция
		sprintf_P(lcd_buffer, PSTR("IsR = Distill"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Fractional"));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 118:// Фракционная ректификация
		sprintf_P(lcd_buffer, PSTR("IsR = Rectif"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Fractional"));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 130:
		sprintf_P(lcd_buffer, PSTR("IsR = External"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Control"));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 129:
		sprintf_P(lcd_buffer, PSTR("Valve Test Mode"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("                "));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 200:
		sprintf_P(lcd_buffer, PSTR("Max_t_Tst=%5i"), TempTerm);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 201:
		sprintf_P(lcd_buffer, PSTR("TEH Nominal"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Power=%5u"), Power);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 202:
		sprintf_P(lcd_buffer, PSTR("Current TEH"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Power=%5u"), UstPowerReg);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 203:
		sprintf_P(lcd_buffer, PSTR("ParamUSART=%u"), FlToUSART);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 204:
		sprintf_P(lcd_buffer, PSTR("ParamGSM=%u"), FlToGSM);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		if (FlToGSM == 1 || FlToGSM == 0) sprintf_P(lcd_buffer, PSTR("GSM (SMS)"));
		if (FlToGSM == 2) sprintf_P(lcd_buffer, PSTR("GPRS Megafon"));
		if (FlToGSM == 3) sprintf_P(lcd_buffer, PSTR("GPRS BeeLine"));
		if (FlToGSM == 4) sprintf_P(lcd_buffer, PSTR("GPRS MTS"));
		if (FlToGSM == 5) sprintf_P(lcd_buffer, PSTR("GPRS Rostelecom"));
		if (FlToGSM == 6) sprintf_P(lcd_buffer, PSTR("GPRS Tele 2"));
		if (FlToGSM >= 7) sprintf_P(lcd_buffer, PSTR("GPRS Reserv %i"), (int)FlToGSM);
		if (FlToGSM == 10) sprintf_P(lcd_buffer, PSTR("Wi-Fi"));
		if (FlToGSM == 11) sprintf_P(lcd_buffer, PSTR("Android ext"));
		if (FlToGSM == 12) sprintf_P(lcd_buffer, PSTR("Android int"));
		if (FlToGSM > 12) sprintf_P(lcd_buffer, PSTR("Err! 0..11 (%i)"), (int)FlToGSM);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 205:
		sprintf_P(lcd_buffer, PSTR("dtTermostat=%3i"), Delta);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 206:
		sprintf_P(lcd_buffer, PSTR("1st Distillation"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("T_end=%3i"), Temp1P);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 207:
		sprintf_P(lcd_buffer, PSTR("Temp 2 Drobn"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Distill=%3i"), Temp2P);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 208:
		sprintf_P(lcd_buffer, PSTR("Temp 3 Drobn"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Distill=%3i"), Temp3P);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 209:
		sprintf_P(lcd_buffer, PSTR("Rect. Preheating"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("(+/-)T_end=%3i"), tEndRectRazgon);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 210:
		sprintf_P(lcd_buffer, PSTR("Rect. Regulator"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Power=%3i"), PowerRect);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 211:
		sprintf_P(lcd_buffer, PSTR("Corrections for"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("ds18b20 values"));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 212:
		sprintf_P(lcd_buffer, PSTR("Rect. Head"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("T_end=%3i"), tEndRectOtbGlv);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 213:
		sprintf_P(lcd_buffer, PSTR("Rect. Head PWM"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Period=%5u"), timeChimRectOtbGlv);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 214:
		sprintf_P(lcd_buffer, PSTR("Rect. Head"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%% PWM=%3i"), (int)ProcChimOtbGlv);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 215:
		sprintf_P(lcd_buffer, PSTR("Rect. C2H5OH PWM"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Period=%5u"), timeChimRectOtbSR);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 216:
		sprintf_P(lcd_buffer, PSTR("Rect. C2H5OH"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Delta=%3u"), tDeltaRect);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 217:
		sprintf_P(lcd_buffer, PSTR("Rect. C2H5OH"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("T_end=%3i"), tEndRectOtbSR);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 218:
		sprintf_P(lcd_buffer, PSTR("Rect. Shutdown"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("T_end=%3i"), tEndRect);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 250:
		sprintf_P(lcd_buffer, PSTR("Rect. C2H5OH"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("min %% PWM=%2i"), minProcChimOtbSR);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 251:
		sprintf_P(lcd_buffer, PSTR("Rect. Adjust"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("T_stable=%3i"), tStabSR);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 252:
		sprintf_P(lcd_buffer, PSTR("Rect. C2H5OH"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("start %% PWM=%2i"), begProcChimOtbSR);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 253:
		sprintf_P(lcd_buffer, PSTR("Popr MPX=%4i"), P_MPX5010);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%4i/%4i"), U_MPX5010, U_MPX5010 + P_MPX5010);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 219:
		sprintf_P(lcd_buffer, PSTR("Power GLV simple"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Distill=%4i"), PowerGlvDistil);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 220:
		sprintf_P(lcd_buffer, PSTR("Power simple"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Distill=%4i"), PowerDistil);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 221:
		sprintf_P(lcd_buffer, PSTR("Temp Begin Dist"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("(+Def -Kub)=%3i"), TempDeflBegDistil);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 222:
		sprintf_P(lcd_buffer, PSTR("Temp Distill"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("With Defl=%3i"), TempDefl);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 223:
		sprintf_P(lcd_buffer, PSTR("Delta Distill"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("With Defl=%3i"), DeltaDefl);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 224:
		sprintf_P(lcd_buffer, PSTR("Temp Kub Okon"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("DistWithDefl=%3i"), tEndDistDefl);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 225:
		sprintf_P(lcd_buffer, PSTR("BeepEndProc=%1u"), BeepEndProcess);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 226:
		sprintf_P(lcd_buffer, PSTR("BeepStateProc=%1u"), BeepStateProcess);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 227:
		sprintf_P(lcd_buffer, PSTR("BeepKeyPress=%1u"), BeepKeyPress);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 228:
		sprintf_P(lcd_buffer, PSTR("Power Razvar"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Zerno=%4i"), PowerRazvZerno);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 229:
		sprintf_P(lcd_buffer, PSTR("Power Varka"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Zerno=%4i"), PowerVarkaZerno);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 230:
		sprintf_P(lcd_buffer, PSTR("Period Refresh"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Server(sec)=%3u"), (unsigned int)PeriodRefrServer);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer);
		break;
	case 231:
		sprintf_P(lcd_buffer, PSTR("U Peregrev=%3uV"), (unsigned int)NaprPeregrev);
		my_lcdprint(lcd_buffer);
		break;
	case 232:
		sprintf_P(lcd_buffer, PSTR("Urv Barda=%4i"), UrovenBarda);
		my_lcdprint(lcd_buffer);
		sprintf_P(lcd_buffer, PSTR("Barda(%4u)"), U_GLV);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer);
		break;
	case 233:
		sprintf_P(lcd_buffer, PSTR("Provod SR=%4i"), UrovenProvodimostSR);
		my_lcdprint(lcd_buffer);
		break;
	case 234:
		sprintf_P(lcd_buffer, PSTR("Column Stabiliz."));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("(+/-)dt=%5isec"), TimeStabKolonna);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 235:
		sprintf_P(lcd_buffer, PSTR("Edit T & PWM"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Count PWM=%3i"), (int)CntCHIM);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 236:
		sprintf_P(lcd_buffer, PSTR("Rect. PWM Adjust"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Auto-%%PWM=%3i"), (int)DecrementCHIM);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer);
		break;
	case 237:
		sprintf_P(lcd_buffer, PSTR("Rect. PWM Adjust"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Auto+%%PWM=%3i"), (int)IncrementCHIM);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer);
		break;
	case 238:
		sprintf_P(lcd_buffer, PSTR("Rect. PWM Time +"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%% PWM=%5isec"), TimeAutoIncCHIM);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 239:
		sprintf_P(lcd_buffer, PSTR("Column reStab."));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("(+/-)dt=%5isec"), TimeRestabKolonna);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 240:
		sprintf_P(lcd_buffer, PSTR("Beer Pause"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Count =%3i"), (int)CntPause);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 241:
		sprintf_P(lcd_buffer, PSTR("Power correct"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("ASC712 =%3i"), (int)CorrectASC712);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 242:
		sprintf_P(lcd_buffer, PSTR("Server adr="));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%3u.%3u.%3u.%3u"), (unsigned int)ip[0], (unsigned int)ip[1], (unsigned int)ip[2], (unsigned int)ip[3]);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 243:
		sprintf_P(lcd_buffer, PSTR("Server port="));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%u"), ipPort);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 244:
		sprintf_P(lcd_buffer, PSTR("ID Device="));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%s"), idDevice);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 245:
		sprintf_P(lcd_buffer, PSTR("My Phone="));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%s"), my_phone);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 246:
		sprintf_P(lcd_buffer, PSTR("Alarm Pressure"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("MPX5010=%i"), AlarmMPX5010);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 247:
		sprintf_P(lcd_buffer, PSTR("Use Avtonom"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("HLD=%i"), FlAvtonom);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 248:
		sprintf_P(lcd_buffer, PSTR("Time Open"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("BRD=%i"), (int)timeOpenBRD);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 249:
		sprintf_P(lcd_buffer, PSTR("PID Paramters"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%4i %4i %4i"), (int)PIDTemp[0], (int)PIDTemp[1], (int)PIDTemp[2]);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 254:
		sprintf_P(lcd_buffer, PSTR("Power NBK=%4i"), PowerNBK);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 255:
		sprintf_P(lcd_buffer, PSTR("Wi-Fi AP="));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%s"), WiFiAP);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 256:
		sprintf_P(lcd_buffer, PSTR("Wi-Fi Password="));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%s"), WiFiPass);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 257:
		sprintf_P(lcd_buffer, PSTR("Fraction Dist"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Count =%3i"), (int)CountFractionDist);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 258:
		sprintf_P(lcd_buffer, PSTR("Fraction Rectif"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Count =%3i"), (int)CountFractionRect);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 259:
		sprintf_P(lcd_buffer, PSTR("Temp Zasyp"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Zator=%3i"), (int)TempZSP * 10);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 260:
		sprintf_P(lcd_buffer, PSTR("Temp Osahariv"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Zator=%3i"), (int)TempZSPSld * 10);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 261:
		sprintf_P(lcd_buffer, PSTR("Temp Brogenia"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Zator=%3i"), (int)TempHLDZatorBrog1 * 10);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 262:
		sprintf_P(lcd_buffer, PSTR("PhasePW %5i="), Power);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%4i+%4i+%4i"), PowerPhase[0], PowerPhase[1], PowerPhase[2]);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 263:
		sprintf_P(lcd_buffer, PSTR("Phase%% %3i="), (int)KtPhase[0] + KtPhase[1] + KtPhase[2]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("%3i + %3i + %3i"), (int)KtPhase[0], (int)KtPhase[1], (int)KtPhase[2]);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 264:
		sprintf_P(lcd_buffer, PSTR("min Pressure"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("NBK=%3i+%3i=%3i"), (int)minPressNBK * 5, (int)deltaPressNBK, (int)minPressNBK * 5 + (int)deltaPressNBK);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 265:
		sprintf_P(lcd_buffer, PSTR("delta Pressure"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("NBK=%3i+%3i=%3i"), (int)minPressNBK * 5, (int)deltaPressNBK, (int)minPressNBK * 5 + (int)deltaPressNBK);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 266:
		sprintf_P(lcd_buffer, PSTR("time Pressure"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("NBK=%3i"), (int)timePressNBK * 5);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 267:
		sprintf_P(lcd_buffer, PSTR("Upravl Nasos"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("NBK=%3i"), (int)UprNasosNBK);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 268:
		sprintf_P(lcd_buffer, PSTR("%% otbor Tsarga"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Paster(+/-)=%3i"), (int)ProcChimOtbCP);
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
#if ENABLE_SENSOR_SORTING
	case 269:
		sprintf_P(lcd_buffer,PSTR("DS18B20 Order")); //запись в буфер текста и значений температуры в подготовленном
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		// Печатаем номера всех наших сенсоров
		for(int i = 0, n = 0; i < MAX_DS1820; i++) {
			n += sprintf_P(lcd_buffer + n, (i>0)?PSTR(" %02d"):PSTR("%02d"), (int)ds1820_nums[i] + 1);
		}
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;  
#endif
#if USE_BMP280_SENSOR
	case 270:
		sprintf(lcd_buffer,String(F("Time AtmP=%3i")).c_str(),(int)timePressAtm); //запись в буфер текста и значений температуры в подготовленном
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf(lcd_buffer,""); //запись в буфер текста и значений температуры в подготовленном
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
#endif // USE

	case 300:
		sprintf_P(lcd_buffer, PSTR("Temp%1i=%3u"), nPopr, DS_TEMP(nPopr));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("Popr=%3i T=%3u"), ds1820_popr[nPopr], (DS_TEMP(nPopr) + ds1820_popr[nPopr]));
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 301:
		sprintf_P(lcd_buffer, PSTR("TempK(%1i)=%4i"), (int)nPopr, tempK[nPopr]);
		if (flPopr == 0)
		{
			sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
		}

		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		sprintf_P(lcd_buffer, PSTR("CHIM(%1i)=%3i"), (int)nPopr, (int)CHIM[nPopr]);
		if (flPopr == 1)
		{
			sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
		}

		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 302:
		// Первое значение это время работы насоса
		if (nPopr == 0)
			sprintf_P(lcd_buffer, PSTR("Work Pump =%2im"), (int)tempP[nPopr]);
		else
			sprintf_P(lcd_buffer, PSTR("TemP(%1i)=%3i.0"), (int)nPopr, (int)tempP[nPopr]);

		if (flPopr == 0)
		{
			sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
		}

		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		if (nPopr == 0)
			sprintf_P(lcd_buffer, PSTR("Pause Pump=%2im"), (int)timeP[nPopr]);
		else
			sprintf_P(lcd_buffer, PSTR("TimP(%1i)=%3i m"), (int)nPopr, (int)timeP[nPopr]);

		if (flPopr == 1)
		{
			sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
		}

		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 303:
		sprintf_P(lcd_buffer, PSTR("%3u.%3u.%3u.%3u"), (unsigned int)ip[0], (unsigned int)ip[1], (unsigned int)ip[2], (unsigned int)ip[3]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		j = -1;
		while (j < nPopr * 4)
		{
			j++;
			lcd_buffer[j] = ' ';
		}
		lcd_buffer[j + 1] = '*';
		lcd_buffer[j + 2] = 0;

		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

		break;
	case 304:
		sprintf_P(lcd_buffer, PSTR("%s (%3u)"), idDevice, (unsigned int)idDevice[nPopr]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		j = 0;
		while (j < nPopr)
		{
			lcd_buffer[j] = ' ';
			j++;
		}
		lcd_buffer[j] = '*';
		lcd_buffer[j + 1] = 0;

		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

		break;

	case 305:
		sprintf_P(lcd_buffer, PSTR("%s/%3u)"), my_phone, (unsigned int)my_phone[nPopr]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		j = 0;
		while (j < nPopr)
		{
			lcd_buffer[j] = ' ';
			j++;
		}
		lcd_buffer[j] = '*';
		lcd_buffer[j + 1] = 0;

		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

		break;
	case 309:
		sprintf_P(lcd_buffer, PSTR("%4i %4i %4i"), (int)PIDTemp[0], (int)PIDTemp[1], (int)PIDTemp[2]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		if (nPopr == 0) sprintf_P(lcd_buffer, PSTR("   P"));
		if (nPopr == 1) sprintf_P(lcd_buffer, PSTR("        I"));
		if (nPopr == 2) sprintf_P(lcd_buffer, PSTR("             D"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;

	case 310:
		sprintf_P(lcd_buffer, PSTR("%s /%3u"), WiFiAP, (unsigned int)WiFiAP[nPopr]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		j = 0;
		while (j < nPopr)
		{
			lcd_buffer[j] = ' ';
			j++;
		}
		lcd_buffer[j] = '*';
		lcd_buffer[j + 1] = 0;

		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

		break;
	case 311:
		sprintf_P(lcd_buffer, PSTR("%s /%3u"), WiFiPass, (unsigned int)WiFiPass[nPopr]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		j = 0;
		while (j < nPopr)
		{
			lcd_buffer[j] = ' ';
			j++;
		}
		lcd_buffer[j] = '*';
		lcd_buffer[j + 1] = 0;

		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

		break;
	case 312:
		// последнее значение это тест фракционника
		if (nPopr == MAX_CNT_FRACTION_DIST)
		{
			sprintf_P(lcd_buffer, PSTR("Fraction Dist"));
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("Test =%3i"), (int)TekFraction + 1);
		}
		else
		{
			if (TempFractionDist[nPopr] >= 0)
				sprintf_P(lcd_buffer, PSTR("tFrDst(%1i)=%4i"), (int)nPopr + 1, (int)TempFractionDist[nPopr]);
			else
				sprintf_P(lcd_buffer, PSTR("timFD(%1i)=%5im"), (int)nPopr + 1, (int)-TempFractionDist[nPopr]);

			if (flPopr == 0)
			{
				sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
			}

			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD

			if (flPopr == 1)  sprintf_P(lcd_buffer, PSTR("Angl=%3i*,%4iW"), (int)AngleFractionDist[nPopr], PowerFractionDist[nPopr]);
			else  sprintf_P(lcd_buffer, PSTR("Angl=%3i ,%4iW"), (int)AngleFractionDist[nPopr], PowerFractionDist[nPopr]);

			if (flPopr == 2)  sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
		}
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 313:
		// последнее значение это тест фракционника
		if (nPopr == MAX_CNT_FRACTION_RECT)
		{
			sprintf_P(lcd_buffer, PSTR("Fraction Rect"));
			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("Test =%3i"), (int)TekFraction + 1);
		}
		else
		{
			if (TempFractionRect[nPopr] >= 0)
				sprintf_P(lcd_buffer, PSTR("tFrRect(%1i)=%4i"), (int)nPopr + 1, (int)TempFractionRect[nPopr]);
			else
				sprintf_P(lcd_buffer, PSTR("timFR(%1i)=%5im"), (int)nPopr + 1, (int)-TempFractionRect[nPopr]);

			if (flPopr == 0)
			{
				sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
			}

			my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
			sprintf_P(lcd_buffer, PSTR("Angle=%3i"), (int)AngleFractionRect[nPopr]);

			if (flPopr == 1)  sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
		}
		lcd.setCursor(0, 1);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 314:
		sprintf_P(lcd_buffer, PSTR("%4i %4i %4i"), (int)PowerPhase[0], (int)PowerPhase[1], (int)PowerPhase[2]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		if (nPopr == 0) sprintf_P(lcd_buffer, PSTR(" PH1"));
		if (nPopr == 1) sprintf_P(lcd_buffer, PSTR("      PH2"));
		if (nPopr == 2) sprintf_P(lcd_buffer, PSTR("           PH3"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
	case 315:
		sprintf_P(lcd_buffer, PSTR("%3i %3i %3i=%3i"), (int)KtPhase[0], (int)KtPhase[1], (int)KtPhase[2], (int)KtPhase[0] + KtPhase[1] + KtPhase[2]);
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		if (nPopr == 0) sprintf_P(lcd_buffer, PSTR("PH1"));
		if (nPopr == 1) sprintf_P(lcd_buffer, PSTR("    PH2"));
		if (nPopr == 2) sprintf_P(lcd_buffer, PSTR("        PH3"));
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
#if ENABLE_SENSOR_SORTING
	case 316:
		// Печатаем номера всех наших сенсоров
		for(int i = 0, n = 0; i < MAX_DS1820; i++) {
			n += sprintf_P(lcd_buffer + n, (i>0)?PSTR(" %02d"):PSTR("%02d"), (int)ds1820_nums[i] + 1);
		}
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		lcd.setCursor(0, 1);
		j=0;
		while (j<(nPopr)*3+1)
		{
			lcd_buffer[j]=' ';
			j++;
		}
		lcd_buffer[j]='*';
		lcd_buffer[j+1]=0;
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
		break;
#endif
	default:
		break;
		}

	//    
	//  lcd.setCursor(0, 0);
	//  my_lcdprint(Seconds);
	//  lcd.setCursor(5, 0);
	//  my_lcdprint(MaxVoltsOut);
	//  lcd.setCursor(9, 0);
	//  my_lcdprint((long)MaxVoltsOut*707/1000);
	//  lcd.setCursor(0, 1);
	//  my_lcdprint(index_input);
	//  lcd.setCursor(5, 1);
	//  my_lcdprint(TimeOpenTriac);
	//  NeedDisplaying=false;
	//  lcd.setCursor(10, 1);
	//  my_lcdprint(TicZero);
	// предупреждения теперь выводим на всех страницах.

	if (flNumStrPrint < 2)
	{
		lcd_buffer[0] = 0;
		my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
	}
	NeedDisplaying = false;

}
