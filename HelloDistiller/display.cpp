// Последнее обновление 2018-07-25 by Phisik
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Здесь всяческий вывод на экран
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "configuration.h"
#include "declarations.h"

#if USE_CYRILLIC_DISPLAY

#if SHOW_FLOAT_TEMPERATURES
#define PRINT_TEMPERATURES()                                                                                                                           \
    {                                                                                                                                                  \
        String msg = "T=" + String(0.1 * DS_TEMP(TEMP_KUB), 1) + "," + String(0.1 * DS_TEMP(TEMP_DEFL), 1) + "," + String(0.1 * DS_TEMP(TEMP_TSA), 1); \
        strcpy(lcd_buffer, msg.c_str());                                                                                                               \
        my_lcdprint(lcd_buffer);                                                                                                                       \
    }
#else
#define PRINT_TEMPERATURES()                                                          \
    {                                                                                 \
        sprintf_P(lcd_buffer, PSTR("C2 T=%i.%i,%i.%i,%i.%i"),                         \
            DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, DS_TEMP(TEMP_RK20) / 10,  \
            DS_TEMP(TEMP_RK20) % 10, DS_TEMP(TEMP_TSA) / 10, DS_TEMP(TEMP_TSA) % 10); \
        my_lcdprint(lcd_buffer, 0);                                                   \
    }
#endif

#if USE_MQTT_BROKER
static_assert(MQTT_BUFFER_SIZE == LCD_BUFFER_SIZE, "MQTT_BUFFER_SIZE == LCD_BUFFER_SIZE");
#endif

// Phisik! @2021-03-24
// Внимание! Тут надо бы все поправить глобально
// Надо передавать строку и колонку, с которой начинать печатать, прямо в my_lcdprint()
// Тогда мы сможем нормально нумеровать строки для передачи по mqtt
// Раньше было 2 строки и первая всегда была первой, теперь их 4 и они в перемешку
//
// Я сейчас поставлю тут затычку, но надо сделать все по уму!

int8_t currentCol = -1, currentRow = -1;
void dirtyTrickSetCursor(int col, int row)
{
    lcd.setCursor(col, row);
    currentCol = col;
    currentRow = row;
}

bool bRawOutputStatus[LCD_HEIGHT] = { false };

void dirtyTrickLcdClear()
{
    lcd.clear();
    currentCol = 0;
    currentRow = 0;

    // clear mqtt buffers to support for empty strings
    for (int i = 0; i < LCD_HEIGHT; i++) {
        lcd_mqtt_buf[i][5] = ' ';
        lcd_mqtt_buf[i][6] = '\0';

        bRawOutputStatus[i] = false;
    }
}

void my_lcdprint_P(const char* progmem_string, int row = -1)
{
    if (row >= 0)
        dirtyTrickSetCursor(0, row);

    memcpy_P(lcd_buffer, progmem_string, strlen_P(progmem_string));
    my_lcdprint(lcd_buffer);
}

