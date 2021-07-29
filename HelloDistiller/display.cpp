// Последнее обновление 2018-07-25 by Phisik
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Здесь всяческий вывод на экран
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "configuration.h"
#include "declarations.h"

char flNumStrPrint = 0;

#if SHOW_FLOAT_TEMPERATURES
#define PRINT_TEMPERATURES()                                                                                                                           \
    {                                                                                                                                                  \
        String msg = "T=" + String(0.1 * DS_TEMP(TEMP_KUB), 1) + "," + String(0.1 * DS_TEMP(TEMP_DEFL), 1) + "," + String(0.1 * DS_TEMP(TEMP_TSA), 1); \
        strcpy(lcd_buffer, msg.c_str());                                                                                                               \
        my_lcdprint(lcd_buffer);                                                                                                                       \
    }
#else
#define PRINT_TEMPERATURES()                                                                                                                                                                                     \
    {                                                                                                                                                                                                            \
        sprintf_P(lcd_buffer, PSTR("C2 T=%i.%i,%i.%i,%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, DS_TEMP(TEMP_TSA) / 10, DS_TEMP(TEMP_TSA) % 10); \
        my_lcdprint(lcd_buffer);                                                                                                                                                                                 \
    }
#endif

void my_lcdprint(char* s)
{
    char i;
    char len;
    len = strlen(lcd_buffer);
    for (i = len; i < LCD_WIDTH; i++)
        s[i] = ' ';
    s[LCD_WIDTH] = 0;

    if (IspReg == 111 && s[9] == 'R' && flNumStrPrint == 0)
        s[9] = 'N';

    lcd.print(s);

    // MQTT code by max506 & limon
#if USE_MQTT_BROKER

    if (flNumStrPrint == 0) {

        snprintf_P(lcd_mqtt_buf1, MQTT_BUFFER_SIZE, fmt_lcd1, s);

        // Убрать завершающие пробелы
        for (i = strlen(lcd_mqtt_buf1) - 1;; i--) {
            if (lcd_mqtt_buf1[i] == ' ')
                lcd_mqtt_buf1[i] = '\0';
            else
                break;
        }
    } else if (flNumStrPrint == 1) {
        snprintf_P(lcd_mqtt_buf2, MQTT_BUFFER_SIZE, fmt_lcd2, s);

        // Убрать завершающие пробелы
        for (i = strlen(lcd_mqtt_buf2) - 1;; i--) {
            if (lcd_mqtt_buf2[i] == ' ')
                lcd_mqtt_buf2[i] = '\0';
            else
                break;
        }
    }
#endif // USE_MQTT_BROKER

#if USE_GSM_WIFI == 1

    // Если состояние отправки - надо отправить экран, тогда отправляем еще и на сервер.
    if (flGPRSState == 20 || flGPRSState == 164) {
        if (flNumStrPrint == 0) { // Первой строчкой отправляем id устройства и первую строку экрана
            sprintf_P(my_gprs_buffer, PSTR("%s;%s"), idDevice, s);
            GSM_SERIAL.print(my_gprs_buffer);
#ifdef TESTGSM
            DEBUG_SERIAL.println(F("SendDTA1:"));
            DEBUG_SERIAL.println(my_gprs_buffer);
#endif

        } else { // Второй строчкой отправляем нужную часть экрана, состояние контроллера и последнюю выполненную команду.
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
char str_temp[6];
char str_cur[8];
char str_off[8];

void formTRZGDistill()
{
    if (TempDeflBegDistil > 0) {
        sprintf_P(lcd_buffer, PSTR("Te\274\276ep-pa K\171\262a= %i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10); //Темпер-ра Куба=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), Power); //Вых.мощность=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Te\274\276.\340e\344a= %i.%i/%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, TempDeflBegDistil / 10, TempDeflBegDistil % 10);
    } //Темп.Дефа=
    else {
        sprintf_P(lcd_buffer, PSTR("Te\274\276.\340e\344\273e\264\274. = %i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
        my_lcdprint(lcd_buffer); //Темп.Дефлегм.=
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), Power); //Вых.мощность=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a= %i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, -TempDeflBegDistil / 10, -TempDeflBegDistil % 10);
    } //Темп.Куба =
}

void formTSAErr()
{
    sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \250PEB\256\254EHA"), hour, minute, second); //00:00:00 ПРЕВЫШЕНА
    my_lcdprint(lcd_buffer);
    lcd.setCursor(0, 3);
    sprintf_P(lcd_buffer, PSTR("Te\274\276-pa TCA=%i.%i/%i.%i"), DS_TEMP(TEMP_TSA) / 10, DS_TEMP(TEMP_TSA) % 10, MAX_TEMP_TSA / 10, MAX_TEMP_TSA % 10);
    my_lcdprint(lcd_buffer); //Темп-ра ТСА=
    lcd.setCursor(0, 2);
    sprintf_P(lcd_buffer, PSTR("TEM\250EPAT\251PA TCA!")); //ТЕМПЕРАТУРА ТСА!
}

void DisplayRectif()
{
    if (DispPage == 0) {
        switch (StateMachine) {
        case 0:
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            if (IspReg != 118) {
                sprintf_P(lcd_buffer, PSTR("                    "));
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("    PEKT\245\252\245KA\341\245\245")); //РЕКТИФИКАЦИИ
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("       PE\243\245M")); //РЕЖИМ
            } else {
                sprintf_P(lcd_buffer, PSTR("    PEKT\245\252\245KA\341\245\245")); //РЕКТИФИКАЦИИ
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("    \252PAK\341\245OHHO\246")); //ФРАКЦИОННОЙ
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("       PE\243\245M")); //РЕЖИМ
            }
            break;
        case 1: //Не запущено
            lcd.clear();
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("       \243\340\245TE \356"));
            my_lcdprint(lcd_buffer);
            break;
        case 2: //Разгон
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PA\244\241OH"), hour, minute, second); //РАЗГОН
            my_lcdprint(lcd_buffer);
            if (tEndRectRazgon > 0) {
                sprintf_P(lcd_buffer, PSTR("\272\171\262a = %i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, tEndRectRazgon / 10, tEndRectRazgon % 10);
                my_lcdprint(lcd_buffer); //куба =
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("Te\274\276.o\272o\275\300.pa\267\264o\275a")); //Темп. оконч. разгона
            } else {
                sprintf_P(lcd_buffer, PSTR("\272o\273o\275\275\303 =  %i.%i/%i.%i"), DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, -tEndRectRazgon / 10, -tEndRectRazgon % 10);
                my_lcdprint(lcd_buffer); //колонны =
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("Te\274\276.o\272o\275\300.pa\267\264o\275a")); //Темп. оконч. разгона
            }
            break;

        case 3: //Стабилицация колонны
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PA\240OTA"), hour, minute, second); //00:00:00 РАБОТА
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4iW"), PowerRect); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("PK(20) = %i.%i(%3i)"), DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, (IspReg == 111 ? (int)(SecOstatok) : (int)(Seconds - SecTempPrev)));
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("KO\247OHH\256 HA CE\240\261 \356")); //КОЛОННЫ НА СЕБЯ (шестигранник)
            break;
        case 4: //Отбор голов
            if (IspReg != 118) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OT\240OP \241O\247OB"), hour, minute, second); //00:00:00 ОТБОР ГОЛОВ
                my_lcdprint(lcd_buffer);
                if (UrovenProvodimostSR == 0) {
                    sprintf_P(lcd_buffer, PSTR("\277e\274\276.K\171\262a= %i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, tEndRectOtbGlv / 10, tEndRectOtbGlv % 10); //Вых.мощность=
                    my_lcdprint(lcd_buffer); //темп.Кубa=
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304=  %4iW"), PowerRect); //Вых.мощность=
                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("O\272o\275\300a\275\270e o\277\262opa \276o")); //Окончание отбора по
                } else {
                    if (UrovenProvodimostSR > 0) {
                        if (UrovenProvodimostSR == 2) {
                            sprintf_P(lcd_buffer, PSTR("\343a\277\300\270\272\171= %3i/%3i"), U_GLV, UROVEN_ALARM); //датчику =
                            my_lcdprint(lcd_buffer);
                            lcd.setCursor(0, 3);
                            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304= %4iW"), PowerRect); //Вых.мощность=
                            my_lcdprint(lcd_buffer);
                            sprintf_P(lcd_buffer, PSTR("O\272o\275\300.\276o a\275a\273o\264o\263o\274\171")); //Оконч.по аналоговому
                        } else {
                            sprintf_P(lcd_buffer, PSTR("\343a\277\300\270\272\171= %3i/%3i"), U_GLV, UrovenProvodimostSR); //датчику =
                            my_lcdprint(lcd_buffer);
                            lcd.setCursor(0, 3);
                            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304= %4iW"), PowerRect); //Вых.мощность=
                            my_lcdprint(lcd_buffer);
                            sprintf_P(lcd_buffer, PSTR("O\272o\275\300.\276o \345\270\344po\263o\274\171")); //Оконч.по цифровому
                        }
                    } else {
                        sprintf_P(lcd_buffer, PSTR("\270c\277.\263pe\274e\275\270=%3i/%3i"), (int)SecOstatok, -UrovenProvodimostSR); //ист.времени=
                        my_lcdprint(lcd_buffer);
                        lcd.setCursor(0, 3);
                        sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304= %4iW"), PowerRect); //Вых.мощность=
                        my_lcdprint(lcd_buffer);
                        sprintf_P(lcd_buffer, PSTR("O\272o\275\300a\275\270e o\277\262opa \276o")); //Окончание отбора по
                    }
                }
            } else {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OT\240OP \241O\247OB"), hour, minute, second); //00:00:00 ОТБОР ГОЛОВ
                my_lcdprint(lcd_buffer);

                if (TekFraction < CountFractionRect)
                    V2 = TempFractionRect[TekFraction];
                if (V2 >= 0) {
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a=%i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, V2 / 10, V2 % 10);
                    my_lcdprint(lcd_buffer); //Темп.Куба=
                    sprintf_P(lcd_buffer, PSTR("\252pa\272\345\270\307 = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
                } else {
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("O\272o\275\300.o\277\262opa:%5i"), (int)SecOstatok); //Оконч.отбора:
                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a=%i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, V2 / 10, V2 % 10);
                } //Темп.Куба=
            }
            break;
        case 5: //Стоп, ожидание возврата температуры
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PEKT.<CTO\250>"), hour, minute, second); //00:00:00 РЕКТ.<СТОП>
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("o\277\262opa C\250\245PTA =%2i%%"), ProcChimSR); //отбора СПИРТА =
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Te\272\171\346\270\271 \276po\345e\275\277")); //Текущий процент
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.Ko\273=%i.%i/%i.%i"), DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, tStabSR / 10, tStabSR % 10);
            break; //Темп.Kол=
        case 6: //Ректификация
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OT\240OP C\250\245PT"), hour, minute, second); //00:00:00 ОТБОР СПИРТ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("\340a\263\273=%i.%i,Mo\346\275=%4iW"), U_MPX5010 / 10, U_MPX5010 % 10, FactPower);
            my_lcdprint(lcd_buffer); //Давл=    ,Мощн=
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("PK(20)=%i.%i/%i.%i %i.%i"), DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, (tStabSR + tDeltaRect) / 10, (tStabSR + tDeltaRect) % 10, tDeltaRect / 10, tDeltaRect % 10);
            my_lcdprint(lcd_buffer); //PK(20)=
            sprintf_P(lcd_buffer, PSTR("\250po\345e\275\277 o\277\262opa = %2i%%"), ProcChimSR); //Процент отбора =
            break;
        case 7: //Отбор хвостов
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OT\240OP XBOCT"), hour, minute, second); //00:00:00 ОТБОР ХВОСТ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e \263 K\171\262e=%i.%i"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление в Кубе=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a=%i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, (IspReg != 118 ? tEndRect : TekTemp) / 10, (IspReg != 118 ? tEndRect : TekTemp) % 10);
            break; //Темп.Куба=
        case 8: //Отбор ожидание 3 минуты
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OX\247A\243\340EH\245E"), hour, minute, second); //00:00:00 ОХЛАЖДЕНИE
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a= %i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, (IspReg != 118 ? tEndRect : TekTemp) / 10, (IspReg != 118 ? tEndRect : TekTemp) % 10);
            my_lcdprint(lcd_buffer); //Темп.Куба=
            sprintf_P(lcd_buffer, PSTR("O\240OP\251\340OBAH\245\261...")); //ОБОРУДОВАНИЯ...
            break;
        case 9: //Ожидание датчика уровня
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u O\243\245\340AH\245E"), hour, minute, second); //00:00:00 ОЖИДАНИЕ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 =%3i/%3i"), U_UROVEN, UROVEN_ALARM); //Уровень =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\340AT\253\245KA \251POBH\261 \356")); //ДАТЧИКА УРОВНЯ
            break;
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \250PO\341ECC"), hour, minute, second); //00:00:00 ПРОЦЕСС
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a= %i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, tEndRect / 10, tEndRect % 10);
            my_lcdprint(lcd_buffer); //Темп.Куба =
            sprintf_P(lcd_buffer, PSTR("PEKT\245\252\245KA\341\245\245 OKOH\253EH")); //РЕКТИФИКАЦИИ ОКОНЧЕН
            break;
        case 101: //Температура в ТСА превысила предельную
            formTSAErr();
            break;
        case 102: //Давление
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340a\263\273e\275\270e"), hour, minute, second); //00:00:00 Давление
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("MPX5010 = %i.%i/%i.%i"), U_MPX5010 / 10, U_MPX5010 % 10, AlarmMPX5010 / 10, AlarmMPX5010 % 10);
            break;
        }
    } else {
        PRINT_TEMPERATURES()
        if (StateMachine != 5) {
            if (IspReg != 118) {
                // Phisik on 2018-07-06 I switched off the 3d screen and place power on the 2nd
                sprintf_P(lcd_buffer, PSTR("B\303x.\277o\272 =  %i.%iA"), (uint16_t)MaxIOut / 10, (uint16_t)MaxIOut % 10); //Вых.ток =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("Mo\346\275oc\277\304 = %4iW"), FactPower); //Мощность =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("Ha\276p\307\266e\275\270e =%3uV"), (uint16_t)MaxVoltsOut); //Напряжение =
            } else if (TekFraction < CountFractionRect) {
                if (TempFractionRect[TekFraction] > 0) {
                    sprintf_P(lcd_buffer, PSTR("Te\274\276.o\272o\275\300a\275\270\307=%i.%i"), TempFractionRect[TekFraction] / 10, TempFractionRect[TekFraction] % 10);
                    my_lcdprint(lcd_buffer); //Темп.окончания=
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e \263 K\171\262e=%i.%i"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление в Кубе=
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("\252pa\272\345\270\307 = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
                } else {
                    sprintf_P(lcd_buffer, PSTR("Bpe\274\307 oc\277a\273oc\304: %4im"), (int)SecOstatok); //Время осталось:
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e \263 K\171\262e=%i.%i"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление в Кубе=
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("\252pa\272\345\270\307 = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
                }
            } else {
                sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \263 Tape=%4u"), U_UROVEN); //Уровень в Таре =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e \263 K\171\262e=%i.%i"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление в Кубе=
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("\252pa\272\345\270\307 = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
            }
        } else {
            sprintf_P(lcd_buffer, PSTR("Bpe\274\307 Pec\277a\262. = %5i"), time1); //Время Рестаб. =
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e \263 K\171\262e=%i.%i"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление в Кубе=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("\252pa\272\345\270\307 = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
        }
    }
}

void DisplayDistDefl()
{
    if (DispPage == 0) {
        switch (StateMachine) {
        case 0: //Не запущено
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("  C \340E\252\247E\241MATOPOM")); //С ДЕФЛЕГМАТОРОМ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
            break;
        case 1:
            lcd.clear();
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("       \243\340\245TE \356"));
            my_lcdprint(lcd_buffer);
            break;
        case 2: //Ожидание закипания (прогреется дефлегматор)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PA\244\241OH"), hour, minute, second); //00:00:00 РАЗГОН
            my_lcdprint(lcd_buffer);
            formTRZGDistill();
            break;
        case 3: //Ожидание закипания (прогреется куб дефлегматор)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PA\244\241OH"), hour, minute, second); //00:00:00 РАЗГОН
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\276oc\273e \340e\344a=%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
            my_lcdprint(lcd_buffer); //Темп.после Дефa=
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a=%i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, TempDeflBegDistil / 10, TempDeflBegDistil % 10);
            break; //Темп.Куба=
        case 4: //Работа без дефлегматора
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\245CT\245\247\247\261\341\245\261"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\340e\344a= %i.%i/%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, TempDefl / 10, TempDefl % 10);
            my_lcdprint(lcd_buffer); //Темп.Дефа=
            sprintf_P(lcd_buffer, PSTR("\240E\244 \340E\252\247E\241MATOPA")); //БЕЗ ДЕФЛЕГМАТОРА
            break;
        case 5: //Работа с 50% дефлегмацией
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\245CT\245\247\247\261\341\245\261"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\340e\344a= %i.%i/%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, (TempDefl - DeltaDefl) / 10, (TempDefl - DeltaDefl) % 10);
            my_lcdprint(lcd_buffer); //Темп.Дефа=
            sprintf_P(lcd_buffer, PSTR("C \340E\252\247E\241MA\341\245E\246 50%%")); //С ДЕФЛЕГМАЦИЕЙ 50%
            break;
        case 6: //Работа с 100% дефлегмацией
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\245CT\245\247\247\261\341\245\261"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\340e\344a=%i.%i/%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, (TempDefl - DeltaDefl) / 10, (TempDefl - DeltaDefl) % 10);
            my_lcdprint(lcd_buffer); //Темп.Дефа=
            sprintf_P(lcd_buffer, PSTR("C \340E\252\247E\241MA\341\245E\246 100%%")); //С ДЕФЛЕГМАЦИЕЙ 100%
            break;
        case 7: //Ожидание для охлаждения
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OKOH\253AH\245E"), hour, minute, second); //00:00:00 ОКОНЧАНИЕ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\276oc\273e \340e\344a=%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
            my_lcdprint(lcd_buffer); //Темп.после Дефa=
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("o\262op\171\343o\263a\275\270\307 \356 %4i"), SecondsEnd); //оборудования (шестигранник)
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("Ox\273a\266\343e\275\270e")); //Охлаждение
            break;
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\245CT\245\247\247\261\341\245\261"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Te\274\276ep-pa K\171\262a=%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10);
            my_lcdprint(lcd_buffer); //Темпер-ра Куба =
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("OKOH\253EHA")); //ОКОНЧЕНА
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("C \340E\252\247E\241MATOPOM"), hour, minute, second); //С ДЕФЛЕГМАТОРОМ
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("Pa\267-\345a Mo\346\275oc\277\270=%4iW"), deltaPower); //Раз-ца Мощности=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \263 Tape =%4u"), U_UROVEN); //Уровень в Таре =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Po\267\273\270\263 Bo\343\303 =   %4u"), U_VODA); //Розлив Воды =
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
 my_lcdprint(lcd_buffer); 
 formTRZGDistill();
 break;
 case 2: //Ожидание закипания (прогреется дефлегматор)
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sGl Rzg"),hour,minute,second);
 my_lcdprint(lcd_buffer); 
 formTRZGDistill();
 break;
 case 3: //Ожидание закипания (прогреется куб дефлегматор)
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sGl Rzg"),hour,minute,second);
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),TempDeflBegDistil,Power);
 break;
 case 4: //Отбор голов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sGl Otb"),hour,minute,second);
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),Temp3P,PowerGlvDistil);
 break;
 case 5: //Ожидание 60 секунд для охлаждения
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sOtbGlv"),hour,minute,second);
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("tK=%3i Wait=%4i"),DS_TEMP(TEMP_KUB),SecondsEnd);
 break;
 case 100: //Окончание
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sOtbGlv"),hour,minute,second);
 my_lcdprint(lcd_buffer); 
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
    if (DispPage == 0) {
        if (IspReg == 104) {
            V1 = 1;
            V2 = Temp1P;
        }
        if (IspReg == 106) {
            V1 = 2;
            V2 = Temp2P;
        }
        if (IspReg == 107 || IspReg == 105) {
            V1 = 3;
            V2 = Temp3P;
        }

        //		if (IspReg != 105)  sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u %1u\246-OT\240OP"), hour, minute, second, V1);     //00:00:00 Й-ОТБОР
        //		else  sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OT\240OP \241O\247OB"), hour, minute, second);                   //00:00:00 ОТБОР ГОЛОВ

        switch (StateMachine) {
        case 0: //Не запущено
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            if (IspReg != 105) {
                sprintf_P(lcd_buffer, PSTR("     %1u-\246 OT\240OP "), V1); //№-Й ОТБОР
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
            } else {
                sprintf_P(lcd_buffer, PSTR("    OT\240OP \241O\247OB")); //ОТБОР ГОЛОВ
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
            }
            break;
        case 1:
            lcd.clear();
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("       \243\340\245TE \356"));
            my_lcdprint(lcd_buffer);
            break;
        case 2: //Ожидание закипания (прогреется дефлегматор)
            if (IspReg != 105)
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u %1u-O PA\244\241OH"), hour, minute, second, V1); //00:00:00 1-О РАЗГОН
            else
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OT\240OP \241O\247OB"), hour, minute, second); //00:00:00 ОТБОР ГОЛОВ
            my_lcdprint(lcd_buffer);
            formTRZGDistill();
            break;
        case 3: //Дистилляция
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\245CT\245\247\247\261\341\245\261"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\276oc\273e \340e\344a=%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
            my_lcdprint(lcd_buffer); //Темп.после Дефa=
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a= %i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, V2 / 10, V2 % 10);
            break; //Темп.Куба=
        case 4: //Ожидание 60 секунд для охлаждения
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OX\247A\243\340EH\245E"), hour, minute, second); //00:00:00 ОХЛАЖДЕНИE
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\276oc\273e \340e\344a=%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
            my_lcdprint(lcd_buffer); //Темп.после Дефa=
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("\243\343\270\277e: %4i"), SecondsEnd); //Ждите:
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("O\240OP\251\340OBAH\245\261...")); //ОБОРУДОВАНИЯ...
            break;
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\245CT\245\247\247\261\341\245\261"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276ep-pa K\171\262a= %i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10); //Темпер-ра Куба=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("OKOH\253EHA")); //ОКОНЧЕНА
            break;
        case 101: //Температура в ТСА превысила предельную
            formTSAErr();
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e(MPX)=%i.%imm"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление(MPX)=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \263 Tape =%4u"), U_UROVEN); //Уровень в Таре =
    }
}

