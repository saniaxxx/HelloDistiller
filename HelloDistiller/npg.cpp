// Последнее обновление 2017-05-31 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Парогенератор
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "configuration.h"
#include "declarations.h"

void ReadStateDtNPG()
{
#if USE_NPG_UROVEN_SENSORS
    U_NPG = analogRead(NPG_UROVEN_PIN);
    // Уровень НПГ подключен по следующей схеме: Подтяджка - 10 ком, защита -330 ом, замыкание - 30, резистор 1.1К (верхниц уровень) - 128, 2.2К (рабочий уровень) - 256, 3,3К (нижний уровень)-266
    if (U_NPG >= 80)
        NpgDt = 1; // Нижний уровень
    if (U_NPG >= 180)
        NpgDt = 2; // Рабочий уровень
    if (U_NPG >= 240)
        NpgDt = 3; // Верхний уровень
    if (U_NPG >= 500)
        NpgDt = 4; // Нет сработки

    // Если находимся в режиме аварии, состояние НПГ всегда нет сработки
    if (IspReg > 240)
        StateNPG = 2;
#endif

#ifdef NPG_UROVEN_PIN_MIN
    NpgDt = 4; // Нет сработки
    // Датчик уровня в следущем виде 10 (первая цифра сработка минимума, 2 цифра сработка нормального уровня)
    U_NPG = analogRead(NPG_UROVEN_PIN_MIN);
    if (UrovenBarda > 0) {
        if (U_NPG <= UrovenBarda)
            NpgDt = 1; // Нижний уровень
        else {
            U_NPG = analogRead(NPG_UROVEN_PIN_WORK);
            if (U_NPG <= UrovenBarda)
                NpgDt = 2;
        }
    } else {
        if (U_NPG >= -UrovenBarda)
            NpgDt = 1; // Нижний уровень
        else {
            NpgDt = 2; // Нет сработки
        }
    }
    if (IspReg > 240)
        StateNPG = 2;
#endif
}

void ProcessNPG()
{

#if !USE_NPG_UROVEN_SENSORS
#ifndef NPG_UROVEN_PIN_MIN
    StateNPG = 2;
    return;
#endif
#endif

    ReadStateDtNPG();

    switch (StateNPG) {
    case 0: // НПГ не запущен
        if (NpgDt == 1) // Если сработка нижнего уровня, то тогда НПГ надо наполнить.
        {
            StateNPG = 1;
            break;
        }
        StateNPG = 2;
        break;
    case 1: // Инициализация НПГ - наполнение
        KlOpen[KLP_NPG] = PER_KLP_OPEN;
        KlClose[KLP_NPG] = PER_KLP_CLOSE;
        if (NpgDt == 2 || NpgDt == 3) // Дошли до рабочего уровня или до переполнения, тогда переходим в рабочий режим.
        {
#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("NAPOLN NPG=%i NpgDt=%i"), U_NPG, NpgDt);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif

            // Повторное чтение для подтверждения датчика
            ReadStateDtNPG();
            if (NpgDt == 2 || NpgDt == 3)
                StateNPG = 4; // Переводим в режим однократной сработки, если прошло подтверждение
        }
        break;
    case 2: // НПГ работает в штатном режиме.
        if (NpgDt == 1 || NpgDt == 3) {
#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("Alarm NPG=%i NpgDt=%i"), U_NPG, NpgDt);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif
            // Повторное чтение для подтверждения датчика
            ReadStateDtNPG();
            if (NpgDt == 1 || NpgDt == 3)
                StateNPG = 3; // Переводим в режим однократной сработки аварии, если прошло подтверждение
            countAlrmNPG = 1;
        }

        if (NpgDt == 2) {
            // Сработка рабочего уровня НПГ
            KlOpen[KLP_NPG] = PER_KLP_OPEN;
            KlClose[KLP_NPG] = PER_KLP_CLOSE;
            countAlrmNPG = 0;
        }

        if (NpgDt == 4) // Нет никакой сработки - отключаем подачу воды
        {
            // Сработка рабочего уровня НПГ
            KlOpen[KLP_NPG] = 0;
            KlClose[KLP_NPG] = 10;
            countAlrmNPG = 0;
        }
        break;
    case 3: // Была однократная сработка датчиков осушения или переполнения

        if (NpgDt == 1) {
            countAlrmNPG++;
            ReadStateDtNPG();
#ifndef TEST
#ifndef DEBUG
            if (countAlrmNPG > MAX_COUNT_NPG_ALARM) {
                IspReg = 253; // Переводим контроллер в режим тревоги по осушению НПГ
            }
#endif
#endif

#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("Alarm pNPG=%i NpgDt=%i"), U_NPG, NpgDt);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif
            break;
        }
        if (NpgDt == 3) {
            countAlrmNPG++;
            ReadStateDtNPG();
            // Сработка верхнего уровня НПГ, исклчение - переполнение НПГ.
#ifndef TEST
#ifndef DEBUG
            IspReg = 254; // Переводим контроллер в режим тревоги по осушению НПГ
#endif
#endif

#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("Alarm pNPG=%i NpgDt=%i"), U_NPG, NpgDt);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif
            break;
        }
        StateNPG = 2; // Если тревога не подтвердилась, переходим в обычный ражим.
        countAlrmNPG = 0;
        break;

    case 4: // При наполнении НПГ была однократная сработка датчиков уровня или переполнения

        // Если подтверждаестся повторная сработка датчика, то токда считаем НПГ переведенным в рабочее состояние
        if (NpgDt == 2 || NpgDt == 3) // Дошли до рабочего уровня или до переполнения, тогда переходим в рабочий режим.
        {
#ifdef TEST
            sprintf_P(my_tx_buffer, PSTR("NAPOLN pNPG=%i NpgDt=%i"), U_NPG, NpgDt);
            DEBUG_SERIAL.println(my_tx_buffer);
#endif
            StateNPG = 2; // Переводим НПГ в рабочее состояние.
        }
        break;
    }
}