void my_lcdprint(char* s, int row = -1)
{
    char i;

    if (row >= 0)
        dirtyTrickSetCursor(0, row);
    bRawOutputStatus[currentRow] = true;

// MQTT code by max506 & limon
#if USE_MQTT_BROKER

    if (currentRow >= 0 && currentRow < LCD_HEIGHT) {
        snprintf_P(lcd_mqtt_buf[currentRow], MQTT_BUFFER_SIZE, fmt_lcd_n, currentRow + 1, s);
    }

#endif // USE_MQTT_BROKER

    // convert uft8 to Hitachi 44780 encoding
    utf8rus(s);
    // fill rest of the screen with spaces
    const uint8_t l = strlen(s);
    if (l < LCD_WIDTH) {
        memset(s + l, ' ', LCD_WIDTH - l);
        s[21] = 0;
    }

    if (IspReg == 111 && s[9] == 'R' && currentRow == 0)
        s[9] = 'N';

    lcd.print(s);
    currentRow++;

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
char str_popr[6];

void formTRZGDistill()
{
    if (TempDeflBegDistil > 0) {

        sprintf_P(lcd_buffer, PSTR("Куб       = %i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10); //9.04.21 Темпер-ра Куба=
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("Дeф   = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, TempDeflBegDistil / 10, TempDeflBegDistil % 10); //9.04.21 Темп.Дефа=
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("Полная P     = %4iW"), Power); //8.04.21 Вых.мощность=
        my_lcdprint(lcd_buffer, 3);
    } else {
        sprintf_P(lcd_buffer, PSTR("Дeфлeгматор = %i.%i\337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //Темп.Дефлегм.=
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("Куб    = %i.%i/%i.%i\337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, -TempDeflBegDistil / 10, -TempDeflBegDistil % 10); // //9.04.21 Темп.Куба =
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("Полная P     = %4iW"), Power); //8.04.21 Вых.мощность=
        my_lcdprint(lcd_buffer, 3);
    }
}

void formTSAErr()
{
    sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ПPEBЫШEHA"), hour, minute, second); //00:00:00 ПРЕВЫШЕНА
    my_lcdprint(lcd_buffer, 0);

    sprintf_P(lcd_buffer, PSTR("  TEMПEPATУPA TCA!")); //ТЕМПЕРАТУРА ТСА!
    my_lcdprint(lcd_buffer, 1);

    sprintf_P(lcd_buffer, PSTR(" TCA = %i.%i/%i.%i \337C"), DS_TEMP(TEMP_TSA) / 10, DS_TEMP(TEMP_TSA) % 10, MAX_TEMP_TSA / 10, MAX_TEMP_TSA % 10); //24.04.21 Темп-ра ТСА=
    my_lcdprint(lcd_buffer, 3);
}

void DisplayRectif()
{
    if (DispPage == 0) {
        switch (StateMachine) {
        case 0:
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 3);
            if (IspReg != 118) {
                sprintf_P(lcd_buffer, PSTR("                    "));
                my_lcdprint(lcd_buffer);
                dirtyTrickSetCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("    PEKTИФИKAЦИИ")); //РЕКТИФИКАЦИИ
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("       PEЖИM")); //РЕЖИМ
                my_lcdprint(lcd_buffer, 1);
            } else {
                sprintf_P(lcd_buffer, PSTR("    PEKTИФИKAЦИИ")); //РЕКТИФИКАЦИИ
                my_lcdprint(lcd_buffer);
                dirtyTrickSetCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("    ФPAKЦИOHHOЙ")); //ФРАКЦИОННОЙ
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("       PEЖИM")); //РЕЖИМ
                my_lcdprint(lcd_buffer, 1);
            }
            break;
        case 1: //Не запущено
            sprintf_P(lcd_buffer, PSTR("       ЖДИTE \356"));
            my_lcdprint(lcd_buffer, 1);
            break;
        case 2: //Разгон
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PAЗГOH"), hour, minute, second); //РАЗГОН
            my_lcdprint(lcd_buffer);

            if (tEndRectRazgon > 0) {

                sprintf_P(lcd_buffer, PSTR("Окончание по темп-ре")); //2.04.21 Темп. оконч. разгона
                my_lcdprint(lcd_buffer, 1);

                sprintf_P(lcd_buffer, PSTR("кубa = %i.%i/%i.%i \337C"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, tEndRectRazgon / 10, tEndRectRazgon % 10); //10.04.21 куба =
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
                my_lcdprint(lcd_buffer, 3);

            } else {
                sprintf_P(lcd_buffer, PSTR("Окончание по темп-ре")); //2.04.21 Темп. оконч. разгона
                my_lcdprint(lcd_buffer, 1);

                sprintf_P(lcd_buffer, PSTR("кoлoнны  %i.%i/%i.%i\337C"), DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, -tEndRectRazgon / 10, -tEndRectRazgon % 10); //10.04.21 колонны =
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
                my_lcdprint(lcd_buffer, 3);
            }
            break;

        case 3: //Стабилицация колонны
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PAБOTA"), hour, minute, second); //00:00:00 РАБОТА
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Заданная P   = %4iW"), PowerRect); //2.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("PK(20) = %i.%i(%3i)"), DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, (IspReg == 111 ? (int)(SecOstatok) : (int)(Seconds - SecTempPrev))); //12.04.21
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("KOЛOHHЫ HA CEБЯ \356")); //КОЛОННЫ НА СЕБЯ (шестигранник)
            my_lcdprint(lcd_buffer, 1);
            break;
        case 4: //Отбор голов
            if (IspReg != 118) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OTБOP ГOЛOB"), hour, minute, second); //00:00:00 ОТБОР ГОЛОВ
                my_lcdprint(lcd_buffer);

                if (UrovenProvodimostSR == 0) {
                    sprintf_P(lcd_buffer, PSTR("Окончание по темп-ре")); //Окончание отбора по
                    my_lcdprint(lcd_buffer, 1);

                    sprintf_P(lcd_buffer, PSTR("кубa = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, tEndRectOtbGlv / 10, tEndRectOtbGlv % 10); //10.04.21
                    my_lcdprint(lcd_buffer, 2);

                    sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), PowerRect); //3.04.21 Вых.мощность=
                    my_lcdprint(lcd_buffer, 3);
                } else {
                    if (UrovenProvodimostSR > 0) {
                        if (UrovenProvodimostSR == 2) {
                            sprintf_P(lcd_buffer, PSTR("дaтчику = %3i/%3i"), U_GLV, UROVEN_ALARM); //датчику =
                            my_lcdprint(lcd_buffer, 2);
                            dirtyTrickSetCursor(0, 3);
                            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), PowerRect); //3.04.21 Вых.мощность=
                            my_lcdprint(lcd_buffer);
                            sprintf_P(lcd_buffer, PSTR("Oкoнчание пo aнaлoг.")); //3.04.21 Оконч.по аналоговому
                            my_lcdprint(lcd_buffer, 1);
                        } else {
                            sprintf_P(lcd_buffer, PSTR("дaтчику = %3i/%3i"), U_GLV, UrovenProvodimostSR); //датчику =
                            my_lcdprint(lcd_buffer, 2);
                            dirtyTrickSetCursor(0, 3);
                            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), PowerRect); //3.04.21 Вых.мощность=
                            my_lcdprint(lcd_buffer);
                            sprintf_P(lcd_buffer, PSTR("Oкoнчание пo цифp.")); //3.04.21 Оконч.по цифровому
                            my_lcdprint(lcd_buffer, 1);
                        }
                    } else {
                        sprintf_P(lcd_buffer, PSTR("вpeмeни =%3i/%3i мин"), (int)SecOstatok * 10, -UrovenProvodimostSR * 10); //6.05.21 ист.времени=
                        my_lcdprint(lcd_buffer, 2);
                        dirtyTrickSetCursor(0, 3);
                        sprintf_P(lcd_buffer, PSTR("Заданная P = %4iW"), PowerRect); //3.04.21 Вых.мощность=
                        my_lcdprint(lcd_buffer);
                        sprintf_P(lcd_buffer, PSTR("Oкoнчaниe oтбopa пo")); //Окончание отбора по
                        my_lcdprint(lcd_buffer, 1);
                    }
                }
            } else {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OTБOP ГOЛOB"), hour, minute, second); //00:00:00 ОТБОР ГОЛОВ
                my_lcdprint(lcd_buffer);

                if (TekFraction < CountFractionRect)
                    V2 = TempFractionRect[TekFraction];
                if (V2 >= 0) {
                    dirtyTrickSetCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
                    my_lcdprint(lcd_buffer);

                    dirtyTrickSetCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("Куб  = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, V2 / 10, V2 % 10); //10.04.21 Темп.Куба=
                    my_lcdprint(lcd_buffer);

                    sprintf_P(lcd_buffer, PSTR("Фpaкция      = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
                    my_lcdprint(lcd_buffer, 1);
                } else {
                    dirtyTrickSetCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=

                    my_lcdprint(lcd_buffer);
                    dirtyTrickSetCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("Oкoнчание:  %5iсек"), (int)SecOstatok); //Оконч.отбора:

                    my_lcdprint(lcd_buffer);
                    sprintf_P(lcd_buffer, PSTR("Куб   = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, V2 / 10, V2 % 10); //10.04.21 Темп.Куба=
                    my_lcdprint(lcd_buffer, 1);
                }
            }
            break;
        case 5: //Стоп, ожидание возврата температуры
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PEKT.<CTOП>"), hour, minute, second); //00:00:00 РЕКТ.<СТОП>
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("oтбopa CПИPTA  = %2i%%"), ProcChimSR); //отбора СПИРТА =
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Teкущий пpoцeнт")); //Текущий процент
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Koлонна =%i.%i/%i.%i\337С"), DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, tStabSR / 10, tStabSR % 10); //10.04.21 Темп.Kол=
            my_lcdprint(lcd_buffer, 1);
            break;
        case 6: //Ректификация
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OTБOP CПИPT"), hour, minute, second); //00:00:00 ОТБОР СПИРТ
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Дaвление  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //10.04.21 Давл=    ,Мощн=
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("PK(20)=%i.%i/%i.%i %i.%i"), DS_TEMP(TEMP_RK20) / 10, DS_TEMP(TEMP_RK20) % 10, (tStabSR + tDeltaRect) / 10, (tStabSR + tDeltaRect) % 10, tDeltaRect / 10, tDeltaRect % 10);
            my_lcdprint(lcd_buffer); //PK(20)=

            sprintf_P(lcd_buffer, PSTR("Пpoцeнт oтбopa = %2i%%"), ProcChimSR); //Процент отбора =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 7: //Отбор хвостов 3.04.21
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OTБOP XBOCT"), hour, minute, second); //00:00:00 ОТБОР ХВОСТ
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR("Температура в")); //3.04.21 Давление в Кубе=
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("кубе = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, (IspReg != 118 ? tEndRect : TekTemp) / 10, (IspReg != 118 ? tEndRect : TekTemp) % 10); //10.04.21 Темп.Куба=
            my_lcdprint(lcd_buffer, 2);
            break;
        case 8: //Отбор ожидание 3 минуты
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OXЛAЖДEHИE"), hour, minute, second); //00:00:00 ОХЛАЖДЕНИE
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Температура в")); //3.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("кубе = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, (IspReg != 118 ? tEndRect : TekTemp) / 10, (IspReg != 118 ? tEndRect : TekTemp) % 10); //10.04.21 Темп.Куба=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("OБOPУДOBAHИЯ...")); //ОБОРУДОВАНИЯ...
            my_lcdprint(lcd_buffer, 1);
            break;
        case 9: //Ожидание датчика уровня
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OЖИДAHИE"), hour, minute, second); //00:00:00 ОЖИДАНИЕ
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Уpoвeнь    = %3i/%3i"), U_UROVEN, UROVEN_ALARM); //Уровень =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("ДATЧИKA УPOBHЯ \356")); //ДАТЧИКА УРОВНЯ
            my_lcdprint(lcd_buffer, 1);
            break;
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ПPOЦECC"), hour, minute, second); //00:00:00 ПРОЦЕСС
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Температура в")); //3.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("кубе = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, tEndRect / 10, tEndRect % 10); //10.04.21 Темп.Куба =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("PEKTИФИKAЦИИ OKOHЧEH")); //РЕКТИФИКАЦИИ ОКОНЧЕН
            my_lcdprint(lcd_buffer, 1);
            break;
        case 101: //Температура в ТСА превысила предельную
            formTSAErr();
            break;
        case 102: //Давление
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ДАВЛЕНИЕ"), hour, minute, second); //00:00:00 Давление
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Давление = %i.%i/%i.%i"), U_MPX5010 / 10, abs(U_MPX5010 % 10), AlarmMPX5010 / 10, AlarmMPX5010 % 10); //9.04.21
            my_lcdprint(lcd_buffer, 1);
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        dirtyTrickSetCursor(0, 1);

        if (StateMachine != 5) { //8.04.21
            if (IspReg != 118) {
                // Phisik on 2018-07-06 I switched off the 3d screen and place power on the 2nd
                //sprintf_P(lcd_buffer, PSTR("Тoк      =  %i.%iA"), (uint16_t)MaxIOut / 10, (uint16_t)MaxIOut % 10);  //5.04.21 Вых.ток =
                // sprintf_P(lcd_buffer, PSTR("Фактическая Р =%4iW"), FactPower);                                  //5.04.21 Мощность =
                //sprintf_P(lcd_buffer, PSTR("Время этапа%02u:%02u:%02u"), ((int)((Seconds - TimeStage) / 3600), (int)((Seconds - TimeStage) % 3600) / 60, (int)((Seconds - TimeStage) % 3600) % 60))); //12.04.21                 //9.04.21   "TimeStage=%02u:%02u:%02u"
                //dirtyTrickSetCursor(0, 1);
                //my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //8.04.21 Давление в Кубе=
                dirtyTrickSetCursor(0, 2);
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("Фактическая Р =%4iW"), FactPower); //8.04.21 Вых.мощность=
                dirtyTrickSetCursor(0, 3);
                my_lcdprint(lcd_buffer);

            } else if (TekFraction < CountFractionRect) {
                if (TempFractionRect[TekFraction] > 0) {
                    sprintf_P(lcd_buffer, PSTR("Окoнчaние = %i.%i \337С"), TempFractionRect[TekFraction] / 10, TempFractionRect[TekFraction] % 10); //Темп.окончания=
                    my_lcdprint(lcd_buffer);
                    dirtyTrickSetCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //Давление в Кубе=
                    my_lcdprint(lcd_buffer);
                    dirtyTrickSetCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("Фpaкция   = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
                    my_lcdprint(lcd_buffer);
                } else {
                    sprintf_P(lcd_buffer, PSTR("Bpeмя ocтaлocь: %4im"), (int)SecOstatok); //Время осталось:
                    my_lcdprint(lcd_buffer);
                    dirtyTrickSetCursor(0, 3);
                    sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //Давление в Кубе=
                    my_lcdprint(lcd_buffer);
                    dirtyTrickSetCursor(0, 2);
                    sprintf_P(lcd_buffer, PSTR("Фpaкция   = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
                    my_lcdprint(lcd_buffer);
                }
            } else {
                sprintf_P(lcd_buffer, PSTR("Уpoвeнь в Tape =%4u"), U_UROVEN); //Уровень в Таре =
                my_lcdprint(lcd_buffer);
                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //Давление в Кубе=
                my_lcdprint(lcd_buffer);
                dirtyTrickSetCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("Фpaкция   = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
                my_lcdprint(lcd_buffer);
            }
        } else {
            sprintf_P(lcd_buffer, PSTR("Pecтaбилизация=%5i"), time1); //10.04.21 Время Рестаб. =
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //10.04.21 Давление в Кубе=
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Фpaкция   = %1i/%1i"), (int)TekFraction + 1, (int)CountFractionRect); //Фракция =
            my_lcdprint(lcd_buffer);
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

            sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ")); //ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer, 1);

            sprintf_P(lcd_buffer, PSTR("  C ДEФЛEГMATOPOM")); //С ДЕФЛЕГМАТОРОМ
            my_lcdprint(lcd_buffer, 2);
            break;
        case 1:
            sprintf_P(lcd_buffer, PSTR("       ЖДИTE \356"));
            my_lcdprint(lcd_buffer, 1);
            break;
        case 2: //Ожидание закипания (прогреется дефлегматор)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PAЗГOH"), hour, minute, second); //00:00:00 РАЗГОН
            my_lcdprint(lcd_buffer);
            formTRZGDistill();
            break;
        case 3: //Ожидание закипания (прогреется куб дефлегматор)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PAЗГOH"), hour, minute, second); //00:00:00 РАЗГОН
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Куб   = %i.%i/%i.%i \337c"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, TempDeflBegDistil / 10, TempDeflBegDistil % 10); //10.04.21 Темп.Куба=
            my_lcdprint(lcd_buffer, 1);

            sprintf_P(lcd_buffer, PSTR("Дефлегматор = %i.%i\337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //10.04.21 Темп.после Дефa=
            my_lcdprint(lcd_buffer, 2);

            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer, 3);
            break;
        case 4: //Работа без дефлегматора
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ДИCTИЛЛЯЦИЯ"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Дeф   = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, TempDefl / 10, TempDefl % 10); //10.04.21 Темп.Дефа=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("БEЗ ДEФЛEГMATOPA")); //БЕЗ ДЕФЛЕГМАТОРА
            my_lcdprint(lcd_buffer, 1);
            break;
        case 5: //Работа с 50% дефлегмацией
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ДИCTИЛЛЯЦИЯ"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Дeф   = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, (TempDefl - DeltaDefl) / 10, (TempDefl - DeltaDefl) % 10); //10.04.21 Темп.Дефа=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("C ДEФЛEГMAЦИEЙ 50%%")); //С ДЕФЛЕГМАЦИЕЙ 50%
            my_lcdprint(lcd_buffer, 1);
            break;
        case 6: //Работа с 100% дефлегмацией
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ДИCTИЛЛЯЦИЯ"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //05.04.21 Вых.мощность=
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Дeф   = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, (TempDefl - DeltaDefl) / 10, (TempDefl - DeltaDefl) % 10); //10.04.21 Темп.Дефа=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("C ДEФЛEГMAЦИEЙ 100%%")); //С ДЕФЛЕГМАЦИЕЙ 100%
            my_lcdprint(lcd_buffer, 1);
            break;
        case 7: //Ожидание для охлаждения
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OKOHЧAHИE"), hour, minute, second); //00:00:00 ОКОНЧАНИЕ
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Дефлегматор = %i.%i\337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //10.04.21 Темп.после Дефa=
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("oбopудoвaния \356 %4i"), SecondsEnd); //оборудования (шестигранник)
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Oxлaждeниe")); //Охлаждение
            my_lcdprint(lcd_buffer, 1);
            break;
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ДИCTИЛЛЯЦИЯ"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Куб       = %i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10);
            my_lcdprint(lcd_buffer, 3);

            sprintf_P(lcd_buffer, PSTR("      OKOHЧEHA")); //ОКОНЧЕНА
            my_lcdprint(lcd_buffer, 2);

            sprintf_P(lcd_buffer, PSTR("  C ДEФЛEГMATOPOM"), hour, minute, second); //С ДЕФЛЕГМАТОРОМ
            my_lcdprint(lcd_buffer, 1);
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("Paз-цa Moщнocти=%4iW"), deltaPower); //Раз-ца Мощности=
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("Poзлив Boды    =%4u"), U_VODA); //Розлив Воды =
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("Уpoвeнь в Tape =%4u"), U_UROVEN); //Уровень в Таре =
        my_lcdprint(lcd_buffer, 3);
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
 my_lcdprint(lcd_buffer, 1);
 break;
 case 4: //Отбор голов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sGl Otb"),hour,minute,second);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),Temp3P,PowerGlvDistil);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 5: //Ожидание 60 секунд для охлаждения
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sOtbGlv"),hour,minute,second);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("tK=%3i Wait=%4i"),DS_TEMP(TEMP_KUB),SecondsEnd);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 100: //Окончание
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u sOtbGlv"),hour,minute,second);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("tK=%3i end"),DS_TEMP(TEMP_KUB));
 my_lcdprint(lcd_buffer, 1);
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
 	my_lcdprint(lcd_buffer, 1);
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

        //		if (IspReg != 105)  sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u %1uЙ-OTБOP"), hour, minute, second, V1);     //00:00:00 Й-ОТБОР
        //		else  sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OTБOP ГOЛOB"), hour, minute, second);                   //00:00:00 ОТБОР ГОЛОВ

        switch (StateMachine) {
        case 0: //Не запущено
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer, 0);

            sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ")); //ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer, 1);

            if (IspReg != 105) {
                sprintf_P(lcd_buffer, PSTR("     %1u-Й OTБOP "), V1); //№-Й ОТБОР
                my_lcdprint(lcd_buffer, 2);
            } else {
                sprintf_P(lcd_buffer, PSTR("    OTБOP ГOЛOB")); //ОТБОР ГОЛОВ
                my_lcdprint(lcd_buffer, 2);
            }
            break;
        case 1:
            sprintf_P(lcd_buffer, PSTR("       ЖДИTE \356"));
            my_lcdprint(lcd_buffer, 1);
            break;
        case 2: //Ожидание закипания (прогреется дефлегматор)
            if (IspReg != 105)
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u %1u-O PAЗГOH"), hour, minute, second, V1); //00:00:00 1-О РАЗГОН
            else
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OTБOP ГOЛOB"), hour, minute, second); //00:00:00 ОТБОР ГОЛОВ
            my_lcdprint(lcd_buffer);
            formTRZGDistill();
            break;
        case 3: //Дистилляция
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ДИCTИЛЛЯЦИЯ"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer, 0);

            sprintf_P(lcd_buffer, PSTR("Куб   = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, V2 / 10, V2 % 10); //10.04.21 Темп.Куба=
            my_lcdprint(lcd_buffer, 1);

            sprintf_P(lcd_buffer, PSTR("Дефлегматор = %i.%i\337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //10.04.21 Темп.после Дефa=
            my_lcdprint(lcd_buffer, 2);

            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность =
            my_lcdprint(lcd_buffer, 3);

            break;
        case 4: //Ожидание 60 секунд для охлаждения
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OXЛAЖДEHИE"), hour, minute, second); //00:00:00 ОХЛАЖДЕНИE
            my_lcdprint(lcd_buffer, 0);

            sprintf_P(lcd_buffer, PSTR("OБOPУДOBAHИЯ...")); //ОБОРУДОВАНИЯ...
            my_lcdprint(lcd_buffer, 1);

            sprintf_P(lcd_buffer, PSTR("Дефлегматор = %i.%i\337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //10.04.21 Темп.после Дефa=
            my_lcdprint(lcd_buffer, 2);

            sprintf_P(lcd_buffer, PSTR("Ждитe: %4i"), SecondsEnd); //Ждите:
            my_lcdprint(lcd_buffer, 3);

            break;
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ДИCTИЛЛЯЦИЯ"), hour, minute, second); //00:00:00 ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer, 0);

            sprintf_P(lcd_buffer, PSTR("      OKOHЧEHA")); //ОКОНЧЕНА
            my_lcdprint(lcd_buffer, 1);

            sprintf_P(lcd_buffer, PSTR("Куб       = %i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10); //10.04.21 Темпер-ра Куба=
            my_lcdprint(lcd_buffer, 2);

            break;
        case 101: //Температура в ТСА превысила предельную
            formTSAErr();
            break;
        }
    } else {
        PRINT_TEMPERATURES();

        sprintf_P(lcd_buffer, PSTR("Уpoвeнь в Tape =%4u"), U_UROVEN); //Уровень в Таре =
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //9.04.21 Давление(MPX)=
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность=
        my_lcdprint(lcd_buffer, 3);
    }
}

void DisplayFracionDistill()
{
    if (DispPage == 0) {
        switch (StateMachine) {
        case 0: //Не запущено
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer, 0);

            sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ")); //ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer, 1);

            sprintf_P(lcd_buffer, PSTR("  C ФPAKЦИOHHИKOM")); //С ФРАКЦИОННИКОМ
            my_lcdprint(lcd_buffer, 2);
            break;
        case 1:
            sprintf_P(lcd_buffer, PSTR("       ЖДИTE \356"));
            my_lcdprint(lcd_buffer, 1);
            break;
        case 2: //Ожидание закипания (прогреется дефлегматор)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u PAЗГОН"), hour, minute, second); //10.04.21 00:00:00 ДИСТ.ФР.РАЗ
            my_lcdprint(lcd_buffer);
            formTRZGDistill();
            break;
        case 3: //Дистилляция
            //00:00:00 ДИСТ.ФР.1/2
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ФРАКЦИЯ %1i/%1i"), hour, minute, second, (int)(TekFraction < CountFractionDist) ? TekFraction + 1 : CountFractionDist, (int)CountFractionDist);
            my_lcdprint(lcd_buffer, 0);

            if (TekFraction < CountFractionDist) {
                V2 = TempFractionDist[TekFraction];
            }
            if (V2 >= 0) {
                sprintf_P(lcd_buffer, PSTR("Куб   = %i.%i/%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, V2 / 10, V2 % 10); //10.04.21 Темп.Куба=
                my_lcdprint(lcd_buffer, 1);

                sprintf_P(lcd_buffer, PSTR("Дeфлeгматор = %i.%i\337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //10.04.21 Темп.Дефлегм.=
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность =
                my_lcdprint(lcd_buffer, 3);
            } else { // if (V2 >= 0)
                sprintf_P(lcd_buffer, PSTR("Куб       = %i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10); //10.04.21 Темпep-pa Куба=
                my_lcdprint(lcd_buffer, 1);

                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //05.04.21 Вых.мощность =
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("Ocтaлocь: %5i мин."), (int)SecOstatok); //Осталось: % мин.
                my_lcdprint(lcd_buffer, 3);
            } //if (V2 >= 0)
            break;
        case 4: //Ожидание 60 секунд для охлаждения
            sprintf_P(lcd_buffer, PSTR("Куб       = %i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10); //10.04.21 Темпep-pa Куба=
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("OCTAЛOCь:  %4i ceк"), time3); //ОСТАЛОСЬ -
            my_lcdprint(lcd_buffer);

            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("OXЛAЖДЕНИЕ OБOPУД-Я")); //24.04.21 ОХЛАЖД.ОБОРУДОВАНИЯ
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Дeфлeгматор =%i.%i\337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //10.04.21 Темп.Дефлегм.=
            my_lcdprint(lcd_buffer, 1);
            break;
        case 100: //Окончание
            if (StateMachine == 100) {
                sprintf_P(lcd_buffer, PSTR("       ПPOЦECC")); //ПРОЦЕСС
                my_lcdprint(lcd_buffer);

                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("      ЗAKOHЧEH")); //ЗАКОНЧЕН
                my_lcdprint(lcd_buffer);

                dirtyTrickSetCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("     ДИCTИЛЛЯЦИИ")); //ДИСТИЛЛЯЦИИ
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("     ФPAKЦИOHHOЙ")); //ФРАКЦИОННОЙ
                my_lcdprint(lcd_buffer, 1);
            }
            break;
        case 101: //Температура в ТСА превысила предельную
            formTSAErr();
            break;
        }
    } else {
        PRINT_TEMPERATURES();

        sprintf_P(lcd_buffer, PSTR("Poзлив Boды    =%4u"), U_VODA); //Розлив Воды =
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("Уpoвeнь в Tape =%4u"), U_UROVEN); //Уровень в Таре =
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4uW"), UstPower); //5.04.21 Вых.мощность =
        my_lcdprint(lcd_buffer, 3);
    }
}

void DisplayRazvar()
{
    if (DispPage == 0) {
        if (IspReg == 114)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u OXЛ. ЗATOPA"), hour, minute, second); //02u:%02u:%02 ОХЛ.ЗАТОРА
        else
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u ЗEPHО/МУКА"), hour, minute, second); //24.04.21 02u:%02u:%02 РАЗВАР ЗЕРН

        my_lcdprint(lcd_buffer);

        switch (StateMachine) {
        case 0:
            sprintf_P(lcd_buffer, PSTR("Teмпepатуpa = %i.%i\337C"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //1.04.21 Темпер-ра воды=
            my_lcdprint(lcd_buffer, 2);

            sprintf_P(lcd_buffer, PSTR("Заданная Р  =  %4iW"), UstPower); //5.04.21 Вых.мощность =
            my_lcdprint(lcd_buffer, 3);

            sprintf_P(lcd_buffer, PSTR("Oжидaниe включeния \356")); //Ожидание включения
            my_lcdprint(lcd_buffer, 1);
            break;
        case 1: //Нагрев до температуры 50 градусов
            sprintf_P(lcd_buffer, PSTR("Вoда = %i.%i/%i.0 \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempZSP); //10.04.21 Темп.воды=
            my_lcdprint(lcd_buffer, 2);

            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность =
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Haгpeв вoды ...")); //Нагрев воды...
            my_lcdprint(lcd_buffer, 1);
            break;
        case 2: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
            sprintf_P(lcd_buffer, PSTR("Зacыпь зepнo и нaжми")); //Засыпь зерно и нажми
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("кнoпку <BHИЗ>")); //кнопку <ВНИЗ>
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Вoда       = %i.%i \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //24.04.21 Темп-ра воды =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 3: //Нагрев до температуры 64 градуса
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность =
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("                    "));
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Зaтop = %i.%i/%i.0 \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempZSPSld); //24.04.21 Тем.затора=
            my_lcdprint(lcd_buffer, 1);
            break;
        case 4: //Ожидание 15 минут, поддержка температуры
            sprintf_P(lcd_buffer, PSTR("Заданная Р =   %4iW"), UstPower); //5.04.21 Вых.мощность =
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Пауза ферм.= %i.%iмин"), time2 / 60, (time2 / 6) % 10); //24.04.21 Осталось -
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Зaтop      = %i.%i \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //24.04.21 Темп.затора =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 5: //Нагрев до кипения
            if (ds1820_devices < 2) {
                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность =
                my_lcdprint(lcd_buffer, 2);
                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("зaтopa = %i.%i/%i.%i\337C"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, TempKipenZator / 10, TempKipenZator % 10); //затора =
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Teмпepaтуpa")); //Температура
                my_lcdprint(lcd_buffer, 1);
            } else {
                sprintf_P(lcd_buffer, PSTR("зaтopa = %i.%i/%i.%i\337C"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10, TempKipenZator / 10, TempKipenZator % 10);
                my_lcdprint(lcd_buffer, 2); //затора =
                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.мощность =
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Teмпepaтуpa кипeния")); //Температура кипения
                my_lcdprint(lcd_buffer, 1);
            }
            break;
        case 6: //Варка
            sprintf_P(lcd_buffer, PSTR("Заданная Р =   %4iW"), PowerRazvZerno); // Вых.мощность =
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Варим     - %i.%i мин"), time2 / 60, (time2 / 6) % 10); //24.04.21 Осталось -
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Зaтop      = %i.%i \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп.затора =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 7: //Охлаждение до температыры осахаривания
            sprintf_P(lcd_buffer, PSTR("Oxлaждeниe дo тeмп.")); //Охлаждение до темп.
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("ocaxapивaния \356")); //осахаривания
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Зaтop = %i.%i/%i.0 \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempZSPSld); //10.04.21 Тем.затора=
            my_lcdprint(lcd_buffer, 1);
            break;
        case 8: //Поддержание температуры 50 градусов, пока не произойдет ручной переход к следующему этапу.
            sprintf_P(lcd_buffer, PSTR("Зacыпь coлoд и нaжми")); //Засыпь солод и нажми
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("кнoпку <BHИЗ>")); //кнопку <ВНИЗ>
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Вoда       = %i.%i \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп-ра воды =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 9: //Мешаем 10 минут
            sprintf_P(lcd_buffer, PSTR("Pазмeшиваем- %i.%iмин"), time2 / 60, (time2 / 6) % 10); //24.04.21 Работа мешалки =
            my_lcdprint(lcd_buffer, 2);

            sprintf_P(lcd_buffer, PSTR("Зaтop      = %i.%i \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп.затора =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 10: //Осахаривание
            sprintf_P(lcd_buffer, PSTR("Пpoцecc ocaxapивaния")); //Процесс осахаривания
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Ocтaлocь  - %i.%i мин"), time2 / 60, (time2 / 6) % 10); //24.04.21 Осталось -
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Зaтop      = %i.%i \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //24.04.21 Темп.затора =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 11: //Охлаждение до температуры первичного внесения дрожжей осахаривания
            sprintf_P(lcd_buffer, PSTR("1го внесения дрожжей")); //24.04.21 Температура
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Зaтop = %i.%i/40.0 \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //24.04.21 затора =
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Oxлaждeниe дo тeмп.")); //24.04.21 Охл.до внес.дрожжей
            my_lcdprint(lcd_buffer, 1);
            break;
        case 12: //Охлаждение до температуры осахаривания
            sprintf_P(lcd_buffer, PSTR("Oxлaждeниe дo тeмп.")); //Охлаждение до темп.
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("ocaxapивaния \356")); //осахаривания
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Зaтop = %i.%i/%i.0 \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempHLDZatorBrog1); //10.04.21 Тем.затора =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 13: //Поддержка брожения, ничего не делаем, только мешаем периодически
            sprintf_P(lcd_buffer, PSTR("Ид\265т бpoжeниe \356")); //Идет брожение (знак многогранника)
            my_lcdprint(lcd_buffer, 2);

            sprintf_P(lcd_buffer, PSTR("Зaтop =%i.%i/%i.0 \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempHLDZatorBrog1); //10.04.21 Тем.затора =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 14: //Поддержка брожения - охлаждение
            sprintf_P(lcd_buffer, PSTR("Пoддepжaниe бpoжeния")); //Поддержание брожения
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("- oxлaждeниe \356")); //- охлаждение (знак многогранника)
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Зaтop = %i.%i/%i.5 \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10, (int)TempHLDZatorBrog1); //10.04.21 Тем.затора =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 100: //Окончание
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("               KOHEЦ")); //КОНЕЦ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Зaтop      = %i.%i \337С"), DS_TEMP(TEMP_RAZVAR) / 10, DS_TEMP(TEMP_RAZVAR) % 10); //Темп.затора =
            my_lcdprint(lcd_buffer, 1);
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("Teмпepaтуpa зacыпи")); //Температура засыпи
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("coлoдa     = %i.0 \337С"), (int)TempZSPSld); //25.04.21 солода =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Уpoвeнь HПГ = %4u"), U_NPG); //Уровень НПГ =
        my_lcdprint(lcd_buffer, 1);
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
 my_lcdprint(lcd_buffer, 1);
 break;
 case 2: //Разгон
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND RZG"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer);
 if (tEndRectRazgon>0) sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i %4iW"),DS_TEMP(TEMP_KUB),tEndRectRazgon,UstPower);
 else sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i %4iW"),DS_TEMP(TEMP_RK20),-tEndRectRazgon,UstPower);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 3: //Стабилицация колонны
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND NSB"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("t=%3i(%4i)%4iW"),DS_TEMP(TEMP_RK20),(int)(SecOstatok),PowerRect);
 my_lcdprint(lcd_buffer, 1);
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
 my_lcdprint(lcd_buffer, 1);
 break;
 case 5: //Стоп, ожидание возврата температуры
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND Stop"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i %2i%%"),DS_TEMP(TEMP_RK20),tStabSR,ProcChimSR);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 6: //Ректификация
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u NDst %2i%%"),hour,minute,second,
);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("tN=%3i/%3i/d%2i"),DS_TEMP(TEMP_RK20),tStabSR+tDeltaRect,tDeltaRect);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 7: //Отбор хвостов
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND Hvst"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i"),DS_TEMP(TEMP_KUB),tEndRect);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 8: //Отбор ожидание 3 минут
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND Wait"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i"),DS_TEMP(TEMP_KUB),tEndRect);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 9: //Ожидание датчика уровня
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u R Wait"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("by Urov=%3i/%3i"),U_UROVEN,UROVEN_ALARM);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 100: //Окончание
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u ND End"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer);
 sprintf_P(lcd_buffer,PSTR("tK=%3i/%3i"),DS_TEMP(TEMP_KUB),tEndRect);
 my_lcdprint(lcd_buffer, 1);
 break;
 case 101: //Температура в ТСА превысила предельную
 formTSAErr();
 break;
 case 102: //Давление
 sprintf_P(lcd_buffer,PSTR("%02u:%02u:%02u Pres"),hour,minute,second);
 //sprintf_P(lcd_buffer,PSTR("%u%u%u%u  %u%u%u%u"),PINB.4,PINB.3,PINB.2,PINA.7, PINA.6,PINA.5,PINA.4,PINA.3);
 my_lcdprint(lcd_buffer); //вывод сz`одержимого буфера на LCD
 sprintf_P(lcd_buffer,PSTR("MPX5010=%3i/%3i"),U_MPX5010,AlarmMPX5010);
 my_lcdprint(lcd_buffer, 1);
 break;

 }
 }
 else
 {
 PRINT_TEMPERATURES();
 sprintf_P(lcd_buffer,PSTR("%3immV=%4u,%4u"),U_MPX5010,U_UROVEN,U_GLV);
 	my_lcdprint(lcd_buffer, 1);
 }

 }*/