void DisplayFracionDistill()
{
    if (DispPage == 0) {
        sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\245CT.\252P.%1i/%1i"), hour, minute, second, (int)(TekFraction < CountFractionDist) ? TekFraction + 1 : CountFractionDist, (int)CountFractionDist);
        //00:00:00 ДИСТ.ФР.1/2
        switch (StateMachine) {
        case 0: //Не запущено
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("  C \252PAK\341\245OHH\245KOM")); //С ФРАКЦИОННИКОМ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
            break;
        case 1:
            lcd.clear();
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("       \243\340\245TE \356"));
            my_lcdprint(lcd_buffer);
            break;
        case 2: //Ожидание закипания (прогреется дефлегматор)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\245CT.\252P.PA\244"), hour, minute, second); //00:00:00 ДИСТ.ФР.РАЗ
            my_lcdprint(lcd_buffer);
            formTRZGDistill();
            break;
        case 3: //Дистилляция
            my_lcdprint(lcd_buffer);
            if (TekFraction < CountFractionDist)
                V2 = TempFractionDist[TekFraction];
            if (V2 >= 0) {
                sprintf_P(lcd_buffer, PSTR("Te\274\276.\340e\344\273e\264\274. = %i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
                my_lcdprint(lcd_buffer); //Темп.Дефлегм.=
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность =
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Te\274\276.K\171\262a= %i.%i/%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, V2 / 10, V2 % 10);
            } //Темп.Куба=
            else {
                sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4iW"), UstPower); //Вых.мощность =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("Oc\277a\273oc\304: %5i \274\270\275."), (int)SecOstatok); //Осталось: % мин.
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Te\274\276ep-pa K\171\262a=%i.%i)"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10);
            } //Темпep-pa Куба=
            break;
        case 4: //Ожидание 60 секунд для охлаждения
            sprintf_P(lcd_buffer, PSTR("Te\274\276ep-pa K\171\262a=%i.%i)"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10);
            my_lcdprint(lcd_buffer); //Темпep-pa Куба=
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("OCTA\247OC\304 -%4i ce\272."), time3); //ОСТАЛОСЬ -
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("OX\247A\243\340.O\240OP\251\340OBAH\245\261")); //ОХЛАЖД.ОБОРУДОВАНИЯ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\340e\344\273e\264\274.=%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
            break; //Темп.Дефлегм.=
        case 100: //Окончание
            if (StateMachine == 100) {
                sprintf_P(lcd_buffer, PSTR("       \250PO\341ECC")); //ПРОЦЕСС
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("      \244AKOH\253EH")); //ЗАКОНЧЕН
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("     \340\245CT\245\247\247\261\341\245\245")); //ДИСТИЛЛЯЦИИ
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("     \252PAK\341\245OHHO\246")); //ФРАКЦИОННОЙ
            }
            break;
        case 101: //Температура в ТСА превысила предельную
            formTSAErr();
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \263 Tape =%4u"), U_UROVEN); //Уровень в Таре =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4uW"), UstPower); //Вых.мощность =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("Po\267\273\270\263 Bo\343\303 =%4u"), U_VODA); //Розлив Воды =
    }
}