void DisplayNBK() // ******************** Режим НБК *********************
{
    if (DispPage == 0) {
        switch (StateMachine) {
            dirtyTrickLcdClear();
        case 0: //Не запущено
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second);
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("  БPAЖHOЙ KOЛOHHOЙ")); //БРАЖНОЙ КОЛОННОЙ
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("    HEПPEPЫBHOЙ")); //НЕПРЕРЫВНОЙ
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ")); //ДИСТИЛЛЯЦИЯ
            my_lcdprint(lcd_buffer, 1);
            break;
        case 1:
            sprintf_P(lcd_buffer, PSTR("       ЖДИTE \356"));
            my_lcdprint(lcd_buffer, 1);
            break;
        case 2: //Ожидание закипания (прогреется дефлегматор)
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u HБK PAЗГOH"), hour, minute, second); //00:00:00 НБК РАЗГОН
            my_lcdprint(lcd_buffer);
            formTRZGDistill();
            break;
        case 3: //Ожидание запуска
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u НБК CTAPT"), hour, minute, second); //00:00:00 СТАРТ НБК
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("нaжми кнoпку <BBEPX>")); //нажми кнопку <ВВЕРХ>
            my_lcdprint(lcd_buffer, 2);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //9.04.21 Давление(MPX)=
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Для cтapтa HБK")); //Для старта НБК,
            my_lcdprint(lcd_buffer, 1);
            break;
        case 4: //Запущено
        case 6: //Запущено, подача браги остановлена
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u HБK PAБOTA"), hour, minute, second); //00:00:00 РАБОТА НБК
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //9.04.21 Давление(MPX)=
            my_lcdprint(lcd_buffer, 2);

            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4uW"), UstPower); //5.04.21 Мощность =
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Cкopocть подачи =%3i"), (int)SpeedNBK); //25.04.21 Скорость Насоса=
            my_lcdprint(lcd_buffer, 1);
            break;
        case 5: //Превышение температуры вверху
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u HБK"), hour, minute, second); //НБК
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Ocтaлocь  - %i.%i мин"), time2 / 60, (time2 / 6) % 10); //24.04.21 Осталось -
            my_lcdprint(lcd_buffer, 2);

            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("ПPEBЫШEHИE T ВBEPXУ")); //25.04.21 ПРЕВЫШЕНИЕ ТЕМП.ВЕРХ
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("ВЕРХ    = %i.%i/98 \337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); //10.04.21 Темп.Дефa=
            my_lcdprint(lcd_buffer, 1);
            break;
        case 100: //Окончание
            sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u НБК ПPOЦECC"), hour, minute, second); //02u:%02u:%02 ПРОЦЕСС
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //9.04.21 Давление(MPX)=
            dirtyTrickSetCursor(0, 2);
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("Cкopocть подачи =%3i"), (int)SpeedNBK); //25.04.21 Скорость Насоса=
            dirtyTrickSetCursor(0, 3);
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR(" ПEPEГOHKИ OKOHЧEH")); //ПЕРЕГОНКИ ОКОНЧЕН
            dirtyTrickSetCursor(0, 1);
            my_lcdprint(lcd_buffer);
            break;
        }
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("Max за период = %imm"), MaxPressByPeriod / 10); ////25.04.21 макс.за период=
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4uW"), UstPower); //5.04.21 Мощность =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //9.04.21 Давление(MPX)=
        my_lcdprint(lcd_buffer, 1);
    }
}
void DisplayTestKLP() // *************** Тест клапанов *****************
{
    if (DispPage == 0) {
        sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TECT"), hour, minute, second); //12.04.21 00:00:00 ТЕСТ ОБОР-Я
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    ОБОРУДОВАНИЯ")); //12.04.21
        my_lcdprint(lcd_buffer, 1);

        if (StateMachine == 2)
            strcpy_P(str_off, PSTR("BКЛ."));
        else
            strcpy_P(str_off, PSTR("СТОП"));

        sprintf_P(lcd_buffer, PSTR("Тест клaпaнoв - %s"), str_off); //12.04.21 Сост.клапанoв >
        my_lcdprint(lcd_buffer, 2);
        sprintf_P(lcd_buffer, PSTR("Процент ШИM = %3i%%"), ProcChimSR); //12.04.21 ШИМ откр.клап.=
        my_lcdprint(lcd_buffer, 3);

    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("ds4=%i.%i, ds5=%i.%i"), DS_TEMP(3) / 10, DS_TEMP(3) % 10, DS_TEMP(4) / 10, DS_TEMP(4) % 10);
        my_lcdprint(lcd_buffer, 1);
    }
}

void DisplayExtContol() //**************** Внешнее управление *********************
{
    if (DispPage == 0) {
        sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u BHEШ.УПPABЛ"), hour, minute, second); //ВНЕШ.УПРАВЛ
        my_lcdprint(lcd_buffer);

        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Уст.Мощность =
        my_lcdprint(lcd_buffer);

        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Дефлегматор = %i.%i\337С"), DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10); ////10.04.21 Темп-ра Дефа =
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("Куб       =%i.%i \337С"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10); //10.04.21 Темпер-ра Куба =
        my_lcdprint(lcd_buffer, 1);
    } else {
        PRINT_TEMPERATURES();
        sprintf_P(lcd_buffer, PSTR("Teмп-pa DS(5)=%i.%i,"), DS_TEMP(4) / 10, DS_TEMP(4) % 10); //Темп-ра DS(5) =
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("Уpoвeнь ГAЗA = %4u"), U_GAS); //Уровень ГАЗА =
        my_lcdprint(lcd_buffer, 3);

        sprintf_P(lcd_buffer, PSTR("Teмп-pa DS(4)=%i.%i,"), DS_TEMP(3) / 10, DS_TEMP(3) % 10); //Темп-ра DS(4) =
        my_lcdprint(lcd_buffer, 1);
    }
}

void DisplayBeer() // ********************* Пивоварение *********************
{
    if (DispPage == 0) {
        if (StateMachine == 0) {
            sprintf_P(lcd_buffer, PSTR("      %02u:%02u:%02u"), hour, minute, second); //00:00:00
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("    ПИBOBAPEHИE")); //ПИВОВАРЕНИЕ
            my_lcdprint(lcd_buffer, 1);
        }

        if (StateMachine == 1) { //Не запущено
            sprintf_P(lcd_buffer, PSTR("       ЖДИTE \356"));
            my_lcdprint(lcd_buffer, 1);
        }

        if (StateMachine == 2) {
            if (KlTek == 1) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u HAГPEB BOДЫ"), hour, minute, second); //00:00:00 НАГРЕВ ВОДЫ
                my_lcdprint(lcd_buffer, 0);

                sprintf_P(lcd_buffer, PSTR("Вода = %i.%i/%i \337С"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10, (int)tempP[KlTek]); //25.04.21 Темп.воды =
                my_lcdprint(lcd_buffer, 1);

                sprintf_P(lcd_buffer, PSTR("Hacoc/Meшaлкa - %4i"), time1); //Hacoc/Мешалка -
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), PowerVarkaZerno); //25.04.21 Макс.мощность
                my_lcdprint(lcd_buffer, 3);

            } else {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u BAPKA CУCЛA"), hour, minute, second); //00:00:00 ВАРКА СУСЛА
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("Суcло   = %i.%i/%i \337С"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10, (int)tempP[KlTek]); //10.04.21 //Темп.сусла =
                my_lcdprint(lcd_buffer, 1);

                sprintf_P(lcd_buffer, PSTR("Cлeдующaя пaузa %i/%i"), (int)KlTek, (int)CntPause); //12.04.21 Следующая пауза-
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), PowerVarkaZerno); //10.04.21 Макс.мощность =
                my_lcdprint(lcd_buffer, 3);
            }
        }
        if (StateMachine == 3) {
            if (timeP[KlTek] != 0) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TEMП. ПAУЗA"), hour, minute, second); //00:00:00 ТЕМП.ПАУЗА
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("Дo кoнцa%2i-й пaузы"), (int)KlTek); //До конца №-й паузы
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("ocтaлocь: %5i мин"), time2 / 60); //24.04.21 oсталось:
                my_lcdprint(lcd_buffer, 3);

                sprintf_P(lcd_buffer, PSTR("Суcло  = %i.%i/%i \337С"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10, (int)tempP[KlTek]); //10.04.21 Tемперат.сусла=
                my_lcdprint(lcd_buffer, 1);
            } else {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TEXHИЧECKAЯ"), hour, minute, second); //00:00:00 ТЕХНИЧЕСКАЯ
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("нaжми кнoпку <BBEPX>")); //нажми кнопку <BBEPX>
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("Пaузa - %i(%i)"), (int)KlTek + 1, (int)CntPause); //Пауза -
                my_lcdprint(lcd_buffer, 3);

                sprintf_P(lcd_buffer, PSTR("ПAУЗA \356 Для пepexoдa")); //ПАУЗА  Для перехода
                my_lcdprint(lcd_buffer, 1);
            }
        }
        if (StateMachine == 100) {
            sprintf_P(lcd_buffer, PSTR("        BAPKA"));
            my_lcdprint(lcd_buffer, 1);

            sprintf_P(lcd_buffer, PSTR("      ЗAKOHЧEHA"));
            my_lcdprint(lcd_buffer, 2);
        }
    } else { //Tемперат.сусла=
        sprintf_P(lcd_buffer, PSTR("Teмп-ра cуcлa = %i.%i"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10);
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("Hacoc/Meшaлкa - %4i"), time1); //Hacoc/Мешалка -
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //9.04.21 Давление(MPX)=
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Вых.Мощность =
        my_lcdprint(lcd_buffer, 3);
    }
}

void DisplayData() //************ РЕЖИМ ПРОСМОТРА **********************
{
    int tic1;
    static int PrevState = 0;
    static char flDspDop = 0; // Флаг для отображения информации (через раз)
    flDspDop = !flDspDop;

    hour = Seconds / 3600;
    tic1 = Seconds % 3600;
    minute = tic1 / 60;
    second = tic1 % 60;

    // clear raw output status to track printed rows
    for (int i = 0; i < LCD_HEIGHT; i++) {
        bRawOutputStatus[i] = false;
    }

    if (FlState == 0)
        dirtyTrickSetCursor(0, 0);
    else {
        // Если предыдущее состояние было 0, то даем задерку в 1/10 секунды, чтобы не слишком часто обновлялся
        // дисплей.
        if (PrevState == 0)
            delay(100);

        if (FlState != PrevState) {
            dirtyTrickLcdClear(); //очистка дисплея
            PrevState = FlState;
        }

        dirtyTrickSetCursor(0, 0);
    }

#ifdef TESTMEM
    DEBUG_SERIAL.println(F("\n[memCheck_displ]"));
    DEBUG_SERIAL.println(freeRam());
#endif

    switch (FlState) {
    case 0:
        // Рисуем основные 10 экранов
        displayMainScreens(flDspDop);
        break;

    case 100 ... 199:
        // Здесь отобрадаем меню с выбором процесса
        displayProcessSelector(FlState);
        break;

    default:
        // Тут будем отображать настройки
        displaySettings(FlState);
        break;
    }

    //
    //  dirtyTrickSetCursor(0, 0);
    //  my_lcdprint(Seconds);
    //  dirtyTrickSetCursor(5, 0);
    //  my_lcdprint(MaxVoltsOut);
    //  dirtyTrickSetCursor(9, 0);
    //  my_lcdprint((long)MaxVoltsOut*707/1000);
    //  dirtyTrickSetCursor(0, 1);
    //  my_lcdprint(index_input);
    //  dirtyTrickSetCursor(5, 1);
    //  my_lcdprint(TimeOpenTriac);
    //  NeedDisplaying=false;
    //  dirtyTrickSetCursor(10, 1);
    //  my_lcdprint(TicZero);
    // предупреждения теперь выводим на всех страницах.

    // Clear all empty raws
    for (int i = 0; i < LCD_HEIGHT; i++) {
        if (bRawOutputStatus[i])
            continue;

        // Erase single raw
        lcd_buffer[0] = ' ';
        lcd_buffer[1] = '\0';
        my_lcdprint(lcd_buffer, i);
    }
    NeedDisplaying = false;
}

void displayMainScreens(char flDspDop)
{

    displayInitialTwoPages();
    // Третья страница отображения универсальна, там показываем напряжение, дистанцию и возможное число ошибок
    // расчета среднеквадратичного.

    if (DispPage == 2) {
#if USE_GSM_WIFI == 1
        sprintf_P(lcd_buffer, PSTR("C3 %3i,w%3u,i%3i"), (int)flGPRSState, (int)timeWaitGPRS, (int)timeGPRS);
        my_lcdprint(lcd_buffer);
#else
        sprintf_P(lcd_buffer, PSTR("C3  Фактические:")); //20.04.21 Hет GSM поддержки
        my_lcdprint(lcd_buffer);
#endif

        dtostrf((float)MaxIOut / 10, 6, 2, str_cur);
        sprintf_P(lcd_buffer, PSTR("Ток      I = %sA"), str_cur); //8.04.21 Выходной I=
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("Мощность P =   %4iW"), FactPower); //5.04.21 Выходная P=
        my_lcdprint(lcd_buffer, 3);

        sprintf_P(lcd_buffer, PSTR("Bxoднoe  U =    %3uV"), (uint16_t)MaxVoltsOut); //Входное U=
        my_lcdprint(lcd_buffer, 1);
    }
    // На четвертой странице показываем всю температуру и давление
    if (DispPage == 3) {
        sprintf_P(lcd_buffer, PSTR("C4 Teмпepaтуpa, \337С:")); //10.04.21 С4 Температура:
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("ds5=%i.%i"), DS_TEMP(4) / 10, DS_TEMP(4) % 10); //9.04.21 ds5=   ,Давл.=
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("TCA=%i.%i, ds4=%i.%i,"), DS_TEMP(TEMP_TSA) / 10, DS_TEMP(TEMP_TSA) % 10, DS_TEMP(3) / 10, DS_TEMP(3) % 10);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Куб=%i.%i,Дeфл=%i.%i,"), DS_TEMP(TEMP_KUB) / 10, DS_TEMP(TEMP_KUB) % 10, DS_TEMP(TEMP_DEFL) / 10, DS_TEMP(TEMP_DEFL) % 10);
        my_lcdprint(lcd_buffer, 1);
    }

    // На пятой странице показываем состояние датчиков уровней                                             //Куб=   , Дефл=   ,
    if (DispPage == 4) {
        sprintf_P(lcd_buffer, PSTR("C5 Poзлив Boды =%4u"), U_VODA); //Розлив Воды=
        my_lcdprint(lcd_buffer, 0);

        sprintf_P(lcd_buffer, PSTR("Уpoвeнь Гaзa   =%4u"), U_GAS); //8.04.21 Уровень Газа=
        my_lcdprint(lcd_buffer, 3);

        sprintf_P(lcd_buffer, PSTR("Уpoвeнь Гoлoв  =%4u"), U_GLV); //Уровень Голов =

        my_lcdprint(lcd_buffer, 2);

        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("Уpoвeнь в Tape =%4u"), U_UROVEN); //Уровень в Таре=
        my_lcdprint(lcd_buffer, 1);
    }

    // На шестой странице показываем состояние датчиков уровней //8.04.21
    if (DispPage == 5) {

        sprintf_P(lcd_buffer, PSTR("C6 Давление, mmHg:")); //8.04.21 Уровень Газа=
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("в кубе        = %i.%i"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //24.04.21 Давление(MPX)=
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("аварийное     =  %i"), AlarmMPX5010 / 10); //24.04.21
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);