void DisplayRazvar()
{
    if (DispPage == 0) {
        if (IspReg == 114)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OX\247.\244ATOPA"), hour, minute, second); //02u:%02u:%02 ОХЛ.ЗАТОРА
        else
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PA\244BAP \244EPH"), hour, minute, second); //02u:%02u:%02 РАЗВАР ЗЕРН
        my_lcdprint(lcd_buffer);

        switch (StateMachine) {
        case 0:
            sprintf_P(lcd_buffer, PSTR("Te\274\276ep-pa \263o\343\303=%i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10);
            my_lcdprint(lcd_buffer); //Темпер-ра воды=
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4iW"), UstPower); //Вых.мощность =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("O\266\270\343a\275\270e \263\272\273\306\300e\275\270\307 \356")); //Ожидание включения
            break;
        case 1: //Нагрев до температуры 50 градусов
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\263o\343\303= %i.%i/%i.0"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempZSP);
            my_lcdprint(lcd_buffer); //Темп.воды=
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), UstPower); //Вых.мощность =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Ha\264pe\263 \263o\343\303...")); //Нагрев воды...
            break;
        case 2: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
            sprintf_P(lcd_buffer, PSTR("\244ac\303\276\304 \267ep\275o \270 \275a\266\274\270")); //Засыпь зерно и нажми
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("\272\275o\276\272\171 <BH\245\244>")); //кнопку <ВНИЗ>
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263o\343\303 = %i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп-ра воды =
            break;
        case 3: //Нагрев до температуры 64 градуса
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4iW"), UstPower); //Вых.мощность =
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274.\267a\277opa=%i.%i/%i.0"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempZSPSld);
            break; //Тем.затора=
        case 4: //Ожидание 15 минут, поддержка температуры
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4iW"), UstPower); //Вых.мощность =
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Oc\277a\273oc\304 - %4i"), time2); //Осталось -
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\267a\277opa = %i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп.затора =
            break;
        case 5: //Нагрев до кипения
            if (ds1820_devices < 2) {
                sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4iW"), UstPower); //Вых.мощность =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("\267a\277opa = %i.%i/%i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, TempKipenZator / 10, TempKipenZator % 10);
                my_lcdprint(lcd_buffer); //затора =
                sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277\171pa"));
            } //Температура
            else {
                sprintf_P(lcd_buffer, PSTR("\267a\277opa = %i.%i/%i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, TempKipenZator / 10, TempKipenZator % 10);
                my_lcdprint(lcd_buffer); //затора =
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4iW"), UstPower); //Вых.мощность =
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277\171pa \272\270\276e\275\270\307")); //Температура кипения
            }
            break;
        case 6: //Варка
            sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 =%4iW"), PowerRazvZerno); //Вых.мощность =
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Oc\277a\273oc\304: %4i"), time2); //Осталось:
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\267a\277opa = %i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп.затора =
            break;
        case 7: //Охлаждение до температыры осахаривания
            sprintf_P(lcd_buffer, PSTR("Ox\273a\266\343e\275\270e \343o \277e\274\276.")); //Охлаждение до темп.
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("ocaxap\270\263a\275\270\307 \356")); //осахаривания
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274.\267a\277opa=%i.%i/%i.0"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempZSPSld);
            break; //Тем.затора=
        case 8: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
            sprintf_P(lcd_buffer, PSTR("\244ac\303\276\304 co\273o\343 \270 \275a\266\274\270")); //Засыпь солод и нажми
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("\272\275o\276\272\171 <BH\245\244>")); //кнопку <ВНИЗ>
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263o\343\303 = %i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп-ра воды =
            break;
        case 9: //Мешаем 10 минут
            sprintf_P(lcd_buffer, PSTR("Pa\262o\277a \274e\301a\273\272\270 - %4i"), time2); //Работа мешалки =
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\267a\277opa = %i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп.затора =
            break;
        case 10: //Осахаривание
            sprintf_P(lcd_buffer, PSTR("\250po\345ecc ocaxap\270\263a\275\270\307")); //Процесс осахаривания
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("oc\277a\273oc\304 - %4i"), time2); //осталось -
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\267a\277opa = %i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп.затора =
            break;
        case 11: //Охлаждение до температуры первичного внесения дрожжей осахаривания
            sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277\171pa")); //Температура
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("\267a\277opa = %i.%i/40,0"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //затора =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Ox\273.\343o \263\275ec.\343po\266\266e\271")); //Охл.до внес.дрожжей
            break;
        case 12: //Охлаждение до температыры осахаривания
            sprintf_P(lcd_buffer, PSTR("Ox\273a\266\343e\275\270e \343o \277e\274\276.")); //Охлаждение до темп.
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("ocaxap\270\263a\275\270\307 \356")); //осахаривания
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274.\267a\277opa=%i.%i/%i.0"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempHLDZatorBrog1);
            break; //Тем.затора =
        case 13: //Поддержка брожения, ничего не делаем, только мешаем периодически
            sprintf_P(lcd_buffer, PSTR("\245\343\265\277 \262po\266e\275\270e \356")); //Идет брожение (знак многогранника)
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274.\267a\277opa=%i.%i/%i.0"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempHLDZatorBrog1);
            break; //Тем.затора =
        case 14: //Поддержка брожения - охлаждение
            sprintf_P(lcd_buffer, PSTR("\250o\343\343ep\266a\275\270e \262po\266e\275\270\307")); //Поддержание брожения
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("- ox\273a\266\343e\275\270e \356")); //- охлаждение (знак многогранника)
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274.\267a\277opa=%i.%i/%i.5"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempHLDZatorBrog1);
            break; //Тем.затора =
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("               KOHE\341")); //КОНЕЦ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\267a\277opa = %i.%i"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп.затора =
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277\171pa \267ac\303\276\270")); //Температура засыпи
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("co\273o\343a = %i.0"), (int)TempZSPSld); //солода =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 H\250\241 =%4u"), U_NPG); //Уровень НПГ =
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
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("tK=%3i P=%4i"),DS_TEMP(TEMP_KUB),Power);
 break;
 case 2: //Разгон
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND RZG"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); 
 if (tEndRectRazgon>0) sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),tEndRectRazgon,UstPower);
 else sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i %4iW"),DS_TEMP(TEMP_RK20),-tEndRectRazgon,UstPower);
 break;
 case 3: //Стабилицация колонны
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND NSB"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("t=%3i(%4i)%4iW"),DS_TEMP(TEMP_RK20),(int)(SecOstatok),PowerRect);
 break;
 case 4: //Отбор голов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND GLV"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); 
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
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i %2i%%"),DS_TEMP(TEMP_RK20),tStabSR,ProcChimSR);
 break;
 case 6: //Ректификация
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u NDst %2i%%"),hour,minute,second,
);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i/d%2i"),DS_TEMP(TEMP_RK20),tStabSR+tDeltaRect,tDeltaRect);
 break;
 case 7: //Отбор хвостов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND Hvst"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i"),DS_TEMP(TEMP_KUB),tEndRect);
 break;
 case 8: //Отбор ожидание 3 минут
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND Wait"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i"),DS_TEMP(TEMP_KUB),tEndRect);
 break;
 case 9: //Ожидание датчика уровня
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u R Wait"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); 
 sprintf_P(lcd_buffer,PSTR("by Urov=%3i/%3i"),U_UROVEN,UROVEN_ALARM);
 break;
 case 100: //Окончание
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND End"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); 
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

void DisplayNBK() // ******************** Режим НБК *********************
{
    if (DispPage == 0) {
        switch (StateMachine) {
            lcd.clear();
        case 0: //Не запущено
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("  \240PA\243HO\246 KO\247OHHO\246")); //БРАЖНОЙ КОЛОННОЙ
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("    HE\250PEP\256BHO\246")); //НЕПРЕРЫВНОЙ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
            break;
        case 1:
            lcd.clear();
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("       \243\340\245TE \356"));
            my_lcdprint(lcd_buffer);
            break;
        case 2: //Ожидание закипания (прогреется дефлегматор)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u H\240K PA\244\241OH"), hour, minute, second); //00:00:00 НБК РАЗГОН
            my_lcdprint(lcd_buffer);
            formTRZGDistill();
            break;
        case 3: //Ожидание запуска
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u CTAPT H\240K"), hour, minute, second); //00:00:00 СТАРТ НБК
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\275a\266\274\270 \272\275o\276\272\171 <BBEPX>")); //нажми кнопку <ВВЕРХ>
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e(MPX)=%i.%imm"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление(MPX)=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\340\273\307 c\277ap\277a H\240K")); //Для старта НБК,
            break;
        case 4: //Запущено
        case 6: //Запущено, подача браги остановлена
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PA\240OTA H\240K"), hour, minute, second); //00:00:00 РАБОТА НБК
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e(MPX)=%i.%imm"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление(MPX)=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Mo\346\275oc\277\304 =%4uW"), UstPower); //Мощность =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("C\272opoc\277\304 Hacoca=%4i"), (int)SpeedNBK); //Скорость Насоса=
            break;
        case 5: //Превышение температуры вверху
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u H\240K"), hour, minute, second); //НБК
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Oc\277a\273oc\304 - %4i"), time2); //Осталось -
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("\250PEB\256\254EH\245E TEM\250.BEPX")); //ПРЕВЫШЕНИЕ ТЕМП.ВЕРХ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\340e\344a = %i.%i/97,0"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //Темп.Дефa=
            break;
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \250PO\341ECC"), hour, minute, second); //02u:%02u:%02 ПРОЦЕСС
            my_lcdprint(lcd_buffer);
            printf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e(MPX)=%i.%imm"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление(MPX)=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("C\272opoc\277\304 Hacoca=%4i"), (int)SpeedNBK); //Скорость Насоса=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\250EPE\241OHK\245 OKOH\253EH")); //ПЕРЕГОНКИ ОКОНЧЕН
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("\274a\272c.\267a \276ep\270o\343=%i.%i"), MaxPressByPeriod / 10, MaxPressByPeriod % 10); //макс.за период=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("Mo\346\275oc\277\304 =%4uW"), UstPower); //Мощность =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e(MPX)=%i.%imm,"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление(MPX)=
    }
}
void DisplayTestKLP() // *************** Тест клапанов *****************
{
    if (DispPage == 0) {
        sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TECT O\240OP-\261"), hour, minute, second); //00:00:00 ТЕСТ ОБОР-Я
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\254\245M o\277\272p.\272\273a\276.=%3i%%"), ProcChimSR); //ШИМ откр.клап.=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        if (StateMachine == 2)
            strcpy_P(str_off, PSTR("B\272\273."));
        else
            strcpy_P(str_off, PSTR("O\277\272\273"));
        sprintf_P(lcd_buffer, PSTR("Coc\277.\272\273a\276a\275o\263 > %s"), str_off); //Сост.клапанoв >
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("ds4=%i.%i, ds5=%i.%i"), DS_TEMP(3) / 10, DS_TEMP(3) % 10, DS_TEMP(4) / 10, DS_TEMP(4) % 10);
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("                    "));
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \241A\244A =%4u"), U_GAS); //Уровень ГАЗА =
    }
}

void DisplayExtContol() //**************** Внешнее управление *********************
{
    if (DispPage == 0) {
        sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u BHE\254.\251\250PAB\247"), hour, minute, second); //ВНЕШ.УПРАВЛ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\251c\277.Mo\346\275oc\277\304 =%4iW"), UstPower); //Уст.Мощность =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \340e\344a = %i.%i"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //Темп-ра Дефа =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Te\274\276ep-pa K\171\262a=%i.%i"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10); //Темпер-ра Куба =
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa DS(5)=%i.%i,"), DS_TEMP(4) / 10, DS_TEMP(4) % 10); //Темп-ра DS(5) =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \241A\244A=%4u"), U_GAS); //Уровень ГАЗА =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa DS(4)=%i.%i,"), DS_TEMP(3) / 10, DS_TEMP(3) % 10); //Темп-ра DS(4) =
    }
}

void DisplayBeer() // ********************* Пивоварение *********************
{
    if (DispPage == 0) {
        if (StateMachine == 0) {
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second); //00:00:00
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("    \250\245BOBAPEH\245E")); //ПИВОВАРЕНИЕ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("                    "));
        }
        if (StateMachine == 2) {
            if (KlTek == 1) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u HA\241PEB BO\340\256"), hour, minute, second); //00:00:00 НАГРЕВ ВОДЫ
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Hacoc/Me\301a\273\272a - %4i"), time1); //Hacoc/Мешалка -
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("Ma\272c.\274o\346\275oc\277\304 = %4iW"), PowerVarkaZerno); //Макс.мощность =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("Te\274\276.\263o\343\303 = %i.%i/%i"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10, (int)tempP[KlTek]);
            } //Темп.воды =
            else {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u BAPKA C\251C\247A"), hour, minute, second); //00:00:00 ВАРКА СУСЛА
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("C\273e\343\171\306\346a\307 \276a\171\267a-%i(%i)"), (int)KlTek, (int)CntPause); //Следующая пауза-
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("Ma\272c.\274o\346\275oc\277\304 = %4iW"), PowerVarkaZerno); //Макс.мощность =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("Te\274\276.c\171c\273a = %i.%i/%i"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10, (int)tempP[KlTek]);
            } //Темп.сусла =
        }
        if (StateMachine == 3) {
            if (timeP[KlTek] != 0) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TEM\250.\250A\251\244A"), hour, minute, second); //00:00:00 ТЕМП.ПАУЗА
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("\340o \272o\275\345a%2i-\271 \276a\171\267\303"), (int)KlTek); //До конца №-й паузы
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("oc\277a\273oc\304: %5i"), time2); //oсталось:
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("Te\274\276.c\171c\273a= %i.%i/%i"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10, (int)tempP[KlTek]);
            } //Tемперат.сусла=
            else {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TEXH\245\253ECKA\261"), hour, minute, second); //00:00:00 ТЕХНИЧЕСКАЯ
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("\275a\266\274\270 \272\275o\276\272\171 <BBEPX>")); //нажми кнопку <BBEPX>
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("\250a\171\267a - %i(%i)"), (int)KlTek + 1, (int)CntPause); //Пауза -
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("\250A\251\244A \356 \340\273\307 \276epexo\343a")); //ПАУЗА  Для перехода
            }
        }
        if (StateMachine == 100) {
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("      \244AKOH\253EHA"));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("        BAPKA"));
        }
    } else { //Tемперат.сусла=
        sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277.c\171c\273a= %i.%i"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e(MPX)=%i.%imm"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление(MPX)=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("B\303x.Mo\346\275oc\277\304 = %4iW"), UstPower); //Вых.Мощность =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("Hacoc/Me\301a\273\272a - %4i"), time1); //Hacoc/Мешалка -
    }
}

void DisplayData() //************ РЕЖИМ ПРОСМОТРА **********************
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
    else {
        // Если предыдущее состояние было 0, то даем задерку в 1/10 секунды, чтобы не слишком часто обновлялся
        // дисплей.
        if (PrevState == 0)
            delay(100);

        if (FlState != PrevState) {
            lcd.clear(); //очистка дисплея
            PrevState = FlState;
        }

        lcd.setCursor(0, 0);
    }

#ifdef TESTMEM
    DEBUG_SERIAL.println(F("\n[memCheck_displ]"));
    DEBUG_SERIAL.println(freeRam());
#endif

    switch (FlState) {
    case 0:

        if (DispPage < 2) // Первые две страницы отображаются по-разному, остальные всегда одни и те же
        {
            switch (IspReg) {
            case 101: // Displaying

                //  Выводим сетевое напряжение по тому же принципу, что и работают дешевые вольтметры, то есть максимальное количество вольт в сети умножаем на 0,707
                //  это напряжение нужно для калибровки нашего измерителя

                if (DispPage == 0) {
                    sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u \340\270c\276\273e\271 1/2"), hour, minute, second); //00:00:00 Дисплей 1/2
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("  homedistiller.ru"));
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("       forum."));
                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("U=%3iB, Zr=%4u(\277\270\272)"), (uint16_t)MaxVoltsOut, TicZero); //Напряжение =
                }

                if (DispPage == 1) {
                    PRINT_TEMPERATURES();
                    sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \241o\273o\263 = %4u"), U_GLV); //Уровень Голов =
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \263 Tape =%4u"), U_UROVEN); //Уровень в Таре =
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 1);
                    sprintf_P(lcd_buffer, PSTR("Po\267\273\270\263 Bo\343\303 =   %4u"), U_VODA); //Розлив Воды =
                }
                break;

            case 102: //****************** Термостат ************************
                if (DispPage == 0) {
                    sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TEPMOCTAT"), hour, minute, second);
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 3);
                    if (StateMachine == 2)
                        strcpy_P(str_off, PSTR("HA\241PEB"));
                    else
                        strcpy_P(str_off, PSTR("      "));
                    sprintf_P(lcd_buffer, PSTR("\340e\273\304\277a = %i.%i %s"), Delta / 10, abs(Delta % 10), str_off); //Дельта =
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("\277ep\274oc\277a\277a=%i.%i/%i.%i"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10, TempTerm / 10, TempTerm % 10);
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 1);
                    sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277\171pa pa\262o\277\303")); //Температура работы
                } else {
                    PRINT_TEMPERATURES();
                    sprintf_P(lcd_buffer, PSTR("\340a\263\273e\275\270e(MPX)=%i.%imm"), U_MPX5010 / 10, U_MPX5010 % 10); //Давление(MPX)=
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("B\303x.\274o\346\275oc\277\304 = %4iW"), PowerVarkaZerno); //Вых.мощность =
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 1);
                    sprintf_P(lcd_buffer, PSTR("ds4=%i.%i, ds5=%i.%i"), DS_TEMP(3) / 10, DS_TEMP(3) % 10, DS_TEMP(4) / 10, DS_TEMP(4) % 10);
                }
                break;

            case 115: //****************** Таймер ************************
                if (DispPage == 0) {
                    sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TA\246MEP:"), hour, minute, second, Delta); //00:00:00 ТАЙМЕР:
                    my_lcdprint(lcd_buffer);
                    minute = Seconds / 60;
                    sprintf_P(lcd_buffer, PSTR("\251c\277.Mo\346\275oc\277\304 =%4iW"), UstPower); //Уст.Мощность =
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("                    "));
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 1);
                    sprintf_P(lcd_buffer, PSTR("Bpe\274\307(\274) =%3i/%3i"), minute, timerMinute); //Время(м) =
                } else {
                    PRINT_TEMPERATURES();
                    sprintf_P(lcd_buffer, PSTR("\251c\277.Mo\346\275oc\277\304 =%4iW"), PowerMinute); //Уст.Мощность =
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("                    "));
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 1);
                    sprintf_P(lcd_buffer, PSTR("ds4=%i.%i, ds5=%i.%i"), DS_TEMP(3) / 10, DS_TEMP(3) % 10, DS_TEMP(4) / 10, DS_TEMP(4) % 10);
                } //ds4= , ds5=
                break;

            case 116: //******************* Пивоварня - клон браумастера **********************
                DisplayBeer();
                break;
            case 103: //******************** Регулятор мощности *************************
                if (DispPage == 0) {
#ifndef USE_SLAVE
                    sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u RMU=%3u"), hour, minute, second, (uint16_t)MaxVoltsOut);
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 3);
#if SIMPLED_VERSION == 20
                    sprintf_P(lcd_buffer, PSTR("B\303x.Mo\346\275.=%3u/%3u%%"), UstPowerReg / 10, Power / 10); //Вых.Мощность =
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("\267a \276o\273\171\276ep\270o\343 U=%3u"), indexOut); //за полупериод U =
                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("Ko\273\270\300ec\277\263o \263\303\262opo\272")); //Количество выборок
                    my_lcdprint(lcd_buffer);
#else
                    sprintf_P(lcd_buffer, PSTR("B\303x.Mo\346\275.=%4u/%4u"), FactPower, UstPowerReg); //Вых.Мощность =
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("\267a \276o\273\171\276ep\270o\343 U=%3u"), indexOut); //за полупериод U =
                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("Ko\273\270\300ec\277\263o \263\303\262opo\272")); //Количество выборок
                    my_lcdprint(lcd_buffer);
#endif
#ifdef TEST
                    if (NUM_PHASE > 1)
                        sprintf_P(lcd_buffer, PSTR("%s"), my_rx_buffer);
#endif
#else
                    sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u RMU=%3u"), hour, minute, second, (uint16_t)MaxVoltsOut);
                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("PSlv=%4u/%4u"), UstPower, Power);
#endif
                } else {
                    PRINT_TEMPERATURES();
#ifndef USE_SLAVE
                    lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("\272\275o\276\272\171 <BBEPX> %4uW"), Power); //кнопку <ВВЕРХ>
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("\274o\346\275oc\277\270 T\257HA,\275a\266a\277\304")); //мощности ТЭНА,нажать
                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("\340\273\307 a\263\277oo\276pe\343e\273e\275\270\307")); //Для автоопределения
                    my_lcdprint(lcd_buffer);
#else
                    if (flAutoDetectPowerTEN)
                        lcd.setCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("\272\275o\276\272\171 <BBEPX> %4uW"), Power); //кнопку <ВВЕРХ>
                    my_lcdprint(lcd_buffer);
                    lcd.setCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("\274o\346\275oc\277\270 T\257HA,\275a\266a\277\304")); //мощности ТЭНА,нажать
                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("\340\273\307 a\263\277oo\276pe\343e\273e\275\270\307")); //Для автоопределения
                    my_lcdprint(lcd_buffer);
                    else sprintf_P(lcd_buffer, PSTR("SlaveOn (Up)=%1i"), (int)SlaveON);
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
                //          my_lcdprint(lcd_buffer);
                //          sprintf_P(lcd_buffer,PSTR("Umax=%i Sec=%3i"),(uint16_t)MaxVoltsOut,(int) ErrCountIndex*12);
                break;
            case 249:
                sprintf_P(lcd_buffer, PSTR("HET \340ETEKOPA H\251\247\261!")); //НЕТ ДЕТЕКТОРА НУЛЯ!
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Umax=%i"), (uint16_t)MaxVoltsOut);
                break;
            case 250:
                sprintf_P(lcd_buffer, PSTR("PO\244\247\245B BO\340\256!")); //РОЗЛИВ ВОДЫ!
                my_lcdprint(lcd_buffer);
                break;
            case 251:
                sprintf_P(lcd_buffer, PSTR("O\254\245\240KA ds18b20!")); //ОШИБКА ds18b20!
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("HOMEP ds - %i"), (int)NumErrDs18);

                break;
            case 252:
                sprintf_P(lcd_buffer, PSTR("CPA\240OTA\247 \340AT\253\245K \241A\244A")); //СРАБОТАЛ ДАТЧИК ГАЗА!
                my_lcdprint(lcd_buffer);
                break;
            case 253:
                sprintf_P(lcd_buffer, PSTR("OC\251\254EH\245E H\250\241!")); //ОСУШЕНИЕ НПГ!
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("\253\270c\273o cpa\262o\277o\272 =%i"), countAlrmNPG); //Число сработок =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 H\250\241= %i"), U_NPG); //Уровень НПГ =
                break;
            case 254:
                sprintf_P(lcd_buffer, PSTR("H\250\241 \250EPE\250O\247HEH!")); //НПГ ПЕРЕПОЛНЕН!
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("\253\270c\273o cpa\262o\277o\272 =%i"), countAlrmNPG); //Число сработок =
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 H\250\241= %i"), U_NPG); //Уровень НПГ =
                break;
            }
            // Это обычное состояние
            //
            if (StateNPG == 1 && DispPage == 0)
                sprintf_P(lcd_buffer, PSTR("Ha\276o\273\275e\275\270e H\250\241...")); //Наполнение НПГ...
        }
        // Третья страница отображения универсальна, там показываем напряжение, дистанцию и возможное число ошибок
        // расчета среднеквадратичного.

        if (DispPage == 2) {
#if USE_GSM_WIFI == 1
            sprintf_P(lcd_buffer, PSTR("C3 %3i,w%3u,i%3i"), (int)flGPRSState, (int)timeWaitGPRS, (int)timeGPRS);
            my_lcdprint(lcd_buffer);
#else
            sprintf_P(lcd_buffer, PSTR("C3 He\277 GSM \276o\343\343ep\266\272\270")); //Hет GSM поддержки
            my_lcdprint(lcd_buffer);
#endif
            dtostrf((float)MaxIOut / 10, 6, 3, str_cur);
            sprintf_P(lcd_buffer, PSTR("B\303xo\343\275o\271 I = %sA"), str_cur); //Выходной I=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("B\303xo\343\275a\307 P =   %4iW"), FactPower); //Выходная P=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Bxo\343\275oe  U =    %3uV"), (uint16_t)MaxVoltsOut); //Входное U=
        }
        // На четвертой странице показываем всю температуру и давление
        if (DispPage == 3) {
            sprintf_P(lcd_buffer, PSTR("C4 Te\274\276epa\277\171pa:")); //С4 Температура:
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("ds5=%i.%i,\340a\263\273.=%i.%i"), DS_TEMP(4) / 10, DS_TEMP(4) % 10, U_MPX5010 / 10, U_MPX5010 % 10); //ds5=   ,Давл.=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("TCA=%i.%i, ds4=%i.%i,"), DS_TEMP(TEMP_TSA) / 10, DS_TEMP(TEMP_TSA) % 10, DS_TEMP(3) / 10, DS_TEMP(3) % 10);
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("K\171\262=%i.%i,\340e\344\273=%i.%i,"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
        }

        // На пятой странице показываем состояние датчиков уровней                                             //Куб=   , Дефл=   ,
        if (DispPage == 4) {
            sprintf_P(lcd_buffer, PSTR("C5 Po\267\273\270\263 Bo\343\303= %4u"), U_VODA); //Розлив Воды=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("A\277\274. \340a\263\273e\275\270e= %3imm"), PressAtm); //Атм. Давление=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \241o\273o\263 = %4u"), U_GLV); //Уровень Голов =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \263 Tape= %4u"), U_UROVEN); //Уровень в Таре=
        }

        // На шестой странице показываем состояние датчиков уровней
        if (DispPage == 5) {
            sprintf_P(lcd_buffer, PSTR("C6 \251po\263e\275\304 \241a\267a=%4u"), U_GAS); //Уровень Газа=
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Mo\346\275oc\277\304 =   %4uW"), UstPwrPH1); //Мощность =
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("\275\171\273\304/ce\272.(zPS)=  %3u"), (int)zPSOut); //нуль/сек.(zPS)=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\253\270c\273o \276pep\303\263a\275\270\271")); //Число прерываний
        }

        // На седьмой странице показываем состояние регулятора мощности
        if (DispPage == 6) {
#if NUM_PHASE == 1
            sprintf_P(lcd_buffer, PSTR(" %4i,%4i,%4iW"), UstPwrPH1, UstPwrPH2, UstPwrPH3);
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("C7 %3i,%3i,%3i %%"), KtPhase[0], KtPhase[1], KtPhase[2]);
#else
            sprintf_P(lcd_buffer, PSTR("C7 Z=%4u"), TicZero);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Bz=%3u"), (int)b_value[0]);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Bpe\274\307 \263\272\273.\272\273\276=%4u"), TimeOpenKLP); //Время вкл.клп.=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Bpe\274\307 \263\272\273.BTA41=%4u"), TimeOpenTriac); //Время вкл.ВТА41=