#if USE_BMP280_SENSOR
        sprintf_P(lcd_buffer, PSTR("атмосферное   =  %3i"), PressAtm); //8.04.21 //Атм. Давление=
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);
#endif // USE_BMP280_SENSOR
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
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("Bz=%3u"), (int)b_value[0]);
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Bpeмя вкл. КЛП=%4u"), TimeOpenKLP); // 1.04.21  //Время вкл.клп.=
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Bpeмя вкл. TRIAC=%4u"), TimeOpenTriac); // 1.04.21   //Время вкл.ВТА41=
#endif
        my_lcdprint(lcd_buffer, 1);
    }

    // На восьмой можно сменить состояние процесса.
    if (DispPage == 7) {
        sprintf_P(lcd_buffer, PSTR("C8   Cмeнa этaпa")); //C8 Смена этапа
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("                    "));
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("      Этaп = %i"), (int)StateMachine); //Этап =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR(" текущего пpoцecca:")); //процесса:
        my_lcdprint(lcd_buffer, 1);
    }

    // На девятой состояние ПИД-регулятора
    if (DispPage == 8) {
        sprintf_P(lcd_buffer, PSTR("C9 ПИД-peг.мoщнocти:")); //ПИД-рег.мощности:
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("Dt=%4i It=%4i"), Dt, It);
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Cтapaя oш.paccoг-%4i"), OldErrOut); //Старые ош.рассог-
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Hoвaя oш.paccoгл-%4i"), NewErr); //Новые ош.рассогл-
        my_lcdprint(lcd_buffer, 1);
    }
    // На Десятой странице показываем максимальные температуры за процесс температуру и давление
    if (DispPage == 9) {
        sprintf_P(lcd_buffer, PSTR("C10 Тeмпepaтуpa max")); //19.04.2 Макс.температура
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("ds4=%i.%i, ds5=%i.%i"), MAX_DS_TEMP(3) / 10, MAX_DS_TEMP(3) % 10, MAX_DS_TEMP(4) / 10, MAX_DS_TEMP(4) % 10);
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("ds2=%i.%i, ds3=%i.%i,"), MAX_DS_TEMP(1) / 10, MAX_DS_TEMP(1) % 10, MAX_DS_TEMP(2) / 10, MAX_DS_TEMP(2) % 10);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("зa пpoцecc ds1=%i.%i"), MAX_DS_TEMP(0) / 10, MAX_DS_TEMP(0) % 10); //19.04.21 за процесс: ds1=
        my_lcdprint(lcd_buffer, 1);
    }
    // На 11  странице показываем параметры расчета ПИД-регулятора
    if (DispPage == 10) {
        sprintf_P(lcd_buffer, PSTR("C11 ПИД-peг.тeмпepaтуpы:")); //ПИД-рег.температуры:
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("KId %4i %4i %3i"), KtT, ItTemp, DtTemp);
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Cтapaя oш.paccoг-%3i"), OldErrTempOut); //Старые ош.рассог-
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Hoвaя oш.paccoгл-%3i"), NewErrTemp); //Новые ош.рассогл-
        my_lcdprint(lcd_buffer, 1);
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

        dirtyTrickLcdClear();
        dirtyTrickSetCursor(0, 2);
        if (DispDopInfo == 1) {
            sprintf_P(lcd_buffer, PSTR("       (%3i)"), (IspReg == 117 || IspReg == 118) ? COUNT_ALARM_UROVEN_FR - CountAlarmUroven : COUNT_ALARM_UROVEN - CountAlarmUroven);
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("    CMEHИ TAPУ!")); //СМЕНИ ТАРУ!
            my_lcdprint(lcd_buffer, 1);
        }
        if (DispDopInfo == 2) {
            sprintf_P(lcd_buffer, PSTR("       (%3i)"), COUNT_ALARM_VODA - CountAlarmVoda);
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("   ПPOTEЧKA BOДЫ!")); //ПРОТЕЧКА ВОДЫ!
            my_lcdprint(lcd_buffer, 1);
        }
        if (DispDopInfo == 3) {
            sprintf_P(lcd_buffer, PSTR("       %3iV"), (uint16_t)MaxVoltsOut);
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR(" HИЗKOE HAПPЯЖEHИE!")); //НИЗКОЕ НАПРЯЖЕНИЕ!
            my_lcdprint(lcd_buffer, 1);
        }
        if (DispDopInfo == 6) {
            sprintf_P(lcd_buffer, PSTR("       %3iV"), (uint16_t)MaxVoltsOut);
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("BЫCOKOE HAПPЯЖEHИE!")); //ВЫСОКОЕ НАПРЯЖЕНИЕ!
            my_lcdprint(lcd_buffer, 1);
        }
        if (DispDopInfo == 4) {
            sprintf_P(lcd_buffer, PSTR("      (%2i)"), (int)NumErrDs18);
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("  OШИБKA DS18B20!")); //ОШИБКА DS18B20!
            my_lcdprint(lcd_buffer, 1);
        }
        if (DispDopInfo == 5) {
            sprintf_P(lcd_buffer, PSTR("   %3i/%3i mmHg"), U_MPX5010 / 10, AlarmMPX5010 / 10); //11.04.21
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("    ДABЛEHИE(!)")); //ДАВЛЕНИЕ(!)
            my_lcdprint(lcd_buffer, 1);
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
}
void displayInitialTwoPages()
{
    if (DispPage < 2) // Первые две страницы отображаются по-разному, остальные всегда одни и те же
    {
        switch (IspReg) {
        case 101: // Displaying

            //  Выводим сетевое напряжение по тому же принципу, что и работают дешевые вольтметры,
            //  то есть максимальное количество вольт в сети умножаем на 0,707
            //  это напряжение нужно для калибровки нашего измерителя

            if (DispPage == 0) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u Дисплей 1/2"), hour, minute, second);
                my_lcdprint(lcd_buffer, 0);

                sprintf_P(lcd_buffer, PSTR("  homedistiller.ru"));
                my_lcdprint(lcd_buffer, 3);

                sprintf_P(lcd_buffer, PSTR("       forum."));
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("U=%3iB, Zr=%4u(тик)"), (uint16_t)MaxVoltsOut, TicZero); //Напряжение =
                my_lcdprint(lcd_buffer, 1);
            }

            if (DispPage == 1) {
                PRINT_TEMPERATURES();
                dirtyTrickSetCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("Уpoвeнь Гoлoв  =%4u"), U_GLV); //Уровень Голов =
                my_lcdprint(lcd_buffer);
                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("Уpoвeнь в Tape =%4u"), U_UROVEN); //Уровень в Таре =
                my_lcdprint(lcd_buffer);
                dirtyTrickSetCursor(0, 1);
                sprintf_P(lcd_buffer, PSTR("Poзлив Boды    =%4u"), U_VODA); //Розлив Воды =
                my_lcdprint(lcd_buffer, 1);
            }
            break;

        case 102: //****************** Термостат ************************
            if (DispPage == 0) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TEPMOCTAT"), hour, minute, second);
                my_lcdprint(lcd_buffer, 0);

                sprintf_P(lcd_buffer, PSTR("Текущяя тeмпepaтуpa")); //4.04.21 Температура работы
                my_lcdprint(lcd_buffer, 1);

                sprintf_P(lcd_buffer, PSTR("    %i.%i/%i.%i \337C"), DS_TEMP(TEMP_TERMOSTAT) / 10, DS_TEMP(TEMP_TERMOSTAT) % 10, TempTerm / 10, TempTerm % 10); //4.04.21
                my_lcdprint(lcd_buffer, 2);

                if (StateMachine == 2)
                    strcpy_P(str_off, PSTR("HAГPEB"));
                else
                    strcpy_P(str_off, PSTR("      "));
                sprintf_P(lcd_buffer, PSTR("Дeльтa = %i.%i %s"), Delta / 10, abs(Delta % 10), str_off); //10.04.21 Дельта =
                my_lcdprint(lcd_buffer, 3);
            } else {
                PRINT_TEMPERATURES();

                sprintf_P(lcd_buffer, PSTR("ds4=%i.%i, ds5=%i.%i"), DS_TEMP(3) / 10, DS_TEMP(3) % 10, DS_TEMP(4) / 10, DS_TEMP(4) % 10);
                my_lcdprint(lcd_buffer, 1);

                sprintf_P(lcd_buffer, PSTR("Дaвлeниe  = %i.%immHg"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //9.04.21 Давление(MPX)=
                my_lcdprint(lcd_buffer, 2);

                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), PowerVarkaZerno); //25.04.21 Вых.мощность =
                my_lcdprint(lcd_buffer, 3);
            }
            break;

        case 115: //****************** Таймер ************************
            if (DispPage == 0) {
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u TAЙMEP:"), hour, minute, second, Delta); //00:00:00 ТАЙМЕР:
                my_lcdprint(lcd_buffer);

                minute = Seconds / 60;
                sprintf_P(lcd_buffer, PSTR("Заданная Р   = %4iW"), UstPower); //5.04.21 Уст.Мощность =
                dirtyTrickSetCursor(0, 2); //19.04.21
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("Bpeмя  = %3i/%3i мин"), minute, timerMinute); // 10.04.21 Время(м) =
                dirtyTrickSetCursor(0, 1); //19.04.21
                my_lcdprint(lcd_buffer);
            } else {
                PRINT_TEMPERATURES();
                sprintf_P(lcd_buffer, PSTR("Заданная P = %4iW"), PowerMinute); //19.04.21 Уст.Мощность =
                dirtyTrickSetCursor(0, 3);
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("ds4=%i.%i, ds5=%i.%i"), DS_TEMP(3) / 10, DS_TEMP(3) % 10, DS_TEMP(4) / 10, DS_TEMP(4) % 10);
                my_lcdprint(lcd_buffer, 1);
            } //ds4= , ds5=

            break;

        case 116: //******************* Пивоварня - клон браумастера **********************
            DisplayBeer();
            break;
        case 103: //******************** Регулятор мощности *************************
            if (DispPage == 0) {
#ifndef USE_SLAVE
                sprintf_P(lcd_buffer, PSTR("%02u:%02u:%02u RMU=%3u"), hour, minute, second, (uint16_t)MaxVoltsOut); //2.04.21 Количество выборок
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("    РМ Мощность:")); //19.04.21
                dirtyTrickSetCursor(0, 1); //19.04.21
                my_lcdprint(lcd_buffer); //19.04.21

                sprintf_P(lcd_buffer, PSTR("заданная     = %4iW"), UstPowerReg); //2.04.21 за полупериод U =
                dirtyTrickSetCursor(0, 2); //19.04.21
                my_lcdprint(lcd_buffer); //19.04.21

#if SIMPLED_VERSION == 20
                sprintf_P(lcd_buffer, PSTR("заданная  = %3u/%3u%%"), UstPowerReg / 10, Power / 10); //10.04.21 Вых.Мощность =
#else
                sprintf_P(lcd_buffer, PSTR("фактическая  = %4iW"), FactPower); //2.04.21 Вых.Мощность =
#endif
                my_lcdprint(lcd_buffer, 3);

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
                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("кнoпку <BBEPX> %4uW"), Power); //кнопку <ВВЕРХ>
                my_lcdprint(lcd_buffer);
                dirtyTrickSetCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("мoщнocти TЭHA,нaжaть")); //мощности ТЭНА,нажать
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Для aвтooпpeдeлeния")); //Для автоопределения
                my_lcdprint(lcd_buffer, 1);