#endif
        }

        // На восьмой можно сменить состояние процесса.
        if (DispPage == 7) {
            sprintf_P(lcd_buffer, PSTR("C8 C\274e\275a \305\277a\276a")); //C8 Смена этапа
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("\257\277a\276 = %i"), (int)StateMachine); //Этап =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\276po\345ecca:")); //процесса:
        }

        // На девятой состояние ПИД-регулятора
        if (DispPage == 8) {
            sprintf_P(lcd_buffer, PSTR("C9 \250\245\340-pe\264.\274o\346\275oc\277\270:")); //ПИД-рег.мощности:
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Dt=%4i It=%4i"), Dt, It);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("C\277apa\307 o\301.pacco\264-%4i"), OldErrOut); //Старые ош.рассог-
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Ho\263a\307 o\301.pacco\264\273-%4i"), NewErr); //Новые ош.рассогл-
        }
        // На Десятой странице показываем максимальные температуры за процесс температуру и давление
        if (DispPage == 9) {
            sprintf_P(lcd_buffer, PSTR("C10 Ma\272c.\277e\274\276epa\277\171pa")); //Макс.температура
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("ds4=%i.%i, ds5=%i.%i"), MAX_DS_TEMP(3) / 10, MAX_DS_TEMP(3) % 10, MAX_DS_TEMP(4) / 10, MAX_DS_TEMP(4) % 10);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("ds2=%i.%i, ds3=%i.%i,"), MAX_DS_TEMP(1) / 10, MAX_DS_TEMP(1) % 10, MAX_DS_TEMP(2) / 10, MAX_DS_TEMP(2) % 10);
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\267a \276po\345ecc: ds1=%i.%i"), MAX_DS_TEMP(0) / 10, MAX_DS_TEMP(0) % 10); //за процесс: ds1=
        }
        // На 11  странице показываем параметры расчета ПИД-регулятора
        if (DispPage == 10) {
            sprintf_P(lcd_buffer, PSTR("C11 \250\245\340-pe\264.\277e\274\276epa\277\171p\303:")); //ПИД-рег.температуры:
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("KId %4i %4i %3i"), KtT, ItTemp, DtTemp);
            my_lcdprint(lcd_buffer);
            lcd.setCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("C\277apa\307 o\301.pacco\264-%3i"), OldErrTempOut); //Старые ош.рассог-
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Ho\263a\307 o\301.pacco\264\273-%3i"), NewErrTemp); //Новые ош.рассогл-
        }

        if (flDspDop && DispDopInfo > 0) // Если отображаем дополнительную информацию, тогда проверяем ее статус
        {

            if (BeepStateProcess > 1) {
                if (DispDopInfo == 3 || DispDopInfo == 4) {
                    if (!(BeepStateProcess & B00000010) && DispDopInfo == 3)
                        my_beep(BEEP_LONG); // Не пищим, если установлена маска в 2 разряде
                    if (!(BeepStateProcess & B00000100) && DispDopInfo == 4)
                        my_beep(BEEP_LONG); // Не пищим, если установлена маска в 3 разряде
                } else
                    my_beep(BEEP_LONG); // Сначала пищим, предупреждая.
            } else
                my_beep(BEEP_LONG); // Сначала пищим, предупреждая.

            lcd.clear();
            lcd.setCursor(0, 2);
            if (DispDopInfo == 1) {
                sprintf_P(lcd_buffer, PSTR("       (%3i)"), (IspReg == 117 || IspReg == 118) ? COUNT_ALARM_UROVEN_FR - CountAlarmUroven : COUNT_ALARM_UROVEN - CountAlarmUroven);
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("    CMEH\245 TAP\251!")); //СМЕНИ ТАРУ!
            }
            if (DispDopInfo == 2) {
                sprintf_P(lcd_buffer, PSTR("       (%3i)"), COUNT_ALARM_VODA - CountAlarmVoda);
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("   \250POTE\253KA BO\340\256!")); //ПРОТЕЧКА ВОДЫ!
            }
            if (DispDopInfo == 3) {
                sprintf_P(lcd_buffer, PSTR("       %3iV"), (uint16_t)MaxVoltsOut);
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR(" H\245\244KOE HA\250P\261\243EH\245E!")); //НИЗКОЕ НАПРЯЖЕНИЕ!
            }
            if (DispDopInfo == 6) {
                sprintf_P(lcd_buffer, PSTR("       %3iV"), (uint16_t)MaxVoltsOut);
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("B\256COKOE HA\250P\261\243EH\245E!")); //ВЫСОКОЕ НАПРЯЖЕНИЕ!
            }
            if (DispDopInfo == 4) {
                sprintf_P(lcd_buffer, PSTR("      (%2i)"), (int)NumErrDs18);
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("  O\254\245\240KA DS18B20!")); //ОШИБКА DS18B20!
            }
            if (DispDopInfo == 5) {
                sprintf_P(lcd_buffer, PSTR("     %3i/%3i"), U_MPX5010, AlarmMPX5010);
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("    \340AB\247EH\245E(!)")); //ДАВЛЕНИЕ(!)
            }

#if USE_GSM_WIFI == 1
            // Активизируем разовую отправку состояния на сервер
            if (FlToGSM > 1 && flGPRSState != 20 && flGPRSState != 164 && (timeGPRS == 0 || flNeedCall == 1)) {
                if (flNeedCall == 1) {
                    flGPRSState = 40; // Активируем звонок если в этом есть необходимость
                } else { // Если в данный момент не производится дозвон, тогда активируем GPRS сессию.
                    if (flNeedCall != 2)
                        flGPRSState = 2;
                }
                // Активируем длительную сессию GPRS
                flNeedRefrServer = 0;
                timeGPRS = 250;
            }
#endif
        }

        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 100:
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("   MEH\260 HACTPO\246K\245"), FlState); //МЕНЮ НАСТРОЙКИ
        my_lcdprint(lcd_buffer);
        break;
    case 101:
        lcd.clear();
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("  PE\243\245M \250POCMOTPA")); //РЕЖИМ ПРОСМОТРА
        my_lcdprint(lcd_buffer);
        break;
    case 102:
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("     TEPMOCTAT      "));
        my_lcdprint(lcd_buffer);
        break;
    case 103:
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR(" PE\241\251\247\261TOP MO\342HOCT\245")); //РЕГУЛЯТОР МОЩНОСТИ
        my_lcdprint(lcd_buffer);
        break;
    case 104:
        sprintf_P(lcd_buffer, PSTR("       \250EPBA\261")); //ПЕРВАЯ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("     HE\340PO\240HA\261")); //НЕДРОБНАЯ
        my_lcdprint(lcd_buffer);
        break;
    case 105:
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("    OT\240OP \241O\247OB")); //ОТБОР ГОЛОВ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR(" (\240E\244 \340E\252\247E\241MATOPA)")); //(БЕЗ ДЕФЛЕГМАТОРА)
        my_lcdprint(lcd_buffer);
        break;
    case 106:
        sprintf_P(lcd_buffer, PSTR("       BTOPA\261")); //BTOPAЯ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("      \340PO\240HA\261")); //ДРОБНАЯ
        my_lcdprint(lcd_buffer);
        break;
        //Температуру этого режима исползуется в отбре голов при дистилляции, поэтому отключаем
        //	case 107:
        //		sprintf_P(lcd_buffer, PSTR("                    "));
        //    lcd.setCursor(0, 1);
        //    sprintf_P(lcd_buffer, PSTR("       TPET\245\246"));                                 //ТРЕТИЙ
        //    my_lcdprint(lcd_buffer);
        //    lcd.setCursor(0, 2);
        //    sprintf_P(lcd_buffer, PSTR("   \340PO\240H\256\246 OT\240OP"));                     //ДРОБНЫЙ ОТБОР
        //    my_lcdprint(lcd_buffer);
        //		break;
    case 108:
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("   \244ATOP \244EPHOBO\246")); //ЗАТОР ЗЕРНОВОЙ
        my_lcdprint(lcd_buffer);
        break;
    case 109:
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("    PEKT\245\252\245KA\341\245\261")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        break;
    case 110: // Дистилляция с дефлегматором
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("  C \340E\252\247E\241MATOPOM")); //С ДЕФЛЕГМАТОРОМ
        my_lcdprint(lcd_buffer);
        break;
    case 111: // НДРФ
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("     HE\340PO\240HA\261")); //НЕДРОБНАЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("    PEKT\245\252\245KA\341\245\261")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        break;
    case 112: // НБК
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("    HE\250PEP\256BHA\261")); //НЕПРЕРЫВНАЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("  \240PA\243HA\261 KO\247OHHA")); //БРАЖНАЯ КОЛОННА
        my_lcdprint(lcd_buffer);
        break;
    case 113: // солодо-мучной затор (без варки
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("  \244ATOP CO\247O\340-M\251KA")); //ЗАТОР СОЛОД-МУКА
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("    (\240E\244 BAPK\245)"), PowerRect); //(БЕЗ ВАРКИ)
        my_lcdprint(lcd_buffer);
        break;
    case 114: // Охлаждение затора
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("     OX\247A\243\340EH\245E")); //ОХЛАЖДЕНИЕ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR(" \244ATOPA C \253\245\247\247EPOM"), PowerRect); //ЗАТОРА С ЧИЛЛЕРОМ
        my_lcdprint(lcd_buffer);
        break;
    case 115: // Таймер
        lcd.clear();
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR(" PE\241\251\247\261TOP MO\342HOCT\245")); //РЕГУЛЯТОР МОЩНОСТИ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR(" + TA\246MEP OTK\247\260\253EH\245\261")); //+ ТАЙМЕР ОТКЛЮЧЕНИЯ
        my_lcdprint(lcd_buffer);
        break;
    case 116: // Варка пива
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("    \250\245BOBAPEH\245E")); //ПИВОВАРЕНИЕ
        my_lcdprint(lcd_buffer);
        break;
    case 117: // Фракционная дистилляция
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("    \252PAK\341\245OHHA\261")); //ФРАКЦИОННАЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261")); //ДИСТИЛЛЯЦИЯ
        my_lcdprint(lcd_buffer);
        break;
    case 118: // Фракционная ректификация
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("    \252PAK\341\245OHHA\261")); //ФРАКЦИОННАЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("    PEKT\245\252\245KA\341\245\261")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        break;
    case 130:
        lcd.clear();
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("      BHE\254HEE")); //ВНЕШНЕЕ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("     \251\250PAB\247EH\245E")); //УПРАВЛЕНИЕ
        my_lcdprint(lcd_buffer);
        break;
    case 129:
        sprintf_P(lcd_buffer, PSTR("                    "));
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("       PE\243\245M")); //РЕЖИМ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR(" TECTA KOHTPO\247\247EPA")); //ТЕСТА КОНТРОЛЛЕРА
        my_lcdprint(lcd_buffer);
        break;
    case 200:
        sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277\171pa o\277\272\273\306\300.")); //Температура отключ.
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\277ep\274oc\277a\277a = %i.%i"), TempTerm / 10, TempTerm % 10);
        lcd.setCursor(0, 1); //термостата =
        my_lcdprint(lcd_buffer);
        break;
    case 201:
        sprintf_P(lcd_buffer, PSTR("\251CTAHOB\247EH T\257H")); //УСТАНОВЛЕН ТЭН
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Mo\346\275oc\277\304\306 =%5uW"), Power); //Мощностью =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 202:
        sprintf_P(lcd_buffer, PSTR("\244A\340AHHA\261 MO\342HOCT\304")); //ЗАДАННАЯ МОЩНОСТЬ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("HA\241PEBA =%5uW"), UstPowerReg); //НАГРЕВА =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 203:
        sprintf_P(lcd_buffer, PSTR("\250APAMETP USART = %u"), FlToUSART); //ПАРАМЕТР USART =
        my_lcdprint(lcd_buffer);
        break;
    case 204:
        sprintf_P(lcd_buffer, PSTR("\250APAMETP GSM=%u"), FlToGSM); //ПАРАМЕТР GSM=
        my_lcdprint(lcd_buffer);
        if (FlToGSM == 1 || FlToGSM == 0)
            sprintf_P(lcd_buffer, PSTR("GSM (SMS)"));
        if (FlToGSM == 2)
            sprintf_P(lcd_buffer, PSTR("GPRS Megafon"));
        if (FlToGSM == 3)
            sprintf_P(lcd_buffer, PSTR("GPRS BeeLine"));
        if (FlToGSM == 4)
            sprintf_P(lcd_buffer, PSTR("GPRS MTS"));
        if (FlToGSM == 5)
            sprintf_P(lcd_buffer, PSTR("GPRS Rostelecom"));
        if (FlToGSM == 6)
            sprintf_P(lcd_buffer, PSTR("GPRS Tele 2"));
        if (FlToGSM >= 7)
            sprintf_P(lcd_buffer, PSTR("GPRS Reserv %i"), (int)FlToGSM);
        if (FlToGSM == 10)
            sprintf_P(lcd_buffer, PSTR("Wi-Fi"));
        if (FlToGSM == 11)
            sprintf_P(lcd_buffer, PSTR("Android ext"));
        if (FlToGSM == 12)
            sprintf_P(lcd_buffer, PSTR("Android int"));
        if (FlToGSM > 12)
            sprintf_P(lcd_buffer, PSTR("Err! 0..11 (%i)"), (int)FlToGSM);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 205:
        sprintf_P(lcd_buffer, PSTR("\340E\247\304TA \277e\274\276epa\277\171p\303")); //ДЕЛЬТА температуры
        my_lcdprint(lcd_buffer);
        if (Delta <= 0)
            Delta = 0;
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\250\245BOBAPEH\245\245 = %i.%i"), Delta / 10, abs(Delta % 10)); //ПИВОВАРЕНИИ =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("\270 \276p\270 pa\267\264o\275e \263")); //и при разгоне в
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\263 pe\266\270\274e TEPMOCTATA")); //в режиме ТЕРМОСТАТА
        my_lcdprint(lcd_buffer);
        break;
    case 206:
        sprintf_P(lcd_buffer, PSTR("  \250EPBA\261 HE\340PO\240HA\261")); //ПЕРВАЯ НЕДРОБНАЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\343\270c\277\270\273\273\307\345\270\270 = %i.%i"), Temp1P / 10, Temp1P % 10);
        my_lcdprint(lcd_buffer); //дистилляции =
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263 K\171\262e o\272o\275\300")); //Темп-ра в Кубе оконч
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261:")); //ДИСТИЛЛЯЦИЯ:
        my_lcdprint(lcd_buffer);
        break;
    case 207:
        sprintf_P(lcd_buffer, PSTR("   BTOPA\261 \340PO\240HA\261")); //ВТОРАЯ ДРОБНАЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\343\270c\277\270\273\273\307\345\270\270 = %i.%i"), Temp2P / 10, Temp2P % 10); //дистилляции =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263 K\171\262e o\272o\275\300")); //Темп-ра в Кубе оконч
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261:")); //ДИСТИЛЛЯЦИЯ:
        my_lcdprint(lcd_buffer);
        break;
    case 208:
        //		sprintf_P(lcd_buffer, PSTR("   TPET\304\261 \340PO\240HA\261"));                               //ТРЕТЬЯ ДРОБНАЯ
        //    my_lcdprint(lcd_buffer);
        //    lcd.setCursor(0, 3);
        //    sprintf_P(lcd_buffer, PSTR("\343\270c\277\270\273\273\307\345\270\270 = %i.%i"), Temp3P/10, Temp3P%10); //дистилляции =
        //    my_lcdprint(lcd_buffer);
        //    lcd.setCursor(0, 2);
        //    sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263 K\171\262e o\272o\275\300"));                   //Темп-ра в Кубе оконч
        //    my_lcdprint(lcd_buffer);
        //    sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\261:"));                    //ДИСТИЛЛЯЦИЯ:
        //    my_lcdprint(lcd_buffer);
        //		break;
        sprintf_P(lcd_buffer, PSTR("  OT\240OP \241O\247OB \250P\245")); //ОТБОР ГОЛОВ ПРИ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("o\277\262opa = %i.%i"), Temp3P / 10, Temp3P % 10); //отбора =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263 K\171\262e o\272o\275\300")); //Темп-ра в Кубе оконч
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    \340\245CT\245\247\247\261\341\245\245:")); //ДИСТИЛЛЯЦИИ:
        my_lcdprint(lcd_buffer);
        break;
    case 209:
        sprintf_P(lcd_buffer, PSTR("PEKT\245\252\245KA\341\245\261 PA\244\241OH")); //РЕКТИФИКАЦИЯ РАЗГОН
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(+)K\171\262, (-)Ko\273o\275\275a")); //(+)Куб, (-)Колонна
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("pa\267\264o\275a = %i.%i"), tEndRectRazgon / 10, abs(tEndRectRazgon % 10)); //разгона =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Te\274\276ep-pa o\272o\275\300a\275\270\307")); //Темпер-ра окончания
        my_lcdprint(lcd_buffer);
        break;
    case 210:
        sprintf_P(lcd_buffer, PSTR("PEKT\245\252\245KA\341\245\261")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\274o\346\275oc\277\304 = %3iW"), PowerRect); //мощность =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("\250o\343a\263ae\274a\307")); //Подаваемая
        my_lcdprint(lcd_buffer);
        break;
    case 211:
        sprintf_P(lcd_buffer, PSTR("\340AT\253\245K\245 TEM\250EPAT\251P\256")); //ДАТЧИКИ ТЕМПЕРАТУРЫ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\263\263o\343 \276o\276pa\263o\272")); //ввод поправок
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 212:
        sprintf_P(lcd_buffer, PSTR("(PEKT)OT\240OP \241O\247OB:")); //(РЕКТ)ОТБОР ГОЛОВ:
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\275a\300.o\277\262op TE\247A= %i.%i"), tEndRectOtbGlv / 10, tEndRectOtbGlv % 10);
        my_lcdprint(lcd_buffer); //нач.отбор ТЕЛА =
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("\267a\272a\275\300.o\277\262op \241O\247OB \270")); //заканч.отбор ГОЛОВ и
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263 K\171\262e \272o\264\343a")); //Темп-ра в Кубе когда
        my_lcdprint(lcd_buffer);
        break;
    case 213:
        sprintf_P(lcd_buffer, PSTR("(PEKT)\254\245M OT\240OPA")); //(РЕКТ)ШИМ ОТБОРА
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\276p\270 o\277\262ope \241O\247OB")); //при отборе ГОЛОВ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("\250ep\270o\343 o\277\272p\303\277\270\307 \272\273a\276.")); //Период открытия клап.
        my_lcdprint(lcd_buffer);
        dtostrf((float)timeChimRectOtbGlv / 100, 3, 1, str_temp);
        sprintf_P(lcd_buffer, PSTR("\241O\247OB = %s(ce\272.)"), str_temp);
        my_lcdprint(lcd_buffer); //ГОЛОВ = 234.3 (сек.)
        break;
    case 214:
        sprintf_P(lcd_buffer, PSTR("(PEKT)\250PO\341EHT \254\245M")); //(РЕКТ)ПРОЦЕНТ ШИМ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)10=0,1ce\272-\272\273.o\277\272p")); //(-)10=0,1сек-кл.откр
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("(+)10=10%%o\277\272,90%%\267a\272p")); //(+)10=10%откр,90%закр
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("OT\240OPA \241O\247OB = %3i"), (int)ProcChimOtbGlv); //ОТБОРА ГОЛОВ =
        my_lcdprint(lcd_buffer);
        break;
    case 215:
        sprintf_P(lcd_buffer, PSTR("(PEKT)\254\245M OT\240OPA")); //(РЕКТ)ШИМ ОТБОРА
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("o\277\262opa CP (ce\272.)")); //отбора СР (сек.)
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("\250ep\270o\343 o\277\272p\303\277\270\307 \272\273a\276.")); //Период открытия клап.
        my_lcdprint(lcd_buffer);
        dtostrf((float)timeChimRectOtbSR / 100, 3, 1, str_temp);
        sprintf_P(lcd_buffer, PSTR("C\250\245PTA:\250ep\270o\343= %s"), str_temp);
        my_lcdprint(lcd_buffer);
        break; //СПИРТА:Период =  (сек.)
    case 216:
        sprintf_P(lcd_buffer, PSTR("(PEKT)OT\240OP C\250\245PTA:")); //(РЕКТ)ОТБОР СПИРТА:
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("CTO\250 \270 \275a\267a\343 \272 Tc\277a\262")); //СТОП и назал к Тстаб
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("\250p\270 \331 Tc\277a\262.+\340e\273\304\277a=")); //При(стр.вверх) Тстаб.+Дельта=
        my_lcdprint(lcd_buffer);
        if (tDeltaRect <= 0)
            tDeltaRect = 0;
        sprintf_P(lcd_buffer, PSTR("\340e\273\304\277a \277e\274\276ep.= %i.%i"), tDeltaRect / 10, abs(tDeltaRect % 10));
        my_lcdprint(lcd_buffer);
        break; //Дельта темпер. =
    case 217:
        sprintf_P(lcd_buffer, PSTR("(PEKT)OT\240OP C\250\245PTA:")); //(РЕКТ)ОТБОР СПИРТА:
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\275a\300\270\275.o\277\262op XBOCTOB")); //начин.отбор ХВОСТОВ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263 K\171\262e \272o\264\343a")); //Темп-ра в Кубе когда
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Te\274\276.o\272o\275\300a\275\270\307 =%i.%i"), tEndRectOtbSR / 10, tEndRectOtbSR % 10);
        my_lcdprint(lcd_buffer); //Темп.окончания =
        break;
    case 218:
        sprintf_P(lcd_buffer, PSTR("(PEKT)OKOH\253AH\245E:")); //(РЕКТ)ОКОНЧАНИЕ:
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\267a\272a\275\300.pe\272\277\270\344\270\272a\345\270\307")); //заканч.ректификация
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa \263 K\171\262e \272o\264\343a")); //Темп-ра в Кубе когда
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("Te\274\276.o\272o\275\300a\275\270\307 =%i.%i"), tEndRect / 10, tEndRect % 10);
        my_lcdprint(lcd_buffer);
        break; //Темп.окончания =
    case 250:
        sprintf_P(lcd_buffer, PSTR("PEKT\245\252\245KA\341\245\261")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("c\276\270p\277a = %2i"), minProcChimOtbSR); //спирта =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("\254\245M o\277\262opa")); //ШИМ отбора
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("M\270\275\270\274a\273\304\275\303\271 \276po\345e\275\277")); //Минимальный процент
        my_lcdprint(lcd_buffer);
        break;
    case 251:
        sprintf_P(lcd_buffer, PSTR("PEKT\245\252\245KA\341\245\261")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        dtostrf((float)tStabSR / 10, 2, 1, str_temp);
        sprintf_P(lcd_buffer, PSTR("\272o\273o\275\275\303 = %s"), str_temp); //колонны =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("c\277a\262\270\273\270\267a\345\270\270")); //стабилизации
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Pe\343a\272\277\270po\263a\275\270e \277e\274\276.")); //Редактирование темп.
        my_lcdprint(lcd_buffer);
        break;
    case 252:
        sprintf_P(lcd_buffer, PSTR("PEKT\245\252\245KA\341\245\261:")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("c\276\270p\277a = %2i"), begProcChimOtbSR); //спирта =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("\254\245M o\277\262opa")); //ШИМ отборa
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Ha\300a\273\304\275\303\271 \276po\345e\275\277"), begProcChimOtbSR);
        my_lcdprint(lcd_buffer); //Начальный процент
        break;
    case 253:
        sprintf_P(lcd_buffer, PSTR("\250o\276p-\272a MPX5010=%i.%i"), P_MPX5010 / 10, abs(P_MPX5010 % 10)); //Попр-ка MPX5010=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("                    "));
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2); //Знач.с попр-ой=
        sprintf_P(lcd_buffer, PSTR("\244\275a\300.c \276o\276p-o\271=%i.%i"), U_MPX5010 / 10, U_MPX5010 % 10 + P_MPX5010 / 10, P_MPX5010 % 10);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Te\272\171\346.\267\275a\300e\275\270e=%i.%i"), U_MPX5010 / 10, U_MPX5010 % 10);
        my_lcdprint(lcd_buffer); //Текущ.значение=
        break;
    case 219:
        sprintf_P(lcd_buffer, PSTR("MO\342HOCT\304 OT\240OPA ")); //МОЩНOCТЬ ОТБОРА
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\343\270c\277\270\273\273\307\345\270\270 = %4iW"), PowerGlvDistil); //дистилляции =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("\241O\247OB \276p\270 \276poc\277o\271")); //ГОЛОВ при простой
        my_lcdprint(lcd_buffer);
        break;
    case 220:
        sprintf_P(lcd_buffer, PSTR("MO\342HOCT\304 OT\240OPA TE\247A")); //МОЩНОСТЬ ОТБОРА ТЕЛА
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\343\270c\277\270\273\273\307\345\270\270 = %4iW"), PowerDistil); //дистилляции =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("\276p\270 \276poc\277o\271")); //при простой
        my_lcdprint(lcd_buffer);
        break;
    case 221:
        sprintf_P(lcd_buffer, PSTR("TEM\250EPAT\251PA HA\253A\247A ")); //ТЕМПEPATУРА НАЧАЛА
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("(+)\276o \340e\344\273e\264\274a\277op\171,")); //(+)по Дефлегматору,
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)\276o K\171\262\171")); //(-)по Кубу
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("\340\245CT\245\247\247\261\341\245\245 = %i.%i"), TempDeflBegDistil / 10, abs(TempDeflBegDistil % 10));
        my_lcdprint(lcd_buffer); //ДИСТИЛЛЯЦИИ =
        break;
    case 222:
        sprintf_P(lcd_buffer, PSTR("TEM\250EPAT\251PA B\256XO\340A")); //ТЕМПЕРATУРА ВЫХОДA
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\343\270c\277\270\273\273\307\345\270\270 \276apo\263\303\274")); //дистилляции паровым
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("o\277\262opo\274 = %i.%i"), TempDefl / 10, TempDefl % 10); //отбором =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("HA \340E\252\247E\241MATOPE \276p\270")); //НА ДЕФЛЕГМАТОРЕ при
        my_lcdprint(lcd_buffer);
        break;
    case 223:
        sprintf_P(lcd_buffer, PSTR("\340E\247\304TA TEM\250EPAT\251P\256")); //ДЕЛЬТА ТЕМПЕРАТУРЫ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\343\270c\277\270\273\273\307\345\270\270 \276apo\263\303\274")); //дистилляции паровым
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("o\277\262opo\274 = %i.%i"), DeltaDefl / 10, DeltaDefl % 10); //отбором =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("HA \340E\252\247E\241MATOPE \276p\270")); //HA ДЕФЛЕГМАТОРE при
        my_lcdprint(lcd_buffer);
        break;
    case 224:
        sprintf_P(lcd_buffer, PSTR("TEM\250EPAT\251PA B K\251\240E")); //ТЕМПЕРАТУРА В КУБЕ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("c \343e\344\273e\264\274a\277opo\274")); //с дефлегматором
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\276apo\263\303\274 o\277\262opo\274=%i.%i"), tEndDistDefl / 10, tEndDistDefl % 10);
        my_lcdprint(lcd_buffer); //паровым отбором=
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("OKOH\253AH\245\261 \340\245CT-\341\245\245")); //ОКОНЧАНИЯ ДИС-ЦИИ
        my_lcdprint(lcd_buffer);
        break;
    case 225:
        sprintf_P(lcd_buffer, PSTR("C\270\264\275a\273 \276o o\272o\275\300a\275\270\270")); //Сигнал по окончании
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\276po\345ecca - %1u"), BeepEndProcess); //процесса -
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 226:
        sprintf_P(lcd_buffer, PSTR("C\270\264\275a\273 c\274e\275\303")); //Сигнал смены
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\305\277a\276a - %1u"), BeepStateProcess); //этапа =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 227:
        sprintf_P(lcd_buffer, PSTR("\244\263\171\272 \275a\266a\277\270\307")); //Звук нажатия
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\272\275o\276o\272 - %1u"), BeepKeyPress); //кнопок -
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 228:
        sprintf_P(lcd_buffer, PSTR("Mo\346\275oc\277\304 \276p\270 pa\267\263ape")); //Мощность при разваре
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\267ep\275a = %4iW"), PowerRazvZerno); //зерна
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 229:
        sprintf_P(lcd_buffer, PSTR("\250pe\343e\273\304\275a\307 \274o\346\275oc\277\304")); //Предельная мощность
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\276\270\263o\264o c\171c\273a = %4iW"), PowerVarkaZerno); //пивного сусла
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("\276p\270 \263ap\272e \267ep\275a \270")); //при варке зерна и
        my_lcdprint(lcd_buffer);
        break;
    case 230:
        sprintf_P(lcd_buffer, PSTR("\250ep\270o\343 o\262\275o\263\273e\275\270\307")); //Период обновления
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("cep\263epa(ce\272) =%3u"), (unsigned int)PeriodRefrServer); //сервера(ceк) =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 231:
        sprintf_P(lcd_buffer, PSTR("Ha\276p\307\266e\275\270e \267a\346\270\277\303")); //Напряжение защиты
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\272\273a\276a\275o\263 = %3uV"), (unsigned int)NaprPeregrev); //клапанов =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 232:
        sprintf_P(lcd_buffer, PSTR("\251po\263e\275\304 \262ap\343\303 =%4i"), UrovenBarda); //Уровень барды =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\240ap\343oo\277\263o\343\300\270\272(%4u)"), U_GLV); //Бардоотводчик(..)
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 233:
        sprintf_P(lcd_buffer, PSTR("OT\240OP \241O\247OB =%4i"), UrovenProvodimostSR); //ОТБОР ГОЛОВ =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)\276o \263pe\274e\275\270(1=10\274)")); //(-)по времени(1=10м)
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("2-a\275a\273o\264.\343a\277.\171po\263\275\307,")); //2-аналог.дат.уровня,
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("0-\277e\274\276,1-\345\270\344p.\343a\277.\171p")); //0-темп,1-цифр.дат.ур
        my_lcdprint(lcd_buffer);
        break;
    case 234:
        sprintf_P(lcd_buffer, PSTR("BPEM\261 PA\240OT\256 KO\247OHH\256")); //ВРЕМЯ РАБОТЫ КОЛОННЫ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("-)c \274o\274e\275\277a \276po\264pe\263a")); //-)с момента прогревa
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("+)o\277 \276oc\273.\270\267\274.\277e\274\276.")); //+)от посл.изм.темп.
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("HA CE\240\261 =%5i ce\272."), TimeStabKolonna); //НА СЕБЯ=
        my_lcdprint(lcd_buffer);
        break;
    case 235:
        sprintf_P(lcd_buffer, PSTR("OT\240OP \250O TEM\250EP.K\251\240A")); //ОТБОР ПО ТЕМПEP.КУБА
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\253\270c\273o \277o\300e\272 = %3i"), (int)CntCHIM); //Число точек =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 236:
        sprintf_P(lcd_buffer, PSTR("(PEKT)HACTPO\246KA \254\245M:")); //(РЕКТ)НАСТРОЙКА ШИМ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\275a c\272o\273\304\272o \254\245M \275\171\266\275o")); //на сколько ШИМ нужно
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\171\274e\275\304\301\270\277\304 \276p\270 CTO\250E")); //уменьшить при СТОПЕ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("ABTO - %% \254\245M =%3i"), (int)DecrementCHIM); //АВТО - % ШИМ =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 237:
        sprintf_P(lcd_buffer, PSTR("(PEKT)HACTPO\246KA \254\245M:")); //(РЕКТ)НАСТРОЙКА ШИМ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\275a c\272o\273\304\272o \254\245M \275\171\266\275o")); //на сколько ШИМ нужно
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\171\263e\273\270\300.\276p\270 \343\273\270\277.CTO\250")); //увелич.при длит.СТОП
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("ABTO + %% \254\245M =%3i"), (int)IncrementCHIM); //АВТО + % ШИМ =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 238:
        sprintf_P(lcd_buffer, PSTR("(PEKT)HACTPO\246KA \254\245M:")); //(РЕКТ)НАСТРОЙКА ШИМ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Bpe\274\307 o\276pe\343e\273e\275\270\307,")); //Время определения,
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("\300\277o \343o\273\264o \275e\277 CTO\250A")); //что долго нет СТОПА
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Bpe\274\307 ABTO+\254\245M=%5ic"), TimeAutoIncCHIM); //Время АВТО+ШИМ=
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 239:
        sprintf_P(lcd_buffer, PSTR("BPEM\261 PECTA\240.KO\247OHH\256")); //ВРЕМЯ РЕСТАБ.КОЛОННЫ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("(-)\276o\277o\274 o\277\262op XBOCT")); //(-)потом отбор ХВОСТ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("0-\263\303xo\343 \270\267 CTO\250A-\263\303\272")); //0-выход из СТОПА-вык
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\340e\273\304\277a =%5ice\272"), TimeRestabKolonna); //Дельта=
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 240:
        sprintf_P(lcd_buffer, PSTR("\250A\251\244\256 \244AT\245PAH\245\261:")); //ПАУЗЫ ЗАТИРАНИЯ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Ko\273\270\300ec\277\263o -%3i"), (int)CntPause); //Кoличество -
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 241:
        sprintf_P(lcd_buffer, PSTR("ASC712 =%3i"), (int)CorrectASC712); //ASC712 =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("2-\272oppe\272\277.\270c\276o\273\304\267.")); //2-коррект.использ.
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("1-\272oppe\272\277.\275e \270c\276o\273\304\267")); //1-коррект.не использ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("0-\272oppe\272\277.\275e \270c\276o\273\304\267")); //0-коррект.не использ
        my_lcdprint(lcd_buffer);
        break;
    case 242:
        sprintf_P(lcd_buffer, PSTR("IP A\340PEC CEPBEPA:")); //IP AДPEC CEPBEPA:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%3u.%3u.%3u.%3u"), (unsigned int)ip[0], (unsigned int)ip[1], (unsigned int)ip[2], (unsigned int)ip[3]);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 243:
        sprintf_P(lcd_buffer, PSTR("\250OPT CEPBEPA:")); //ПОРТ СЕРВЕРА:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%u"), ipPort);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 244:
        sprintf_P(lcd_buffer, PSTR("ID O\240OP\251\340OBAH\245\261:")); //ID ОБОРУДОВАНИЯ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%s"), idDevice);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 245:
        sprintf_P(lcd_buffer, PSTR("HOMEP TE\247E\252OHA:")); //НОМЕР ТЕЛЕФОНА:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%s"), my_phone);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 246:
        sprintf_P(lcd_buffer, PSTR("TPEBO\241A \250O \340AB\247EH\245\260:")); //ТРЕВОГА ПО ДАВЛЕНИЮ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("MPX5010 = %i"), AlarmMPX5010);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 247:
        sprintf_P(lcd_buffer, PSTR("\250P\245MEHEH\245E ABTOHOM.")); //Применение автоном.
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("OX\247A\243\340EH\245\261 = %i"), FlAvtonom); //охлаждения =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 248:
        sprintf_P(lcd_buffer, PSTR("Bpe\274\307 o\277\272p.\272\273a\276a\275a")); //Время откр.клапана
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\262ap\343oo\277\263o\343\300\270\272a = %i"), (int)timeOpenBRD); //бардоотводчика =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 249:
        sprintf_P(lcd_buffer, PSTR("PID \250APAMETP\256:")); //PID ПАРАМЕТРЫ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%4i %4i %4i"), (int)PIDTemp[0], (int)PIDTemp[1], (int)PIDTemp[2]);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 254:
        sprintf_P(lcd_buffer, PSTR("Mo\346\275oc\277\304 H\240K = %4iW"), PowerNBK); //Мощность НБК =
        my_lcdprint(lcd_buffer);
        break;
    case 255:
        sprintf_P(lcd_buffer, PSTR("Wi-Fi To\300\272a \343oc\277\171\276a:")); //Wi-Fi Точка доступа:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%s"), WiFiAP);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 256:
        sprintf_P(lcd_buffer, PSTR("Wi-Fi \250apo\273\304 ce\277\270:")); //Wi-Fi Пароль сети:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%s"), WiFiPass);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 257:
        sprintf_P(lcd_buffer, PSTR("\252PAK\341\245OHHA\261 \340\245CT-\261:")); //ФРАКЦИОННАЯ ДИСТ-Я:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Ko\273\270\300-\263o \344pa\272\345\270\271=%3i"), (int)CountFractionDist); //Колич-во фраккций=
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 258:
        sprintf_P(lcd_buffer, PSTR("\252PAK\341\245OHHA\261 PEKT-\261:")); //ФРАКЦИОННАЯ РЕКТ-Я:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Ko\273\270\300-\263o \344pa\272\345\270\271=%3i"), (int)CountFractionRect); //Колич-во фраккций=
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 259:
        sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277\171pa \267ac\303\276\270")); //Температура засыпи
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\267a\277opa = %i.0"), (int)TempZSP); //затора =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 260:
        sprintf_P(lcd_buffer, PSTR("Te\274\276-pa ocaxap\270\263a\275\270\307")); //Тмпер-ра осахаривания
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\267a\277opa = %i.0"), (int)TempZSPSld); //затора
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 261:
        sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277-pa \262po\266e\275\270\307")); //Температ-ра брожения
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\267a\277opa = %i.0"), (int)TempHLDZatorBrog1); //затора =
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 262:
        sprintf_P(lcd_buffer, PSTR("O\240\342A\261 MO\342HOCT\304=%4iW"), Power); //ОБЩАЯ МОЩНОСТЬ =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("Mo\346\275.\252A\244\256 3 =  %4iW"), PowerPhase[2]); //Мощн.ФАЗЫ 3 =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Mo\346\275.\252A\244\256 2 =  %4iW"), PowerPhase[1]); //Мощн.ФАЗЫ 2 =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Mo\346\275.\252A\244\256 1 =  %4iW"), PowerPhase[0]); //Мощн.ФАЗЫ 1 =
        my_lcdprint(lcd_buffer);
        break;
    case 263:
        sprintf_P(lcd_buffer, PSTR("\250po\345e\275\277 pac\276pe\343-\275\270\307")); //Пpoцент распред-ния
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("O\262\346\270\271 \276po\345e\275\277 = %3i%%"), (int)KtPhase[0] + KtPhase[1] + KtPhase[2]);
        my_lcdprint(lcd_buffer); //Общий процент =
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("%3i%% + %3i%% + %3i%%"), (int)KtPhase[0], (int)KtPhase[1], (int)KtPhase[2]); //100% + 100% + 100%
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\274o\346\275oc\277\270 \276o \344a\267a\274:"), (int)KtPhase[0]); //мощности по фазам:
        my_lcdprint(lcd_buffer);
        break;
    case 264:
        sprintf_P(lcd_buffer, PSTR("M\270\275\270\274a\273\304\275oe \343a\263\273e\275\270e")); //Минимальное давление
        my_lcdprint(lcd_buffer); //НБК:
        sprintf_P(lcd_buffer, PSTR("H\240K: %i.%i+%i.%i=%i.%i"), ((int)minPressNBK * 5) / 10, ((int)minPressNBK * 5) % 10, ((int)deltaPressNBK) / 10, ((int)deltaPressNBK) % 10, ((int)minPressNBK * 5 + (int)deltaPressNBK) / 10, ((int)minPressNBK * 5 + (int)deltaPressNBK) % 10);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 265:
        sprintf_P(lcd_buffer, PSTR("\340e\273\304\277a \343a\263\273e\275\270\307")); //Дельта давления
        my_lcdprint(lcd_buffer); //НБК:
        sprintf_P(lcd_buffer, PSTR("H\240K: %i.%i+%i.%i=%i.%i"), ((int)minPressNBK * 5) / 10, ((int)minPressNBK * 5) % 10, ((int)deltaPressNBK) / 10, ((int)deltaPressNBK) % 10, ((int)minPressNBK * 5 + (int)deltaPressNBK) / 10, ((int)minPressNBK * 5 + (int)deltaPressNBK) % 10);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 266:
        sprintf_P(lcd_buffer, PSTR("Koppe\272\345\270\307 \276ep\270o\343a")); //Коррекция периода
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\262pa\264\270(H\240K) = %3i"), (int)timePressNBK * 5); //браги(НБК) =
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("c\272opoc\277\270 \276o\343a\300\270")); //скорости подачи
        my_lcdprint(lcd_buffer);
        break;
    case 267:
        sprintf_P(lcd_buffer, PSTR("\251\276pa\263.\275acoco\274 H\240K%3i"), (int)UprNasosNBK); //Управ.насосом НБК:
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("5-\171\276pa\263\273e\275\270e \276o \254\245M")); //5-управление по ШИМ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("1-BK\247.\263.\171p,B\256K.\275.\171p")); //1-ВКЛ.в.ур,ВЫК.н.ур
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("0-BK\247.\275.\171p,B\256K.\263.\171p")); //0-ВКЛ.н.ур,ВЫК.в.ур
        my_lcdprint(lcd_buffer);
        break;
    case 268:
        sprintf_P(lcd_buffer, PSTR("%%-o\277\262opa \341.\250.=%3i"), (int)ProcChimOtbCP); //%-отбора Ц.П.=
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)1=0,1ce\272-\272a\266\343.10c")); //(-)1=0,1сек-кажд.10с
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("(+)1=1%%<OT\240OP \241O\247OB>")); //(+)1=1%<ОТБОР ГОЛОВ>
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("0-\275e\277 o\277\262opa,")); //0-нет отбора
        my_lcdprint(lcd_buffer);
        break;