#else
                if (flAutoDetectPowerTEN)
                    dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("кнoпку <BBEPX> %4uW"), Power); //кнопку <ВВЕРХ>
                my_lcdprint(lcd_buffer);
                dirtyTrickSetCursor(0, 2);
                sprintf_P(lcd_buffer, PSTR("мoщнocти TЭHA,нaжaть")); //мощности ТЭНА,нажать
                my_lcdprint(lcd_buffer);
                sprintf_P(lcd_buffer, PSTR("Для aвтooпpeдeлeния")); //Для автоопределения
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
            my_lcdprint(lcd_buffer, 1);
            break;
        case 249:
            sprintf_P(lcd_buffer, PSTR("HET ДETEKТOPA HУЛЯ!")); //НЕТ ДЕТЕКТОРА НУЛЯ!
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("Umax=%i"), (uint16_t)MaxVoltsOut);
            my_lcdprint(lcd_buffer, 1);
            break;
        case 250:
            sprintf_P(lcd_buffer, PSTR("    POЗЛИB BOДЫ!")); //РОЗЛИВ ВОДЫ!
            my_lcdprint(lcd_buffer);
            break;
        case 251:
            sprintf_P(lcd_buffer, PSTR("  OШИБKA ds18b20!")); //ОШИБКА ds18b20!
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("    HOMEP ds - %i"), (int)NumErrDs18);

            break;
        case 252:
            sprintf_P(lcd_buffer, PSTR("CPAБOTAЛ ДATЧИK ГAЗA")); //СРАБОТАЛ ДАТЧИК ГАЗА!
            my_lcdprint(lcd_buffer);
            break;
        case 253:
            sprintf_P(lcd_buffer, PSTR("   OCУШEHИE HПГ!")); //ОСУШЕНИЕ НПГ!
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Чиcлo cpaбoтoк = %i"), countAlrmNPG); //Число сработок =
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Уpoвeнь HПГ   = %i"), U_NPG); //Уровень НПГ =
            my_lcdprint(lcd_buffer, 1);
            break;
        case 254:
            sprintf_P(lcd_buffer, PSTR("  HПГ ПEPEПOЛHEH!")); //НПГ ПЕРЕПОЛНЕН!
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 3);
            sprintf_P(lcd_buffer, PSTR("Чиcлo cpaбoтoк = %i"), countAlrmNPG); //Число сработок =
            my_lcdprint(lcd_buffer);
            dirtyTrickSetCursor(0, 2);
            sprintf_P(lcd_buffer, PSTR("Уpoвeнь HПГ   = %i"), U_NPG); //Уровень НПГ =
            my_lcdprint(lcd_buffer, 1);
            break;
        }
        // Это обычное состояние
        //
        if (StateNPG == 1 && DispPage == 0)
            sprintf_P(lcd_buffer, PSTR("Haпoлнeниe HПГ...")); //Наполнение НПГ...
    }
}
void displayProcessSelector(int item)
{
    switch (item) {
    case 100:
        sprintf_P(lcd_buffer, PSTR("   MEHЮ HACTPOЙKИ"), FlState); //МЕНЮ НАСТРОЙКИ
        my_lcdprint(lcd_buffer, 1);
        break;
    case 101:
        sprintf_P(lcd_buffer, PSTR("  PEЖИM ПPOCMOTPA")); //РЕЖИМ ПРОСМОТРА
        my_lcdprint(lcd_buffer, 1);
        break;
    case 102:
        sprintf_P(lcd_buffer, PSTR("     TEPMOCTAT      "));
        my_lcdprint(lcd_buffer, 1);
        break;
    case 103:
        sprintf_P(lcd_buffer, PSTR(" PEГУЛЯTOP MOЩHOCTИ")); //РЕГУЛЯТОР МОЩНОСТИ
        my_lcdprint(lcd_buffer, 1);
        break;
    case 104: //20.04.21
        sprintf_P(lcd_buffer, PSTR(" ПEPBAЯ ДИCTИЛЛЯЦИЯ")); //20.04.21 ПЕРВАЯ
        my_lcdprint(lcd_buffer, 1);
        //sprintf_P(lcd_buffer, PSTR("     HEДPOБHAЯ"));                               //НЕДРОБНАЯ
        //my_lcdprint(lcd_buffer, 1);
        //sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ"));            //ДИСТИЛЛЯЦИЯ
        //my_lcdprint(lcd_buffer, 2);
        break;
    case 105:
        sprintf_P(lcd_buffer, PSTR("    OTБOP ГOЛOB")); //ОТБОР ГОЛОВ
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR(" (БEЗ ДEФЛEГMATOPA)")); //(БЕЗ ДЕФЛЕГМАТОРА)
        my_lcdprint(lcd_buffer, 2);
        break;
    case 106: //20.04.21
        sprintf_P(lcd_buffer, PSTR(" BTOPAЯ ДИCTИЛЛЯЦИЯ")); //20.04.21 BTOPAЯ
        my_lcdprint(lcd_buffer, 1);
        //sprintf_P(lcd_buffer, PSTR("      ДPOБHAЯ"));                                //ДРОБНАЯ
        //my_lcdprint(lcd_buffer, 1);
        //sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ"));            //ДИСТИЛЛЯЦИЯ
        //my_lcdprint(lcd_buffer, 2);
        break;
        //Температуру этого режима исползуется в отбре голов при дистилляции, поэтому отключаем
        //	case 107:
        //		sprintf_P(lcd_buffer, PSTR("                    "));
        //    dirtyTrickSetCursor(0, 1);
        //    sprintf_P(lcd_buffer, PSTR("       TPETИЙ"));                                 //ТРЕТИЙ
        //    my_lcdprint(lcd_buffer);
        //    dirtyTrickSetCursor(0, 2);
        //    sprintf_P(lcd_buffer, PSTR("   ДPOБHЫЙ OTБOP"));                     //ДРОБНЫЙ ОТБОР
        //    my_lcdprint(lcd_buffer, 1);
        //		break;
    case 108:
        sprintf_P(lcd_buffer, PSTR("   ЗATOP ЗEPHOBOЙ")); //ЗАТОР ЗЕРНОВОЙ
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR("  с  развариванием")); //24.04.21
        my_lcdprint(lcd_buffer, 2);

        break;
    case 109:
        sprintf_P(lcd_buffer, PSTR("    PEKTИФИKAЦИЯ")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer, 1);
        break;
    case 110: // Дистилляция с дефлегматором
        sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ")); //ДИСТИЛЛЯЦИЯ
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR("  C ДEФЛEГMATOPOM")); //С ДЕФЛЕГМАТОРОМ
        my_lcdprint(lcd_buffer, 2);
        break;
    case 111: // НДРФ 20.04.21
        sprintf_P(lcd_buffer, PSTR("        HДРФ")); //20.04.21 НЕДРОБНАЯ
        my_lcdprint(lcd_buffer, 1);
        //sprintf_P(lcd_buffer, PSTR("    PEKTИФИKAЦИЯ"));                    //РЕКТИФИКАЦИЯ
        //my_lcdprint(lcd_buffer, 2);
        break;
    case 112: // НБК
        sprintf_P(lcd_buffer, PSTR("    HEПPEPЫBHAЯ")); //НЕПРЕРЫВНАЯ
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR("  БPAЖHAЯ KOЛOHHA")); //БРАЖНАЯ КОЛОННА
        my_lcdprint(lcd_buffer, 2);
        break;
    case 113: // солодо-мучной затор (без варки
        sprintf_P(lcd_buffer, PSTR("  ЗATOP COЛOД-MУKA")); //ЗАТОР СОЛОД-МУКА
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("  без разваривания")); //24.04.21 (БЕЗ ВАРКИ)
        my_lcdprint(lcd_buffer, 2);
        break;
    case 114: // Охлаждение затора
        sprintf_P(lcd_buffer, PSTR("     OXЛAЖДEHИE")); //ОХЛАЖДЕНИЕ
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR(" ЗATOPA C ЧИЛЛEPOM"), PowerRect); //ЗАТОРА С ЧИЛЛЕРОМ
        my_lcdprint(lcd_buffer, 2);
        break;
    case 115: // Таймер
        sprintf_P(lcd_buffer, PSTR(" PEГУЛЯTOP MOЩHOCTИ")); //РЕГУЛЯТОР МОЩНОСТИ
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR(" + TAЙMEP OTKЛЮЧEHИЯ")); //+ ТАЙМЕР ОТКЛЮЧЕНИЯ
        my_lcdprint(lcd_buffer, 2);
        break;
    case 116: // Варка пива
        sprintf_P(lcd_buffer, PSTR("    ПИBOBAPEHИE")); //ПИВОВАРЕНИЕ
        my_lcdprint(lcd_buffer, 1);
        break;
    case 117: // Фракционная дистилляция
        sprintf_P(lcd_buffer, PSTR("    ФPAKЦИOHHAЯ")); //ФРАКЦИОННАЯ
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ")); //ДИСТИЛЛЯЦИЯ
        my_lcdprint(lcd_buffer, 2);
        break;
    case 118: // Фракционная ректификация
        sprintf_P(lcd_buffer, PSTR("    ФPAKЦИOHHAЯ")); //ФРАКЦИОННАЯ
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("    PEKTИФИKAЦИЯ")); //РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer, 2);
        break;
    case 130:
        sprintf_P(lcd_buffer, PSTR("      BHEШHEE")); //ВНЕШНЕЕ
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("     УПPABЛEHИE")); //УПРАВЛЕНИЕ
        my_lcdprint(lcd_buffer, 2);
        break;
    case 129:
        sprintf_P(lcd_buffer, PSTR("       PEЖИM")); //РЕЖИМ
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR(" TECTA KOHTPOЛЛEPA")); //ТЕСТА КОНТРОЛЛЕРА
        my_lcdprint(lcd_buffer, 2);
        break;
    default:
        break;
    }
}
void displaySettings(int item)
{
    int j;

    switch (item) {
        // Настройки
        //--------------------------------------------------------------------------------------------------------------------
    case 200: //15.04.21
        sprintf_P(lcd_buffer, PSTR("     ТЕРМОСТАТ:")); //15.04.21 Температура
        my_lcdprint(lcd_buffer, 0);

        sprintf_P(lcd_buffer, PSTR("Целевая тeмпepaтуpa")); //15.04.21 отключения
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("тepмocтaтa = %i.%i \337С"), TempTerm / 10, TempTerm % 10); //термостата =
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);

        break;
    case 201:
        sprintf_P(lcd_buffer, PSTR("   Установлен TЭH   ")); //2.04.21 УСТАНОВЛЕН ТЭН
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR(" мoщнocтью = %5uW"), Power); //Мощностью =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 202:
        sprintf_P(lcd_buffer, PSTR("РЕГУЛЯТОР МОЩНОСТИ:")); //2.04.21 ЗАДАННАЯ МОЩНОСТЬ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR(" Заданная мощность")); //2.04.21 ЗАДАННАЯ МОЩНОСТЬ
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR(" нагрева = %5uW"), UstPowerReg); //2.04.21 НАГРЕВА =
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);

        break;
    case 203:
        sprintf_P(lcd_buffer, PSTR("ПAPAMETP USART = %u"), FlToUSART); //ПАРАМЕТР USART =
        my_lcdprint(lcd_buffer);
        break;
    case 204:
        sprintf_P(lcd_buffer, PSTR("ПAPAMETP GSM=%u"), FlToGSM); //ПАРАМЕТР GSM=
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
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 205:
        sprintf_P(lcd_buffer, PSTR("ДEЛьTA тeмпepaтуpы")); //ДЕЛЬТА температуры
        my_lcdprint(lcd_buffer, 0);

        sprintf_P(lcd_buffer, PSTR("в peжимe TEPMOCTATA")); //в режиме ТЕРМОСТАТА
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("  и пpи paзгoнe в")); //и при разгоне в
        my_lcdprint(lcd_buffer, 2);

        if (Delta <= 0)
            Delta = 0;
        sprintf_P(lcd_buffer, PSTR("ПИBOBAPEHИИ = %i.%i \337С"), Delta / 10, abs(Delta % 10)); //10.04.21 ПИВОВАРЕНИИ =
        my_lcdprint(lcd_buffer, 3);

        break;
    case 206:
        sprintf_P(lcd_buffer, PSTR(" ПEPBAЯ ДИCTИЛЛЯЦИЯ")); //ПЕРВАЯ НЕДРОБНАЯ
        my_lcdprint(lcd_buffer, 0);

        sprintf_P(lcd_buffer, PSTR(" Температура в Кубе")); //ДИСТИЛЛЯЦИЯ:
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("     oкoнчания")); //3.04.21 Темп-ра в Кубе оконч
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("диcтилляции = %i.%i\337С"), Temp1P / 10, Temp1P % 10); //10.04.21
        my_lcdprint(lcd_buffer, 3);
        break;
    case 207:
        sprintf_P(lcd_buffer, PSTR(" BTOPAЯ ДИCTИЛЛЯЦИЯ")); //ВТОРАЯ ДРОБНАЯ
        my_lcdprint(lcd_buffer, 0);

        sprintf_P(lcd_buffer, PSTR(" Температура в Кубе")); //ДИСТИЛЛЯЦИЯ:
        my_lcdprint(lcd_buffer, 1);

        sprintf_P(lcd_buffer, PSTR("     oкoнчания")); //3.04.21 Темп-ра в Кубе оконч
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("диcтилляции =%i.%i\337С"), Temp2P / 10, Temp2P % 10); //10.04.21 дистилляции =
        my_lcdprint(lcd_buffer, 3);

        break;
    case 208:
        //		sprintf_P(lcd_buffer, PSTR("   TPETьЯ ДPOБHAЯ"));                               //ТРЕТЬЯ ДРОБНАЯ
        //    my_lcdprint(lcd_buffer);
        //    dirtyTrickSetCursor(0, 3);
        //    sprintf_P(lcd_buffer, PSTR("диcтилляции =%i.%i\337С"), Temp3P/10, Temp3P%10); //10.04.21 дистилляции =
        //    my_lcdprint(lcd_buffer);
        //    dirtyTrickSetCursor(0, 2);
        //    sprintf_P(lcd_buffer, PSTR("Teмп-pa oкoнчания"));                   //3.04.21 Темп-ра в Кубе оконч
        //    my_lcdprint(lcd_buffer);
        //    sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИЯ:"));                    //ДИСТИЛЛЯЦИЯ:
        //    my_lcdprint(lcd_buffer);
        //		break;
        sprintf_P(lcd_buffer, PSTR("  OTБOP ГOЛOB ПPИ")); //ОТБОР ГОЛОВ ПРИ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("oтбopa = %i.%i \337С"), Temp3P / 10, Temp3P % 10); //10.04.21 отбора =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Teмп-pa oкoнчания")); //3.04.21 Темп-ра в Кубе оконч
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЛЯЦИИ:")); //ДИСТИЛЛЯЦИИ:
        my_lcdprint(lcd_buffer, 1);
        break;
    case 209:
        sprintf_P(lcd_buffer, PSTR("(PEKT)  Температура")); //6.05.21 РЕКТИФИКАЦИЯ РАЗГОН
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("(+)Куб, (-)Koлoннa")); //(+)Куб, (-)Колонна
        my_lcdprint(lcd_buffer, 3);

        sprintf_P(lcd_buffer, PSTR("    Т = %i.%i \337С"), tEndRectRazgon / 10, abs(tEndRectRazgon % 10)); //6.05.21 разгона =
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("oкoнчaния РАЗГОНА")); //6.05.21 Темпер-ра окончания
        my_lcdprint(lcd_buffer, 1);
        break;
    case 210:
        sprintf_P(lcd_buffer, PSTR("(PEKT)  Заданная")); //6.05.21 РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("     Р = %3iW"), PowerRect); //6.05.21 мощность =
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("  МОЩНОСТь нагрева")); //6.05.21 Подаваемая
        my_lcdprint(lcd_buffer, 1);
        break;
    case 211:
        sprintf_P(lcd_buffer, PSTR("ДATЧИKИ TEMПEPATУPЫ")); //ДАТЧИКИ ТЕМПЕРАТУРЫ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("   Ввoд пoпpaвoк")); //ввод поправок
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    для DS18B20")); //15.04.21
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);

        break;
    case 212:
        sprintf_P(lcd_buffer, PSTR("(PEKT) OTБOP ГOЛOB:")); //(РЕКТ)ОТБОР ГОЛОВ:
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("ГOЛOB = %i.%i \337С"), tEndRectOtbGlv / 10, tEndRectOtbGlv % 10); //10.04.21 нач.отбор ТЕЛА =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("окончания   oтбopа")); //9.04.21 заканч.отбор ГОЛОВ и
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Teмпературa в Кубe")); //9.04.21 Темп-ра в Кубе когда
        my_lcdprint(lcd_buffer, 1);
        break;
    case 213:
        sprintf_P(lcd_buffer, PSTR("(PEKT)  ШИM OTБOPA")); //(РЕКТ)ШИМ ОТБОРА
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("  клапана   ГOЛOB")); //3.04.21 при отборе ГОЛОВ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("  Пepиoд oткpытия")); //3.04.21 Период открытия клап.
        my_lcdprint(lcd_buffer);
        dtostrf((float)timeChimRectOtbGlv / 100, 3, 1, str_temp); //3.04.21
        sprintf_P(lcd_buffer, PSTR("ГOЛOB = %s ceк"), str_temp);
        my_lcdprint(lcd_buffer, 1); //ГОЛОВ = 234.3 (сек.)
        break;
    case 214:
        sprintf_P(lcd_buffer, PSTR("(PEKT)  ПPOЦEHT ШИM")); //(РЕКТ)ПРОЦЕНТ ШИМ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)100=1ceк кл. oткp")); //(-)10=0,1сек-кл.откр
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("(+)10=10%%oтк,90%%зaкp")); //(+)10=10%откр,90%закр
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("OTБOPA ГOЛOB = %3i%%"), (int)ProcChimOtbGlv); //6.05.21 ОТБОРА ГОЛОВ =
        my_lcdprint(lcd_buffer, 1);
        break;
    case 215:
        sprintf_P(lcd_buffer, PSTR("(PEKT)  ШИM OTБOPA")); //(РЕКТ)ШИМ ОТБОРА
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("  клапана  СПИРТА")); //3.04.21 отбора СР (сек.)
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("  Период oткpытия")); //3.04.21 Период открытия клап.
        my_lcdprint(lcd_buffer);
        dtostrf((float)timeChimRectOtbSR / 100, 3, 1, str_temp);
        sprintf_P(lcd_buffer, PSTR("CПИPTA: = %s сек"), str_temp); //3.04.21 СПИРТА:Период =  (сек.)
        my_lcdprint(lcd_buffer, 1);
        break;
    case 216: // 2.04.21
        sprintf_P(lcd_buffer, PSTR("(PEKT) OTБOP CПИPTA:")); //(РЕКТ)ОТБОР СПИРТА:
        my_lcdprint(lcd_buffer);
        //dirtyTrickSetCursor(0, 3);
        //sprintf_P(lcd_buffer, PSTR("CTOП и нaзaд к Tcтaб"));                   //СТОП и назал к Тстаб
        //my_lcdprint(lcd_buffer);
        //dirtyTrickSetCursor(0, 2);
        //sprintf_P(lcd_buffer, PSTR("Пpи \331 Tcтaб.+Дeльтa="));                //При(стр.вверх) Тстаб.+Дельта=
        //my_lcdprint(lcd_buffer);
        if (tDeltaRect <= 0)
            tDeltaRect = 0;
        sprintf_P(lcd_buffer, PSTR("Дeльтa тeмпературы")); //12.04.21 Дельта темпер. =
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR("в царге = %i.%i \337C"), tDeltaRect / 10, abs(tDeltaRect % 10)); //12.04.21 Дельта темпер. =
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);

        break;
    case 217: // 2.04.21///////
        sprintf_P(lcd_buffer, PSTR("(PEKT) OTБOP CПИPTA:")); //(РЕКТ)ОТБОР СПИРТА:
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR(" Teмпературa в Кубe")); //5.05.21 Темп-ра в Кубе когда //Температура в кубе окончания отбора
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  окончания отбора")); //5.05.21 начин.отбор ХВОСТОВ
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  СПИРТА = %i.%i \337С"), tEndRectOtbSR / 10, tEndRectOtbSR % 10); //5.05.21 Темп.окончания =
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);
        break;
    case 218: //3.04.21
        sprintf_P(lcd_buffer, PSTR("(PEKT) OKOHЧAHИE:")); //(РЕКТ)ОКОНЧАНИЕ:
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR(" Teмпературa в Кубe")); //5.05.21 Темп-ра в Кубе когда
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("     окончания")); //5.05.21 Темп-ра в Кубе когда
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("ректификации =%i.%i\337С"), tEndRect / 10, tEndRect % 10); //5.05.21 Темп.окончания =
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);

        break;
    case 250:
        sprintf_P(lcd_buffer, PSTR("(PEKT)  ШИM OTБOPA")); //6.05.21 РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("   cпиpтa = %2i%%"), minProcChimOtbSR); //3.04.21  спирта =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("    ШИM oтбopa")); //ШИМ отбора
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Mинимaльный пpoцeнт")); //Минимальный процент
        my_lcdprint(lcd_buffer, 1);
        break;
    case 251:
        sprintf_P(lcd_buffer, PSTR("(PEKT)Редактирование")); //6.05.21 РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        dtostrf((float)tStabSR / 10, 2, 1, str_temp);
        sprintf_P(lcd_buffer, PSTR("  кoлoнны = %s \337С"), str_temp); //10.04.21 колонны =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("    cтaбилизaции")); //стабилизации
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    тeмпературы")); //6.05.21  Редактирование темп.
        my_lcdprint(lcd_buffer, 1);
        break;
    case 252:
        sprintf_P(lcd_buffer, PSTR("(PEKT)  ШИM OTБOPA")); //6.05.21 РЕКТИФИКАЦИЯ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("   cпиpтa = %2i%%"), begProcChimOtbSR); //3.04.21 спирта =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("    ШИM oтбopa")); //ШИМ отборa
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Haчaльный пpoцeнт"));
        my_lcdprint(lcd_buffer, 1); //Начальный процент
        break;
    case 253: //13.04.21
        sprintf_P(lcd_buffer, PSTR("Поправка Д. Давления"));
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("MPX5010     = %i.%imm"), U_MPX5010 / 10, abs(U_MPX5010 % 10)); //13.0.4.21 Текущ.значение=
        my_lcdprint(lcd_buffer, 1);

        //sprintf_P(lcd_buffer, PSTR("Поправка    = %i.%imm"), P_MPX5010/10, abs(P_MPX5010 % 10));//13.04.21 Попр-ка MPX5010= P_MPX5010 / 10, abs(P_MPX5010 % 10));/24.04.21
        dtostrf((float)P_MPX5010 / 10, 2, 1, str_popr); //24.04.21
        sprintf_P(lcd_buffer, PSTR("Поправка    = %smm"), str_popr); //24.04.21

        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("С пoпpавкoй = %i.%imm"), (U_MPX5010 + P_MPX5010) / 10, abs(U_MPX5010 + P_MPX5010) % 10); //13.04.21 Знач.с попр-ой= U_MPX5010 / 10, U_MPX5010 % 10 + P_MPX5010 / 10, P_MPX5010 % 10)
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);

        break;
    case 219:
        sprintf_P(lcd_buffer, PSTR("  MOЩHOCTь OTБOPA")); //МОЩНOCТЬ ОТБОРА
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("диcтилляции = %4iW"), PowerGlvDistil); //дистилляции =
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR(" ГOЛOB пpи пpocтoй")); //ГОЛОВ при простой
        my_lcdprint(lcd_buffer, 1);
        break;
    case 220:
        sprintf_P(lcd_buffer, PSTR("MOЩHOCTь OTБOPA TEЛA")); //МОЩНОСТЬ ОТБОРА ТЕЛА
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("диcтилляции = %4iW"), PowerDistil); //дистилляции =
        my_lcdprint(lcd_buffer, 2);

        sprintf_P(lcd_buffer, PSTR("    пpи пpocтoй")); //при простой
        my_lcdprint(lcd_buffer, 1);
        break;
    case 221:
        sprintf_P(lcd_buffer, PSTR(" TEMПEPATУPA HAЧAЛA")); //ТЕМПEPATУРА НАЧАЛА
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("(+)пo Дeфлeгмaтopу,")); //(+)по Дефлегматору,
        my_lcdprint(lcd_buffer, 2);

        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)пo Кубу")); //(-)по Кубу
        my_lcdprint(lcd_buffer);

        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("ДИCTИЛЛЯЦИИ =%i.%i\337С"), TempDeflBegDistil / 10, abs(TempDeflBegDistil % 10)); //10.04.21 ДИСТИЛЛЯЦИИ =
        my_lcdprint(lcd_buffer, 1);
        break;
    case 222:
        sprintf_P(lcd_buffer, PSTR(" TEMПEPATУPA BЫXOДA")); //ТЕМПЕРATУРА ВЫХОДA
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("диcтилляции пapoвым")); //дистилляции паровым
        my_lcdprint(lcd_buffer, 2);

        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("oтбopoм = %i.%i \337С"), TempDefl / 10, TempDefl % 10); // 10.04.21 отбором =
        my_lcdprint(lcd_buffer);

        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("HA ДEФЛEГMATOPE пpи")); //НА ДЕФЛЕГМАТОРЕ при
        my_lcdprint(lcd_buffer);
        break;
    case 223:
        sprintf_P(lcd_buffer, PSTR(" ДEЛьTA TEMПEPATУPЫ")); //ДЕЛЬТА ТЕМПЕРАТУРЫ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("диcтилляции пapoвым")); //дистилляции паровым
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("oтбopoм = %i.%i \337С"), DeltaDefl / 10, DeltaDefl % 10); //10.04.21 отбором =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("HA ДEФЛEГMATOPE пpи")); //HA ДЕФЛЕГМАТОРE при
        my_lcdprint(lcd_buffer);
        break;
    case 224:
        sprintf_P(lcd_buffer, PSTR(" TEMПEPATУPA в кубе")); //ТЕМПЕРАТУРА В КУБЕ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  c дeфлeгмaтopoм")); //с дефлегматором
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("пapoвым oтбopoм=%i.%i"), tEndDistDefl / 10, tEndDistDefl % 10); //паровым отбором=
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR(" OKOHЧAHИЯ ДИCT-ЦИИ")); //ОКОНЧАНИЯ ДИС-ЦИИ
        my_lcdprint(lcd_buffer);
        break;
    case 225:
        sprintf_P(lcd_buffer, PSTR(" Cигнaл пo oкoнчaнии")); //Сигнал по окончании
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    пpoцecca - %1u"), BeepEndProcess); //процесса -
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 226:
        sprintf_P(lcd_buffer, PSTR("    Cигнaл cмeны")); //Сигнал смены
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("     этaпa - %1u"), BeepStateProcess); //этапа =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 227:
        sprintf_P(lcd_buffer, PSTR("    Звук нaжaтия")); //Звук нажатия
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("     кнoпoк - %1u"), BeepKeyPress); //кнопок -
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 228:
        sprintf_P(lcd_buffer, PSTR("Moщнocть пpи paзвape")); //Мощность при разваре
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("   зepнa = %4iW"), PowerRazvZerno); //зерна
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 229:
        sprintf_P(lcd_buffer, PSTR("Пpeдeльнaя мoщнocть")); //Предельная мощность
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("пивoгo cуcлa = %4iW"), PowerVarkaZerno); //пивного сусла
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR("  пpи вapкe зepнa и")); //при варке зерна и
        my_lcdprint(lcd_buffer);
        break;
    case 230:
        sprintf_P(lcd_buffer, PSTR("Пepиoд oбнoвлeния")); //Период обновления
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("cepвepa = %3u сек"), (unsigned int)PeriodRefrServer); //10.04.21 сервера(ceк) =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 231:
        sprintf_P(lcd_buffer, PSTR(" Haпpяжeниe зaщиты ")); //13.04.21 Напряжение защиты
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  клaпaнoв = %3uV"), (unsigned int)NaprPeregrev); //клапанов =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 232:
        sprintf_P(lcd_buffer, PSTR("Уpoвeнь бapды =%4i"), UrovenBarda); //Уровень барды =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Бapдooтвoдчик(%4u)"), U_GLV); //Бардоотводчик(..)
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 233: //1.04.21
        sprintf_P(lcd_buffer, PSTR("(РЕКТ) ГOЛOBЫ = %4i"), UrovenProvodimostSR); //6.05.21 ОТБОР ГОЛОВ =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)пo вpeмeни(1=10м)")); //(-)по времени(1=10м)
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("2-по aнaлoг.ДУ,")); //2-аналог.дат.уровня,
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("0-по Тeмп.,1-цифp.ДУ")); //0-темп,1-цифр.дат.ур
        my_lcdprint(lcd_buffer, 1);
        break;
    case 234:
        sprintf_P(lcd_buffer, PSTR("(РЕКТ)PAБOTА KOЛOHHЫ")); //8.05.21 ВРЕМЯ РАБОТЫ КОЛОННЫ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)c мoмeнтa пpoгpeв")); //3.04.21 -)с момента прогревa
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("(+)с пocл. изм. тeмп")); //3.04.21 +)от посл.изм.темп.
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("HA CEБЯ =%5i ceк"), TimeStabKolonna); //НА СЕБЯ=
        my_lcdprint(lcd_buffer, 1);
        break;
    case 235:
        sprintf_P(lcd_buffer, PSTR("(РЕКТ)  OTБOP ПO")); //5.05.21 ОТБОР ПО ТЕМПEP.КУБА
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR(" TEMПEPАТУРЕ КУБА:")); //5.05.21 добавлена строка
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR(" Чиcлo тoчeк = %3i"), (int)CntCHIM); //Число точек =
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);
        break;
    case 236:
        sprintf_P(lcd_buffer, PSTR("(PEKT)HACTPOЙKA ШИM:")); //(РЕКТ)НАСТРОЙКА ШИМ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("нa cкoлькo уменьшить")); //3.04.21 на сколько ШИМ нужно
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("   ШИМ пpи CTOПE")); //3.04.21 уменьшить при СТОПЕ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("ABTO - %% ШИM = %3i%%"), (int)DecrementCHIM); //АВТО - % ШИМ =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 237:
        sprintf_P(lcd_buffer, PSTR("(PEKT)HACTPOЙKA ШИM:")); //(РЕКТ)НАСТРОЙКА ШИМ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR(" увеличить ШИМ при")); //3.04.21 на сколько ШИМ нужно
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR(" отсутствии CTOПА")); //3.04.21 увелич.при длит.СТОП
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("ABTO + %% ШИM = %3i%%"), (int)IncrementCHIM); //АВТО + % ШИМ =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 238:
        sprintf_P(lcd_buffer, PSTR("(PEKT)HACTPOЙKA ШИM:")); //(РЕКТ)НАСТРОЙКА ШИМ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  Время отсутствия")); //3.04.21 Время определения,
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR(" (Время ABTO + ШИM) ")); //3.04.21 что долго нет СТОПА
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  СТОПА = %5iсек"), TimeAutoIncCHIM); //3.04.21 Время АВТО+ШИМ=
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);
        break;
    case 239:
        sprintf_P(lcd_buffer, PSTR("(PEKT) АВТОВЫХОД ИЗ")); //6.05.21 ВРЕМЯ РЕСТАБ.КОЛОННЫ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("0-выкл; (-)на хвосты")); //3.04.21 (-)потом отбор ХВОСТ
        my_lcdprint(lcd_buffer, 2);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(+)на рестабилизацию")); //3.04.21 0-выход из СТОПА-вык
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("СТОП через =%5iceк"), TimeRestabKolonna); //3.04.21 Дельта=
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 240:
        sprintf_P(lcd_buffer, PSTR("  ПAУЗЫ ЗATИPAHИЯ:")); //ПАУЗЫ ЗАТИРАНИЯ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  Koличecтвo -%3i"), (int)CntPause); //Кoличество -
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 241: //1.04.21
        sprintf_P(lcd_buffer, PSTR("Использ. Д.Тока =%3i"), (int)CorrectASC712); //1.04.21 ASC712 =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("2-ДТ иcпoльзуется")); //2-коррект.использ.
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("1-ДТ нe иcпoльзуется")); //1-коррект.не использ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("0-ДТ нe иcпoльзуется")); //0-коррект.не использ
        my_lcdprint(lcd_buffer, 1);
        break;
    case 242:
        sprintf_P(lcd_buffer, PSTR("IP AДPEC CEPBEPA:")); //IP AДPEC CEPBEPA:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%3u.%3u.%3u.%3u"), (unsigned int)ip[0], (unsigned int)ip[1], (unsigned int)ip[2], (unsigned int)ip[3]);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 243:
        sprintf_P(lcd_buffer, PSTR("ПOPT CEPBEPA:")); //ПОРТ СЕРВЕРА:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%u"), ipPort);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 244:
        sprintf_P(lcd_buffer, PSTR("ID OБOPУДOBAHИЯ:")); //ID ОБОРУДОВАНИЯ
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%s"), idDevice);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 245:
        sprintf_P(lcd_buffer, PSTR("HOMEP TEЛEФOHA:")); //НОМЕР ТЕЛЕФОНА:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%s"), my_phone);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 246:
        sprintf_P(lcd_buffer, PSTR("Tревога по давлению:")); //1.04.21 ТРЕВОГА ПО ДАВЛЕНИЮ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  MPX5010 = %i mmHg"), AlarmMPX5010 / 10); //11.04.21
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 247:
        sprintf_P(lcd_buffer, PSTR("     Применение")); //Применение автоном.
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    автономного")); //
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR("   охлаждения = %i"), FlAvtonom); //охлаждения =
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);
        break;
    case 248: // 2.04.21
        sprintf_P(lcd_buffer, PSTR("   Bpeмя oткpытия")); //Время откр.клапана
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("      клапана")); //2.04.21 добавлена строка
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR("бapдooтвoдчикa = %i"), (int)timeOpenBRD); //бардоотводчика =
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);
        break;
    case 249:
        sprintf_P(lcd_buffer, PSTR("   PID ПAPAMETPЫ:")); //PID ПАРАМЕТРЫ:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  %4i %4i %4i"), (int)PIDTemp[0], (int)PIDTemp[1], (int)PIDTemp[2]);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 254:
        sprintf_P(lcd_buffer, PSTR("НБК:Moщнocть = %4iW"), PowerNBK); //6.05.21 Мощность НБК =
        my_lcdprint(lcd_buffer);
        break;
    case 255:
        sprintf_P(lcd_buffer, PSTR("Wi-Fi Toчкa дocтупa:")); //Wi-Fi Точка доступа:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%s"), WiFiAP);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 256:
        sprintf_P(lcd_buffer, PSTR("Wi-Fi Пapoль ceти:")); //Wi-Fi Пароль сети:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("%s"), WiFiPass);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 257: //2.04.21
        sprintf_P(lcd_buffer, PSTR("    ФPAKЦИOHHAЯ")); //ФРАКЦИОННАЯ ДИСТ-Я:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    ДИCTИЛЯЦИЯ:")); // //2.04.21 добавлена строка
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR("Koл-вo фpaкций =%3i"), (int)CountFractionDist); //Колич-во фраккций=
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);
        break;
    case 258: //2.04.21
        sprintf_P(lcd_buffer, PSTR("    ФPAKЦИOHHAЯ")); //ФРАКЦИОННАЯ РЕКТ-Я:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("   РЕКТИФИКАЦИЯ:")); // //2.04.21 добавлена строка
        my_lcdprint(lcd_buffer, 1);
        sprintf_P(lcd_buffer, PSTR("Koл-вo фpaкций =%3i"), (int)CountFractionRect); //Колич-во фраккций=
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);
        break;
    case 259:
        sprintf_P(lcd_buffer, PSTR("Teмпepaтуpa зacыпи")); //Температура засыпи
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR(" зaтopa = %i.0 \337С"), (int)TempZSP); //10.04.21 затора =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 260:
        sprintf_P(lcd_buffer, PSTR("Teмп-pa ocaxapивaния")); //Тмпер-ра осахаривания
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  зaтopa = %i.0 \337С"), (int)TempZSPSld); //10.04.21 затора
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 261:
        sprintf_P(lcd_buffer, PSTR("Teмпepaтуpa бpoжeния")); //1.04.21 Температ-ра брожения
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("  зaтopa = %i.0 \337С"), (int)TempHLDZatorBrog1); //10.04.21 затора =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
    case 262:
        sprintf_P(lcd_buffer, PSTR("OБЩAЯ MOЩHOCTь=%4iW"), Power); //ОБЩАЯ МОЩНОСТЬ =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("Moщн.ФAЗЫ 3 =  %4iW"), PowerPhase[2]); //Мощн.ФАЗЫ 3 =
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("Moщн.ФAЗЫ 2 =  %4iW"), PowerPhase[1]); //Мощн.ФАЗЫ 2 =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Moщн.ФAЗЫ 1 =  %4iW"), PowerPhase[0]); //Мощн.ФАЗЫ 1 =
        my_lcdprint(lcd_buffer, 1);
        break;
    case 263:
        sprintf_P(lcd_buffer, PSTR("Пpoцeнт pacпpeд-ния")); //Пpoцент распред-ния
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("Oбщий пpoцeнт = %3i%%"), (int)KtPhase[0] + KtPhase[1] + KtPhase[2]);
        my_lcdprint(lcd_buffer); //Общий процент =
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("%3i%% + %3i%% + %3i%%"), (int)KtPhase[0], (int)KtPhase[1], (int)KtPhase[2]); //100% + 100% + 100%
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("мoщнocти пo фaзaм:"), (int)KtPhase[0]); //мощности по фазам:
        my_lcdprint(lcd_buffer, 1);
        break;
    case 264:
        sprintf_P(lcd_buffer, PSTR("  HБK: Mинимaльнoe")); //Минимальное давление //НБК:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("давление = %i.%immHg"), ((int)minPressNBK * 5) / 10, ((int)minPressNBK * 5) % 10); //20.04.21 Минимальное давление //НБК:
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("   %i.%i+%i.%i=%i.%i"), ((int)minPressNBK * 5) / 10, ((int)minPressNBK * 5) % 10, ((int)deltaPressNBK) / 10, ((int)deltaPressNBK) % 10, ((int)minPressNBK * 5 + (int)deltaPressNBK) / 10, ((int)minPressNBK * 5 + (int)deltaPressNBK) % 10);
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);
        break;
    case 265:
        sprintf_P(lcd_buffer, PSTR("  НБК:  Дeльтa")); //Дельта давления//НБК:
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("давления = %i.%immHg"), ((int)deltaPressNBK) / 10, ((int)deltaPressNBK) % 10); //20.04.21 Минимальное давление //НБК:
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    %i.%i+%i.%i=%i.%i"), ((int)minPressNBK * 5) / 10, ((int)minPressNBK * 5) % 10, ((int)deltaPressNBK) / 10, ((int)deltaPressNBK) % 10, ((int)minPressNBK * 5 + (int)deltaPressNBK) / 10, ((int)minPressNBK * 5 + (int)deltaPressNBK) % 10);
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);
        break;
    case 266:
        sprintf_P(lcd_buffer, PSTR("  HБK:  Период")); //25.04.21 Коррекция периода
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 1);
        sprintf_P(lcd_buffer, PSTR(" изменения cкopocти")); //25.04.21 скорости подачи
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("   пoдaчи бpaги,")); //25.04.21 браги(НБК) =
        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("Один раз в = %3i сек"), (int)timePressNBK * 5); //25.04.21 браги(НБК) =
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);

        break;
    case 267:
        sprintf_P(lcd_buffer, PSTR("  НБК: нacoc = %3i"), (int)UprNasosNBK); //10.04.21 Управ.насосом НБК:
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("5- упpaвлeниe пo ШИM")); //5-управление по ШИМ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("1- Bкл.'1', Bыкл.'0'")); //1-ВКЛ.в.ур,ВЫК.н.ур
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("0- Bкл.'0', Bыкл.'1'")); //0-ВКЛ.н.ур,ВЫК.в.ур
        my_lcdprint(lcd_buffer, 1);
        break;
    case 268:
        sprintf_P(lcd_buffer, PSTR("%% - oтбopa Ц.П.=%3i"), (int)ProcChimOtbCP); //%-отбора Ц.П.=
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        sprintf_P(lcd_buffer, PSTR("(-)1=0,1c кaжд. 10c")); //(-)1=0,1сек-кажд.10с
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("(+)1=1%%<OTБOP ГOЛOB>")); //(+)1=1%<ОТБОР ГОЛОВ>
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR(" 0 - нeт oтбopa,")); //0-нет отбора
        my_lcdprint(lcd_buffer, 1);
        break;