#if ENABLE_SENSOR_SORTING
    case 269:
        sprintf_P(lcd_buffer, PSTR("\245\267\274e\275.\275o\274epa DS18B20")); //Измен.номера DS18B20
        my_lcdprint(lcd_buffer);
        // Печатаем номера всех наших сенсоров
        for (int i = 0, n = 0; i < MAX_DS1820; i++) {
            n += sprintf_P(lcd_buffer + n, (i > 0) ? PSTR(" %02d") : PSTR("%02d"), (int)ds1820_nums[i] + 1);
        }
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
#endif
#if USE_BMP280_SENSOR
    case 270:
        sprintf_P(lcd_buffer, PSTR("Bpe\274\307 o\276poca \343a\277\300\270\272a")); //Время опроса датчика
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("BMP280 = %3i"), (int)timePressAtm);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
#endif // USE                                                                                      //Температура DS%1i =
    case 300:
        sprintf_P(lcd_buffer, PSTR("Te\274\276epa\277\171pa DS%1i=%i.%i"), nPopr + 1, DS_TEMP(nPopr) / 10, DS_TEMP(nPopr) % 10);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\250o\276p.%i.%i Te\274\276.=%i.%i"), ds1820_popr[nPopr] / 10, abs(ds1820_popr[nPopr] % 10), (DS_TEMP(nPopr) + ds1820_popr[nPopr]) / 10, (DS_TEMP(nPopr) + ds1820_popr[nPopr]) % 10);
        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer); //Попр.   Темп.=
        break;
    case 301:
        sprintf_P(lcd_buffer, PSTR("TempK(%1i)=%4i"), (int)nPopr, tempK[nPopr]);
        if (flPopr == 0) {
            sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        }

        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\254\245M(%1i)=%3i"), (int)nPopr, (int)CHIM[nPopr]); //ШИМ(%1i)=%3i
        if (flPopr == 1) {
            sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        }

        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;

    case 302:
        // Первое значение это время работы насоса
        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("Pa\262o\277a \275acoca =%2im"), (int)tempP[nPopr]); //Работа насоса =
        else
            sprintf_P(lcd_buffer, PSTR("Te\274\276.\276a\171\267\303 \356%i = %i"), (int)nPopr, (int)tempP[nPopr]);

        if (flPopr == 0) //Темп.паузы(значок шестигранника)=
        {
            sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        }

        my_lcdprint(lcd_buffer);
        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("\250a\171\267a \275acoca  =%2im"), (int)timeP[nPopr]); //Пауза насоса =
        else
            sprintf_P(lcd_buffer, PSTR("Bpe\274\307 \276a\171\267\303\356%i=%3im"), (int)nPopr, (int)timeP[nPopr]);

        if (flPopr == 1) //Время паузы(значок шестигранника)=
        {
            sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        }

        lcd.setCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;

    case 303:
        sprintf_P(lcd_buffer, PSTR("%3u.%3u.%3u.%3u"), (unsigned int)ip[0], (unsigned int)ip[1], (unsigned int)ip[2], (unsigned int)ip[3]);
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        j = -1;
        while (j < nPopr * 4) {
            j++;
            lcd_buffer[j] = ' ';
        }
        lcd_buffer[j + 1] = '*';
        lcd_buffer[j + 2] = 0;

        my_lcdprint(lcd_buffer);

        break;
    case 304:
        sprintf_P(lcd_buffer, PSTR("%s (%3u)"), idDevice, (unsigned int)idDevice[nPopr]);
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        j = 0;
        while (j < nPopr) {
            lcd_buffer[j] = ' ';
            j++;
        }
        lcd_buffer[j] = '*';
        lcd_buffer[j + 1] = 0;

        my_lcdprint(lcd_buffer);

        break;

    case 305:
        sprintf_P(lcd_buffer, PSTR("%s/%3u)"), my_phone, (unsigned int)my_phone[nPopr]);
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        j = 0;
        while (j < nPopr) {
            lcd_buffer[j] = ' ';
            j++;
        }
        lcd_buffer[j] = '*';
        lcd_buffer[j + 1] = 0;

        my_lcdprint(lcd_buffer);

        break;
    case 309:
        sprintf_P(lcd_buffer, PSTR("%4i %4i %4i"), (int)PIDTemp[0], (int)PIDTemp[1], (int)PIDTemp[2]);
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("   P"));
        if (nPopr == 1)
            sprintf_P(lcd_buffer, PSTR("        I"));
        if (nPopr == 2)
            sprintf_P(lcd_buffer, PSTR("             D"));
        my_lcdprint(lcd_buffer);
        break;

    case 310:
        sprintf_P(lcd_buffer, PSTR("%s /%3u"), WiFiAP, (unsigned int)WiFiAP[nPopr]);
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        j = 0;
        while (j < nPopr) {
            lcd_buffer[j] = ' ';
            j++;
        }
        lcd_buffer[j] = '*';
        lcd_buffer[j + 1] = 0;

        my_lcdprint(lcd_buffer);

        break;
    case 311:
        sprintf_P(lcd_buffer, PSTR("%s /%3u"), WiFiPass, (unsigned int)WiFiPass[nPopr]);
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        j = 0;
        while (j < nPopr) {
            lcd_buffer[j] = ' ';
            j++;
        }
        lcd_buffer[j] = '*';
        lcd_buffer[j + 1] = 0;

        my_lcdprint(lcd_buffer);

        break;
    case 312:
        // последнее значение это тест фракционника
        if (nPopr == MAX_CNT_FRACTION_DIST) {
            lcd.clear();
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("TECT \252pa\272\345\270o\275\275\270\272a")); //ТЕСТ Фракционника
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\343\270c\277\270\273\273\307\345\270\270 =%3i"), (int)TekFraction + 1);
        } //дистилляции =
        else {
            if (TempFractionDist[nPopr] >= 0) {
                sprintf_P(lcd_buffer, PSTR("Te\274\276-pa o\272o\275\300a\275\270\307")); //Темп-ра окончания
                my_lcdprint(lcd_buffer); //отбора %1i фракц.=%4i
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("(+)\276o Te\274\276,(-)\276o Bp.")); //(+)по Темп,(-)по Вр.
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("o\277\262opa %1i\344pa\272\345.=%i.%i"), (int)nPopr + 1, (int)TempFractionDist[nPopr] / 10, (int)TempFractionDist[nPopr] % 10);
            } else {
                sprintf_P(lcd_buffer, PSTR("Bpe\274\307 o\277\262opa")); //Вpемя отбора
                my_lcdprint(lcd_buffer); //%1i-й фракции=%5im
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("(+)\276o Te\274\276,(-)\276o Bp.")); //(+)по Темп,(-)по Вр.
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("%1i-\271 \344pa\272\345\270\270 =%5im"), (int)nPopr + 1, (int)-TempFractionDist[nPopr]);
            }
            if (flPopr == 0) {
                sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
            }

            my_lcdprint(lcd_buffer);

            if (flPopr == 1)
                sprintf_P(lcd_buffer, PSTR("\251\264o\273=%3i*, %4iW"), (int)AngleFractionDist[nPopr], PowerFractionDist[nPopr]);
            else
                sprintf_P(lcd_buffer, PSTR("\251\264o\273=%3i , %4iW"), (int)AngleFractionDist[nPopr], PowerFractionDist[nPopr]);
            //угол=%3i ,%4iW
            if (flPopr == 2)
                sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*'); //угол=%3i ,%4iW
        }
        lcd.setCursor(0, 2);
        my_lcdprint(lcd_buffer);
        break;
    case 313:
        // последнее значение это тест фракционника
        if (nPopr == MAX_CNT_FRACTION_RECT) {
            lcd.clear();
            lcd.setCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("TECT \252pa\272\345\270o\275\275\270\272a")); //ТЕСТ Фракционника
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("pe\272\277\270\344\270\272a\345\270\270 =%3i"), (int)TekFraction + 1);
        } //ректификации =
        else {
            if (TempFractionRect[nPopr] >= 0) {
                sprintf_P(lcd_buffer, PSTR("Te\274\276-pa o\272o\275\300a\275\270\307")); //Темп-ра окончания
                my_lcdprint(lcd_buffer); //отбора %1i фракции=%4i
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("(+)\276o Te\274\276,(-)\276o Bp.")); //(+)по Темп,(-)по Вр.
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("o\277\262opa %1i\344pa\272\345.=%i.%i"), (int)nPopr + 1, (int)TempFractionRect[nPopr] / 10, (int)TempFractionRect[nPopr] % 10);
            } else {
                sprintf_P(lcd_buffer, PSTR("Bpe\274\307 o\277\262opa")); //Вpемя отбора
                my_lcdprint(lcd_buffer); //%1i-й фракции =%5im
                lcd.setCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("(+)\276o Te\274\276,(-)\276o Bp.")); //(+)по Темп,(-)по Вр.
                my_lcdprint(lcd_buffer);
                lcd.setCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("%1i-\271 \344pa\272\345\270\270 =%5im"), (int)nPopr + 1, (int)-TempFractionRect[nPopr]);
            }
            if (flPopr == 0) {
                sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
            }

            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("\251\264o\273 \252pa\272\345-\272a = %3i"), (int)AngleFractionRect[nPopr]);
            //угол фракц-ка =
            if (flPopr == 1)
                sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
        }
        lcd.setCursor(0, 2);
        my_lcdprint(lcd_buffer);
        break;
    case 314:
        sprintf_P(lcd_buffer, PSTR("PAC\250PE\340-H\245E MO\342HOCT\245")); //РАСПРЕД-НИЕ МОЩНОСТИ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("\252a\267a1")); //фаза 1
        if (nPopr == 1)
            sprintf_P(lcd_buffer, PSTR("        \252a\267a2")); //фаза 2
        if (nPopr == 2)
            sprintf_P(lcd_buffer, PSTR("               \252a\267a3")); //фаза 3
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("%4i    %4i    %4i"), (int)PowerPhase[0], (int)PowerPhase[1], (int)PowerPhase[2]);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\250O \252A\244AM:")); //ПО ФАЗАМ:
        my_lcdprint(lcd_buffer);
        break;
    case 315:
        sprintf_P(lcd_buffer, PSTR("PAC\250PE\340-H\245E MO\342HOCT\245")); //РАСПРЕД-НИЕ МОЩНОСТИ
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 3);
        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("\252a\267a1")); //фаза 1
        if (nPopr == 1)
            sprintf_P(lcd_buffer, PSTR("      \252a\267a2")); //фаза 2
        if (nPopr == 2)
            sprintf_P(lcd_buffer, PSTR("            \252a\267a3")); //фаза 3
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("%3i   %3i   %3i =%3i"), (int)KtPhase[0], (int)KtPhase[1], (int)KtPhase[2], (int)KtPhase[0] + KtPhase[1] + KtPhase[2]);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("\250O \252A\244AM B \250PO\341EHTAX")); //ПО ФАЗАМ В ПРОЦЕНТАХ
        my_lcdprint(lcd_buffer);
        break;
#if ENABLE_SENSOR_SORTING
    case 316:
        // Печатаем номера всех наших сенсоров
        for (int i = 0, n = 0; i < MAX_DS1820; i++) {
            n += sprintf_P(lcd_buffer + n, (i > 0) ? PSTR(" %02d") : PSTR("%02d"), (int)ds1820_nums[i] + 1);
        }
        my_lcdprint(lcd_buffer);
        lcd.setCursor(0, 1);
        j = 0;
        while (j < (nPopr)*3 + 1) {
            lcd_buffer[j] = ' ';
            j++;
        }
        lcd_buffer[j] = '*';
        lcd_buffer[j + 1] = 0;
        my_lcdprint(lcd_buffer);
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

    if (flNumStrPrint < 2) {
        lcd_buffer[0] = 0;
        my_lcdprint(lcd_buffer);
    }
    NeedDisplaying = false;
}