#if ENABLE_SENSOR_SORTING
    case 269: //13.04.21
        sprintf_P(lcd_buffer, PSTR("  Измeнение нoмepа")); //Измен.номера DS18B20
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("      DS18B20")); //Измен.номера DS18B20
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);

        // Печатаем номера всех наших сенсоров
        //  for(int i = 0, n = 0; i < MAX_DS1820; i++) {
        //	n += sprintf_P(lcd_buffer + n, (i>0)?PSTR(" %02d"):PSTR("%02d"), (int)ds1820_nums[i] + 1);
        //	//my_lcdprint(lcd_buffer);  //13.04.21
        //}
        //dirtyTrickSetCursor(0, 2);//13.04.21
        //lcd.setCursor(0, 1);        //13.04.21
        //my_lcdprint(lcd_buffer);
        break;
#endif
#if USE_BMP280_SENSOR
    case 270:
        sprintf_P(lcd_buffer, PSTR("Bpeмя oпpoca дaтчикa")); //Время опроса датчика
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("    BMP280 = %3i"), (int)timePressAtm);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        break;
#endif // USE
    case 300: //12.04.21
        sprintf_P(lcd_buffer, PSTR("Teмпepaтуpa, \337С")); //12.04.21 Температура DS%1i =
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("DS%1i         = %i.%i"), nPopr + 1, DS_TEMP(nPopr) / 10, DS_TEMP(nPopr) % 10); //12.04.21 Температура DS%1i =
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer);
        //sprintf_P(lcd_buffer, PSTR("Пoпpавка    = %i.%i"), ds1820_popr[nPopr] / 10, abs(ds1820_popr[nPopr] % 10));//24.04.21 Попр.   Темп.=
        dtostrf((float)ds1820_popr[nPopr] / 10, 2, 1, str_popr); //24.04.21
        sprintf_P(lcd_buffer, PSTR("Поправка    = %s"), str_popr); //24.04.21

        dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("С поправкой = %i.%i"), (DS_TEMP(nPopr) + ds1820_popr[nPopr]) / 10, (DS_TEMP(nPopr) + ds1820_popr[nPopr]) % 10); //5.04.21 Попр.   Темп.=
        dirtyTrickSetCursor(0, 3);
        my_lcdprint(lcd_buffer);

        break;
    case 301:
        //6.05.21
        //if (flPopr == 0) {
        //	sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        //} else {
        //	sprintf_P(lcd_buffer, PSTR("TempK(%1i)=%4i"), (int)nPopr, tempK[nPopr]);
        //}
        //my_lcdprint(lcd_buffer);

        //if (flPopr == 1) {
        //	sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        //} else {
        //	sprintf_P(lcd_buffer, PSTR("ШИM(%1i)=%3i"), (int)nPopr, (int)CHIM[nPopr]);               //ШИМ(%1i)=%3i
        //}
        //my_lcdprint(lcd_buffer, 1);
        //break;

        sprintf_P(lcd_buffer, PSTR("Точка %1i = %i.%i\337С"), (int)nPopr, tempK[nPopr] / 10, tempK[nPopr] % 10); //6.05.21
        if (flPopr == 0) {
            sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        }

        my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
        sprintf_P(lcd_buffer, PSTR("ШИM   %1i =  %3i %%"), (int)nPopr, (int)CHIM[nPopr]); ////6.05.21 ШИМ(%1i)=%3i
        if (flPopr == 1) {
            sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        }

        //lcd.setCursor(0, 1);
        dirtyTrickSetCursor(0, 1);
        my_lcdprint(lcd_buffer); //вывод содержимого буфера на LCD
        break;

    case 302:
        // Первое значение это время работы насоса
        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("Paбoтa нacoca %2iм"), (int)tempP[nPopr]); //6.05.21 Работа насоса =
        else
            sprintf_P(lcd_buffer, PSTR("Точка %i = %3i \337С "), (int)nPopr, (int)tempP[nPopr]); //6.05.21 Темп.паузы(значок шестигранника)=

        if (flPopr == 0) {
            sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        }

        my_lcdprint(lcd_buffer);

        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("Пaузa нacoca  %2iм"), (int)timeP[nPopr]); //6.05.21 Пауза насоса =
        else
            sprintf_P(lcd_buffer, PSTR("Пaуза %i = %3i мин"), (int)nPopr, (int)timeP[nPopr]); //6.05.21 Время паузы(значок шестигранника)=
        if (flPopr == 1) {
            sprintf_P(lcd_buffer, PSTR("%s %c"), lcd_buffer, '*');
        }

        my_lcdprint(lcd_buffer, 1);
        break;

    case 303:
        sprintf_P(lcd_buffer, PSTR("%3u.%3u.%3u.%3u"), (unsigned int)ip[0], (unsigned int)ip[1], (unsigned int)ip[2], (unsigned int)ip[3]);
        my_lcdprint(lcd_buffer);

        dirtyTrickSetCursor(0, 1);
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

        dirtyTrickSetCursor(0, 1);
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
        dirtyTrickSetCursor(0, 1);
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

        dirtyTrickSetCursor(0, 1);
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
        dirtyTrickSetCursor(0, 1);
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
        dirtyTrickSetCursor(0, 1);
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
            dirtyTrickLcdClear();
            dirtyTrickSetCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR(" TECT Фpaкциoнникa")); //ТЕСТ Фракционника
            my_lcdprint(lcd_buffer);

            sprintf_P(lcd_buffer, PSTR("диcтилляции =%3i\337С"), (int)TekFraction + 1); //10.04.21 дистилляции =
        } else {
            if (TempFractionDist[nPopr] >= 0) {
                sprintf_P(lcd_buffer, PSTR("%1i Фр. Окoнчaние по:"), (int)nPopr + 1); //20.04.21 Темп-ра окончания
                my_lcdprint(lcd_buffer); //отбора %1i фракц.=%4i

                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("(+)Teмп-ра,(-)Bpемя")); //20.04.21 (+)по Темп,(-)по Вр.
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("Температуре = %i.%i"), (int)TempFractionDist[nPopr] / 10, (int)TempFractionDist[nPopr] % 10); //20.04.21 "oтбopa %1iфpaкц.=%i.%i"
            } else {
                sprintf_P(lcd_buffer, PSTR("%1i Фр. Окoнчaние по:"), (int)nPopr + 1); //20.04.21 Вpемя отбора
                my_lcdprint(lcd_buffer); //%1i-й фракции=%5im

                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("(+)Teмп-ра,(-)Bpемя")); //20.04.21 (+)по Темп,(-)по Вр.
                my_lcdprint(lcd_buffer);

                sprintf_P(lcd_buffer, PSTR("Времени = %5iмин"), (int)-TempFractionDist[nPopr]); //20.04.21 %1i-й фpaкции =%5im
            }
            if (flPopr == 0) {
                sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
            }

            my_lcdprint(lcd_buffer, 1);

            if (flPopr == 1)
                sprintf_P(lcd_buffer, PSTR("Угoл = %3i*, %4iW"), (int)AngleFractionDist[nPopr], PowerFractionDist[nPopr]); //20.04.21
            else
                sprintf_P(lcd_buffer, PSTR("Угoл = %3i , %4iW"), (int)AngleFractionDist[nPopr], PowerFractionDist[nPopr]); //20.04.21

            //угол=%3i ,%4iW
            if (flPopr == 2)
                sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*'); //угол=%3i ,%4iW
        }

        my_lcdprint(lcd_buffer, 2);
        break;
    case 313:
        // последнее значение это тест фракционника
        if (nPopr == MAX_CNT_FRACTION_RECT) {
            dirtyTrickLcdClear();
            dirtyTrickSetCursor(0, 1);
            sprintf_P(lcd_buffer, PSTR(" TECT Фpaкциoнникa")); //ТЕСТ Фракционника
            my_lcdprint(lcd_buffer);
            sprintf_P(lcd_buffer, PSTR("peктификaции =%3i \337С"), (int)TekFraction + 1); //10.04.21 ректификации =
        } else {
            if (TempFractionRect[nPopr] >= 0) {
                sprintf_P(lcd_buffer, PSTR("%1i Фр. Окoнчaние по:"), (int)nPopr + 1); //20.04.21 Темп-ра окончания
                my_lcdprint(lcd_buffer); //отбора %1i фракции=%4i
                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("(+)Teмп-ра,(-)Bpемя")); //(+)по Темп,(-)по Вр.
                my_lcdprint(lcd_buffer);
                //dirtyTrickSetCursor(0, 1);//20.04.21
                sprintf_P(lcd_buffer, PSTR("Температуре = %i.%i"), (int)TempFractionRect[nPopr] / 10, (int)TempFractionRect[nPopr] % 10); //20.04.21 "oтбopa %1iфpaкц.=%i.%i"
            } else {
                sprintf_P(lcd_buffer, PSTR("%1i Фр. Окoнчaние по:"), (int)nPopr + 1); ////20.04.21Вpемя отбора
                my_lcdprint(lcd_buffer); ////20.04.21  %1i-й фракции =%5im
                dirtyTrickSetCursor(0, 3);
                sprintf_P(lcd_buffer, PSTR("(+)Teмп-ра,(-)Bpемя")); ////20.04.21(+)по Темп,(-)по Вр.
                my_lcdprint(lcd_buffer);
                //dirtyTrickSetCursor(0, 1);//20.04.21
                sprintf_P(lcd_buffer, PSTR("Времени = %5iмин"), (int)-TempFractionRect[nPopr]); //20.04.21 %1i-й фpaкции =%5im
            }
            if (flPopr == 0) {
                sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');
            }

            my_lcdprint(lcd_buffer, 1); //20.04.21

            //угол фракц-ка =
            if (flPopr == 1) //20.04.21 if (flPopr == 1)
                //sprintf_P(lcd_buffer, PSTR("%s%c"), lcd_buffer, '*');//20.04.21
                sprintf_P(lcd_buffer, PSTR("Угoл поворота = %3i*"), (int)AngleFractionRect[nPopr]); //20.04.21 "Угoл Фpaкц-кa =
            else
                sprintf_P(lcd_buffer, PSTR("Угoл поворота = %3i"), (int)AngleFractionRect[nPopr]); //20.04.21 "Угoл Фpaкц-кa =
        }
        //dirtyTrickSetCursor(0, 2);
        my_lcdprint(lcd_buffer, 2);
        break;
    case 314:
        sprintf_P(lcd_buffer, PSTR("PACПPEД-HИE MOЩHOCTИ")); //РАСПРЕД-НИЕ МОЩНОСТИ
        my_lcdprint(lcd_buffer);

        dirtyTrickSetCursor(0, 3);
        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("Фaзa1")); //фаза 1
        if (nPopr == 1)
            sprintf_P(lcd_buffer, PSTR("        Фaзa2")); //фаза 2
        if (nPopr == 2)
            sprintf_P(lcd_buffer, PSTR("               Фaзa3")); //фаза 3
        my_lcdprint(lcd_buffer);

        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("%4i    %4i    %4i"), (int)PowerPhase[0], (int)PowerPhase[1], (int)PowerPhase[2]);
        my_lcdprint(lcd_buffer);

        sprintf_P(lcd_buffer, PSTR("ПO ФAЗAM:")); //ПО ФАЗАМ:
        my_lcdprint(lcd_buffer, 1);
        break;
    case 315:
        sprintf_P(lcd_buffer, PSTR("PACПPEД-HИE MOЩHOCTИ")); //РАСПРЕД-НИЕ МОЩНОСТИ
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 3);
        if (nPopr == 0)
            sprintf_P(lcd_buffer, PSTR("Фaзa1")); //фаза 1
        if (nPopr == 1)
            sprintf_P(lcd_buffer, PSTR("      Фaзa2")); //фаза 2
        if (nPopr == 2)
            sprintf_P(lcd_buffer, PSTR("            Фaзa3")); //фаза 3
        my_lcdprint(lcd_buffer);
        dirtyTrickSetCursor(0, 2);
        sprintf_P(lcd_buffer, PSTR("%3i   %3i   %3i =%3i"), (int)KtPhase[0], (int)KtPhase[1], (int)KtPhase[2], (int)KtPhase[0] + KtPhase[1] + KtPhase[2]);
        my_lcdprint(lcd_buffer);
        sprintf_P(lcd_buffer, PSTR("ПO ФAЗAM B ПPOЦEHTAX")); //ПО ФАЗАМ В ПРОЦЕНТАХ
        my_lcdprint(lcd_buffer, 1);
        break;
#if ENABLE_SENSOR_SORTING
    case 316: //13.04.21
        // Печатаем номера всех наших сенсоров
        for (int i = 0, n = 0; i < MAX_DS1820; i++) {
            n += sprintf_P(lcd_buffer + n, (i > 0) ? PSTR(" %02d") : PSTR("%02d"), (int)ds1820_nums[i] + 1);
        }
        my_lcdprint(lcd_buffer);
        //lcd.setCursor(0, 1);         //13.04.21
        dirtyTrickSetCursor(0, 1); //13.04.21
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
}

#endif // USE_CYRILLIC_DISPLAY
