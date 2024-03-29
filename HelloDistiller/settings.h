// Последнее обновление 2018-09-15 by Phisik
// Файл содержит дефолтные настройки системы
// Часть настроек меняется в файле configuration.h при выборе версий, внимательно с этим!

#define PR_REWRITE_EEPROM 9 // Константа, которая содержит признак необходимости перезаписи энергонезависимой памяти (1-254). \
    // При запуске программы, значение 0-го байта ЕЕПРОМ сравнивается с этим значением,                                                  \
    // и если они не совпадают, тогда энергонезависимая памиять переписывается текущими значениями переменных    \
    // То есть для значений переменных из скетча в контроллер, ее значение надо поменять например с 9 до 10.

//#define TEST       // Раскомметировать, если нужно получать всяческие тестовые значения через Serial1
//#define TESTMEM     // Раскомметировать, если нужно получать всяческие тестовые значения через Serial1
//#define TESTGSM     // Раскомметировать, если нужно дублировать входящюю информацияю из GSM_SERIAL (сотового телефона) в Serial1 (для отладки)//.
//#define TESTGSM1   // Раскомметировать, если нужно дублировать входящюю информацияю из GSM_SERIAL (сотового телефона) в Serial1 (для отладки)
//#define TESTRM 1    // Раскомметировать, если нужно получать тестовые значения регулятора мощности через Serial1
//#define TESTERR   // Раскомметировать, если нужно получать тестовые значения ошибок через Serial1

//#define DEBUG // Режим отладки, в этом режиме не считываются значения датчиков температуры, а они передаются через монитор порта в ардуине
// в формате: сколько прибавить секунд, Температура датчика 0, Температура датчика 1,Температура датчика 2
// например: 60,820,810,800
//  (монитор порта надо настроить так, чтобы он выдавал возврат каретки и перевод строки)

//=======================================================================================================
// Phisik: настройка UART для работы с периферией

#define DEBUG_SERIAL Serial // Куда выводить отладку
#define DEBUG_SERIAL_BAUDRATE 115200

#define GSM_SERIAL Serial2 // Порт, к которому подключена ESP, важно, чтобы не пересекалось с MQTT_SERIAL
#define GSM_SERIAL_BAUDRATE 115200 // Ставим максимальную, при которой еще нет ошибок связи
#define USE_GSM_WIFI 0 // Phisik: Отключаем wifi & gsm за ненадобностью, сэкономим часть памяти, удалив лишние переменные и повысив стабильность + уменьшим код на ~5% \
    // NB! Это оригинальная реализация WiFi, не зависящая от MQTT и работающая с проприетарным сервером

#define MQTT_SERIAL Serial3 // Порт, к которому подключена ESP, важно, чтобы не пересекалось с GSM_SERIAL
#define MQTT_SERIAL_BAUDRATE 115200 // Ставим максимальную, при которой еще нет ошибок связи
#define USE_MQTT_BROKER 1 // MQTT protocol by max506 & limon \
    // В этом режиме для ESP надо использовать прошивку от limon или мою

// #define MQTT_SERIAL_PING_CHECK   0  // Update 24.02.2020: отключил я эту штуку, т.к. не помогает. Что-нибудь придумаем потом.
// Phisik: Попытка наладить связь между контроллерами при обрыве.
//         Работает, если вообще работает, только с моей прошивкой для ESP
//         С другими прошивками будет только мешать!

//=======================================================================================================
// НАСТРОЙКА ЭКРАНА И КНОПОК
#define USE_LCD_KEYPAD_SHIELD 1 // 1 - использовать стандартный шильд с кнопками - один пин A0 на все кнопки \
    // 0 - каждая кнопка привязана к своему пину
#define USE_I2C_LCD 1 // + много свободных пинов, - много лишних проводов
#define LCD_I2C_ADDRESS 0x27 // У каждого экспандера свой адрес. Используйте сканнер I2C.

#define PRINT_ADC_VALUES 0 // Phisik 2020-03-07: Выводить raw значения кнопок для отладки

#define MENU_DELAY_SEC 60 // Phisik 2020-03-07: Через сколько секунд надо выходить из меню

#define DEBOUNCE_CYCLES 10 // Число отсчетов таймера для устранения дребезга кнопок, \
    // Phisik: было 20, и, как по мне, то кнопки тормозили

#define ENABLE_LCD_CLEAR 0 // Phisik 2020-03-07: Отключить периодические очищения дисплея, \
    // Включить обратно, если где-то будут оставаться фантомные буквы, и сообщить мне

// Поскольку у людей разные экраны, то лучше мы сделаем тут макрос
#define LCD_WIDTH 20
#define LCD_HEIGHT 4

// В международной кодировке UTF8 может быть 1-4 байта на символ,
#define LCD_BUFFER_SIZE (LCD_WIDTH * 4 + 2)

#define USE_CYRILLIC_DISPLAY 1

//=======================================================================================================
// ОБЩИЕ НАСТРОЙКИ

#define USE_WDT 1 // Поставить 1, если использовать встроенный wath dog
#define BEEP_LONG 20 // Длительность сигнала оповещения о состояниях процесса

#define NUM_PHASE 0 // Phisik: видимо, число подведеных фаз, пока стоит 0

#define ZMPT101B_MODULE_ENABLE 1 // Phisik: поддержка датчика напряжения ZMPT101B

// Phisik: Отключаем всякие надоедливые пищалки
#define NO_LOW_POWER_WARNING 1
#define NO_DETECT_ZERO_WARNING 1
#define NO_PAGE_BEEP 1 // Не пищать на 0ой странице

#define USE_MPX5010_SENSOR 1 // Phisik: раньше использовалось условие #ifdef PIN_MPX5010, \
    // из-за чего приходилось постоянно следить чтобы пин не был определен

#define USE_NPG_UROVEN_SENSORS 0 // Phisik: раньше использовалось условие #ifdef NPG_UROVEN_PIN, \
    // из-за чего приходилось постоянно следить чтобы пин не был определен

#define USE_BRESENHAM_ASC712 0 // Надо ли регулировать мощность по алгоритму Брезенхема, используя пропуск полупериодов и только датчик \
    // Если USE_BRESENHAM_ASC712 = 0, то осуществляется фазовое регулирование                                                                                                           \
    // Использование датчика тока при фазовом регулировании определяется переменной CorrectASC712

#define USE_CURRENT_TRANSFORMER 1 // Использовать трансформатор тока вместо датчика Холла \
    // Схема подключения трансформатора и объяснение тут:                                    \
    // https://learn.openenergymonitor.org/electricity-monitoring/ct-sensors/interface-with-arduino

#define USE_BMP280_SENSOR 1 // Использовать датчик давления bpm280                                                                                        \
    // Реализация выдрана из скетча 3.58i                                                                                                                  \
    // Возможно, стоит отключить ADJUST_COLUMN_STAB_TEMP ниже, если хочется работать только с датчиком давления \
    // НО можно и не отключать
#define SENSOR_IS_BME280_NOT_BMP280 0 // включить, если датчик в реальности BME280

#define BMP_SENSOR_ADDRESS 0x76

#if USE_BMP280_SENSOR == 1
#include <SPI.h>

#include <Adafruit_Sensor.h>

#if SENSOR_IS_BME280_NOT_BMP280
#include <Adafruit_BME280.h>
#else
#include <Adafruit_BMP280.h>
#endif

#endif

#if USE_CURRENT_TRANSFORMER
// Для ленивых: вот схема https://learn.openenergymonitor.org/electricity-monitoring/ct-sensors/files/Arduino_AC_current_input_A.png
// R1=R2=47k, C=10мкФ.  С надо выбирать так, чтобы sqrt(R1*C)~1

// Настройка чувствительности датчика тока на трансформаторе тока
// Этап 1 Ищем/определяем число витков нашего трансформатора
//=======================================================================================
// Для DL-CT08CL5-20A/10ma по datasheet-y заявлено 2000:1
// У вас может быть другое число витков
const int N_ac_transformer_turns = 2000; // limon // 30.11.20

// Для Simonsen123 CT-051-1.0 по datasheet-y заявлено 1000:1
// https://simonsen123.en.made-in-china.com/product/sKeESfXJZPYy/China-40A-Current-Transformer-CT-051-1-0.html
// У вас может быть другое число витков
//const int N_ac_transformer_turns = 1000;

// Этап 2 Рассчитываем номинал резистора, на который нагружен датчик тока
//=======================================================================================
// Рассчитывается так:
// 1. Определяем максимальную мощность, которую можем измерить. Например, 3200W
// ( На самом деле считаем с конца методики, трансформатор 20A/10ma Irms=20A/1.41=13.8A P=13.8*230=3174W ~ 3200W
// 3200W Это максимальная мощность для этого трансформатора DL-CT08CL5-20A/10ma)
// 2. Ищем средний RMS ток: I_rms = 3200Вт/230В ~ 13.8A
// 3. Определяет амплитуду тока: I_peak = I_rms*sqrt(2) ~ 20A
// 4. Определяем ток во вторичной обмотке: I_peak_2 = I_peak / N_ac_transformer_turns = 20A/2000 ~ 0.01A
// 5. Убеждаемся, что этот ток меньше максимально допустимого для трансформатора, для DL-CT08CL5-20A/10ma - 0.01А.
// 6. Определяем сопротивление из расчета, чтобы пиковое напряжение на нем было < 2.5В: R_burden =  2.5/I_peak_2 = 2.5/0.01= 250 Ом
// 7. Ищем ближайшее похожее в своих коробках, лучше в меньшую сторону. Я поставил пораллельно два по 510 Ом ~ 250 Ом.
const int R_burden = 250; // limon	// 30.11.20  R_burden =  2.5/0,01 ~ 250 Ом

// Этап 2 Рассчитываем номинал резистора, на который нагружен датчик тока
//=======================================================================================
// Рассчитывается так:
// 1. Определяем максимальную мощность, которую хотим измерять. Например, 3500Вт
// 2. Ищем средний RMS ток: I_rms = 3500Вт/230В ~ 15A
// 3. Определяет амплитуду тока: I_peak = I_rms*sqrt(2) ~ 21A
// 4. Определяем ток во вторичной обмотке: I_peak_2 = I_peak / N_ac_transformer_turns = 21A/1000 ~ 0.021A
// 5. Убеждаемся, что этот ток меньше максимально допустимого для трансформатора, у меня 400мА. Иначе ищем другой датчик.
// 6. Определяем сопротивление из расчета, чтобы пиковое напряжение на нем было < 2.5В: R_burden =  2.5/I_peak_2 ~ 119 Ом
// 7. Ищем ближайшее похожее в своих коробках, лучше в меньшую сторону. Я нашел 120 Ом
//const int R_burden = 120;

// Этап 3 Рассчитываем коэффициент чувствительности, т.е. количество ступеней ADC на 10A входного тока
//=======================================================================================
const int SENSITIVE_ASC712 = int(2048.0 * R_burden / N_ac_transformer_turns); // у меня получилось ~ 245 из 1023, т.е. ~ +-21А в пике

#else
#define SENSITIVE_ASC712 135 // Чувствительность датчика тока (показаний АЦП ардуино на 10 ампер тока 135 для 30А датчика, 205 для 20А датчика 82)
#endif

// Phisik @ 2021-03-25
// Этот параметр используется для сглаживания мощности, чтобы она туда сюда так не скакала
// FactPower = (1-POWER_SMOOTHING_FACTOR)*FactPower + POWER_SMOOTHING_FACTOR*FindPower;
// Должен быть строго больше 0! Не уверен - не трогай!!!
#define POWER_SMOOTHING_FACTOR 0.1 // 0 < POWER_SMOOTHING_FACTOR <= 1

#define CNT_PERIOD 4 // Количество полу-периодов для обсчета среднеквадратичного

#define USE_DIFAVTOMAT 1 // Константа, которая показывает, используется ли дифавтомат в работе системы \
    // нужно для того, чтобы при нормальном завершении процесса                                                        \
    // при плановом отключении дифавтомата не выдавалась тревога.

#define RELAY_HIGH HIGH // Какой сигнал подавать на релейные выходы мешалки и разгона
#define ALL_OFF_HIGH 1 // Какой сигнал подавать на выход ALL_OFF

#define MAX_COUNT_PROVODIMOST_SR 6 // Количество срабатываний датчика, по достижении которых можно точно сказать, что головы закончились \
    // Каждое значение - это 5 секунд, то есть в данном случае датчик должен показывать 30 секунд проводимость менее 20

// Update 2018-05-09
// Phisik: этот параметр определяет нужно ли менять температуру стабилизации колонны, если она долго не меняется
// Это связано необходимостью учитывать изменение атмосферного давления. При повышении давления, ситуацию в принципе
// спасет TimeRestabKolonna, а вот при пониженнии давления температура уплывает вниз, из-за чего "эффективная" дельта
// становится больше и хвосты можно пропустить. Если у вас есть датчик давления - оно вам скорее всего не надо.
#define ADJUST_COLUMN_STAB_TEMP 0

#if ADJUST_COLUMN_STAB_TEMP
    // Как часто будем проверять изменение температуры
const long tStabCheckPeriod = 10000; // ms
// С каким весом будем добавлять новую
const long tStabTimeConstant = 480; // 360 * 10сек ~ 1 час
// Множитель для усреднения посчитаем один раз, чтобы не тратить время в loop()
const float tStabAverageDivisor = 1 + 1.0 / tStabTimeConstant;
#endif

// Update 2018-09-15
// Phisik: сортировка датчиков температуры в меню
#define ENABLE_SENSOR_SORTING 1

// Update 2018-09-15
// Phisik: показывать температуру на 2ом экране с точкой в дробном виде
#define SHOW_FLOAT_TEMPERATURES 0

//=======================================================================================================
// НАСТРОЙКА КЛАПАНОВ

// Клапана для управления ШИМ подключены, начиная с PIN 22
// Всего максимум 5 клапанов, то есть на пины с 22 по 26 реализован
// программный ШИМ по количество полупериодов с контролем нуля.

// Phisik:  Не используйте 220В клапана и "ШИМ по количество полупериодов с контролем нуля" -
//			рано или поздно кого-нибудь прибьет!
//			Используйте 12ти вольтовые клапана и USE_12V_PWM = 1 для защиты от перегрева

#define KLP_HIGH 1 // Уровень на выходе для сработки клапана \
    // Для клапанов с низким уровнем управления поменять 0 на 1

#define PEREGREV_ON 0 // Защита от перегрева клапанов, 1- использовать, 0-нет.
#define USE_12V_PWM 1 // Phisik: Признак того, что надо использовать защиту от перегрева 12В клапанов

#if PEREGREV_ON == 0
#define PER_KLP_OPEN 1 // клапана открываем через полу-период (аналог диода), чтобы не перегревались и
#define PER_KLP_CLOSE 1 // чтобы не было гидроударов
#else
#define PER_KLP_OPEN 1000 // клапана на воду переводим в фазовое управление, чтобы раз в 10 секунд на них подавалось полное напряжение, а затем напряжение
#define PER_KLP_CLOSE 0 // из конcтанты U_PEREGREV 150
#endif

#define MAX_KLP 5 // Количество клапанов, которыми надо управлять по ШИМ.

#define KLP_NPG 0 // Номер клапана для управления НПГ при дистилляции
#define KLP_VODA 0 // Номер клапана для управления общей подачей воды в систему
#define KLP_DEFL 1 // Номер клапана для подачи воды в дефлегматор
#define KLP_DEFL_D 1 // Номер клапана для подачи воды в дефлегматор при дистилляции с дефлегматором с паровым отбором
#define KLP_HLD 1 // Номер клапана холодильника для дистилляции
#define KLP_PB 0 // Номер клапана для слива польского буфера
#define KLP_GLV_HVS 3 // Номер клапана отбора головных и хвостовых фракций
#define KLP_SR 4 // Номер клапана отбора ректификата

// Phisik: С клапаном барды был косяк. Он включался в 129 процессе и мешал тестам клапанов
// см. alarm.cpp 201 строчка if (IspReg==112 || IspReg==129)
// Т.к. барду за меня никто не сливает, то клапан отключил, внимательно здесь!
#define KLP_BARDA 0 // 4  // Номер клапана слива барды

//=======================================================================================================
// АВАРИЙНАЯ СИГНАЛИЗАЦИЯ

#define USE_ALARM_UROVEN 2 // Нужно ли использовать датчик уровня в приемной емкости ардуино \
    //  1 - останавливать процесс при налолнении емкости, в т.ч. ректификацию
//  2 - останавливать отбор, но не останавливать ректификацию (остальные процессы останавливаются)

#define USE_ALARM_VODA 1 // Нужно ли использовать датчик разлития воды ардуино

#define UROVEN_ALARM 50 //1		// Уровень сигнала, достижение котогого свидетельсвует о срабатывании аналогового датчика. \
    // В обычном состоянии он выведен на значение около 1000 для датчиков на уменьшение напряжения
// или на значение около 0-10 для датчиков на увеличение напряжения
// Для цифрового датчика используем 1

#define COUNT_ALARM 6 // Сколько должно держаться значение уровня, чтобы сработало предупреждение каждое значение - это полсекунды.
#define COUNT_ALARM_VODA 60 // Сколько должно держаться значение разлития воды, чтобы сработала тревога каждое значение - это полсекунды.
#define COUNT_ALARM_UROVEN 200 // Сколько должно держаться значение уровня, чтобы сработала тревога по уровню в приемной емкости каждое значение - это полсекунды.
#define COUNT_ALARM_UROVEN_FR 60 // Сколько должно держаться значение уровня, чтобы сработала тревога по уровню в приемной емкости при фракционной перегонке каждое значение - это полсекунды.

#ifdef DEBUG
#define COUNT_ALARM_UROVEN 20 // Сколько должно держаться значение уровня, чтобы сработала тревога по уровню в приемной емкости каждое значение - это полсекунды.
#endif

#define MAX_COUNT_NPG_ALARM 60 // Количество сработок осушения или переполнения, чтобы достоверно детектировать состояние НПГ (каждое значение -полсекунды)

// Газовый сенсор
#define USE_GAS_SENSOR 1 // Надо ли использовать датчик загазованности
#define UROVEN_GAS_SENSOR 700 // Уровень сигнала, достижение котогого свидетельсвует о срабатывании датчика спиртового пара
#define COUNT_GAS_SENSOR 6 // Сколько должно держаться значение уровня, чтобы сработала тревога каждое значение - это полсекунды.
#define TIME_PROGREV_GAS_SENSOR 60 // Время для програва датчика спиртовых паров газа.

//=======================================================================================================
// ДАТЧИКИ ТЕМПЕРАТУРЫ

#define MAX_DS1820 5

// Phisik: update 2018-07-25
// Номера датчиков теперь приведены в порядок. Если датчики определяются не в том порядке, меняем числа тут

#define TEMP_KUB 0 // Номер датчика  термометра в кубе
#define TEMP_RK20 1 // Номер датчика термометра в РК 20 см от насадки
#define TEMP_TSA 2 // Номер термометра в трубке связи с атмосферой
#define TEMP_DEFL TEMP_RK20 // Номер датчика  термометра в дефлегматоре
#define TEMP_TERMOSTAT 0 // Номер датчика  термометра термостата
#define TEMP_RAZVAR 0 // Номер датчика  термометра для разваривания зерновых

#define MAX_TEMP_TSA 650 // Максимальная температура в датчике ТСА
#define MAX_ERR_DS18 120 // После тридцати глюков подряд (а это тридцать секунд) от датчиков температуры, считаем, что процесс надо остановить.
#define MAX_ERR_MPX5010 120 // После 120 значений датчика давления подряд (а это 60 секунд) от датчиков температуры, считаем, что процесс надо остановить.

#define MAX_INDEX_INPUT 540

#define MAX_INDEX_BY_PERIOD 90
#define MIN_INDEX_BY_PERIOD 75

//=======================================================================================================
// РЕДАКТИРОВАНИЕ МЕНЮ

// Added by Phisik on 2017-08-15
// Здесь можно отключить ненужные нам пункты в меню

// NB! Пункты 129, 130 всегда должны быть в конце! Пункт 100 - вначале!
//     Это связано с обработчиком нажатий в файле keyboard.cpp, строки 600-630

#define MENU_ITEMS 21
const bool menuEnableFlag[MENU_ITEMS] = {
    1, // case 100:  Установка параметров  // Не отключается!
    1, // case 101:  Displaying   // Отключается, но отключаеть не стоит
    1, // case 102:  Термостат
    1, // case 103:  Регулятор мощности
    1, // case 104:  Первый (недробный) отбор
    0, // case 105:  Отбор голов
    0, // case 106:  Второй дробный отбор
    0, // case 107:  Третий дробный отбор
    1, // case 108:  Затор зерно
    1, // case 109:  Ректификация
    1, // case 110:  Дистилляция с дефлегматором
    0, // case 111:  НДРФ
    1, // case 112:  NBK
    1, // case 113:  Разваривание мучно-солодового затора (без варки).
    1, // case 114:  Разваривание с чиллером и миксером
    1, // case 115:  Таймер + регулятор мощности
    1, // case 116:  Пивоварня - клон браумастера
    1, // case 117:  Фракционная перегонка
    1, // case 118:  Ректификация Фракционная
    1, // case 129:  Тест клапанов // Не отключается!
    0 // case 130:  Внешнее управление
};

//=======================================================================================================
// РЕДАКТИРОВАНИЕ МЕНЮ НАСТРОЕК

// Здесь можно отключить ненужные нам пункты в меню настроек
#if ENABLE_SENSOR_SORTING
#if USE_BMP280_SENSOR
#define SETTINGS_ITEMS 71
#else
#define SETTINGS_ITEMS 70
#endif
#else
#if USE_BMP280_SENSOR
#define SETTINGS_ITEMS 70
#else
#define SETTINGS_ITEMS 69
#endif
#endif

// Edited by Phisik on 2017-08-16
// Обновил алгоритм зацикливания. Теперь не надо задавать LAST_ITEM/FIRST_ITEM
// Просто расставить  1/0

const bool settingsEnableFlagDefault[SETTINGS_ITEMS] = {
    //Общие настройки
    1, //  200: "Max_t_Tst=%5i"
    1, //  201: "Power TEN=%5u"
    1, //  202: "Power Reg=%5u"
    0, //  203: "ParamUSART=%u"
    USE_GSM_WIFI, //  204: "ParamGSM=%u"
    1, //  205: "dtTermostat=%3i"
    1, //  206: "Temp 1 Nedrobn Distill=%3i"
    0, //  207: "Temp 2 Drobn Distill=%3i"
    0, //  208: "Temp 3 Drobn Distill=%3i"
    1, //  209: "Temp Razgon Rect (+Kub,-Kol)=%3i"
    1, //  210: "Power Rectif=%3i"
    1, //  211: "Vvod Popravok ds18b20 "
    0, //  212: "Temp Okon Otbor Glv Rectif=%3i"
    1, //  213: "CHIM Otbor GLV Rectif=%5u"
    1, //  214: "%% CHIM Otbor GLV Rectif=%3i"
    1, //  215: "CHIM Otbor SR Rectif=%5u"
    1, //  216: "Delta Otbor SR Rectif=%3u"
    1, //  217: "Temp Okon Otbor SR Rectif=%3i"
    1, //  218: "Temp Okon Rectif=%3i"
    0, //  219: "Power GLV simple Distill=%4i"
    1, //  220: "Power simple Distill=%4i"
    1, //  221: "Temp Begin Dist (+Def -Kub)=%3i"
    1, //  222: "Temp Distill With Defl=%3i"
    1, //  223: "Delta Distill With Defl=%3i"
    1, //  224: "Temp Kub Okon DistWithDefl=%3i"
    1, //  225: "BeepEndProc=%1u"
    1, //  226: "BeepStateProc=%1u"
    1, //  227: "BeepKeyPress=%1u"
    1, //  228: "Power Razvar Zerno=%4i"
    1, //  229: "Power Varka Zerno=%4i"
    0, //  230: "Period Refresh Server(sec)=%3u"
    1, //  231: "U Peregrev=%3uV"
    1, //  232: "Urv Barda=%4i Barda(%4u)"
    1, //  233: "Provod SR=%4i"
    1, //  234: "Time Stab (+/-) Kolonna=%5isec"
    1, //  235: "Edit T & CHIM Count CHIM=%3i"
    1, //  236: "Auto - CHIM=%3i"
    1, //  237: "Auto + CHIM=%3i"
    1, //  238: "Time Auto + CHIM=%5isec"
    1, //  239: "Time reStab(+/-) Kolonna=%5isec"
    1, //  240: "Beer Pause Count =%3i"
    1, //  241: "Power correct ASC712 =%3i"
    USE_GSM_WIFI, //  242: "Server adr= %3u.%3u.%3u.%3u"
    USE_GSM_WIFI, //  243: "Server port= %u"
    USE_GSM_WIFI, //  244: "ID Device= %s"
    USE_GSM_WIFI, //  245: "My Phone= %s"
    1, //  246: "Alarm Pressure MPX5010=%i"
    1, //  247: "Use Avtonom HLD=%i"
    1, //  248: "Time Open BRD=%i"
    1, //  249: "PID Paramters %4i %4i %4i"
    1, //  250: "min %% CHIM  Otbor SR=%2i"
    1, //  251: "Edit Temp Stab Rectif=%3i"
    1, //  252: "Beg %% CHIM  Otbor SR=%2i"
    1, //  253: "Popr MPX=%4i %4i/%4i"
    1, //  254: "Power NBK=%4i"
    USE_GSM_WIFI, //  255: "Wi-Fi AP= %s"
    USE_GSM_WIFI, //  256: "Wi-Fi Password= %s"
    1, //  257: "Fraction Dist Count =%3i"
    1, //  258: "Fraction Rectif Count =%3i"
    1, //  259: "Temp Zasyp Zator=%3i"
    1, //  260: "Temp Osahariv Zator=%3i"
    1, //  261: "Temp Brogenia Zator=%3i"
    0, //  262: "PhasePW %5i= %4i+%4i+%4i"
    0, //  263: "Phase%% %3i= %3i + %3i + %3i"
    1, //  264: "min Pressure NBK=%3i+%3i=%3i"
    1, //  265: "delta Pressure NBK=%3i+%3i=%3i"
    1, //  266: "time Pressure NBK=%3i"
    1, //  267: "Upravl Nasos NBK=%3i"
    1 //  268: "%% otbor Tsarga Paster(+/-)=%3i"
#if ENABLE_SENSOR_SORTING
    ,
    1 //  269: Поправки к датчикам
#endif
#if USE_BMP280_SENSOR
    ,
    1 // case 270: Датчик давления
#endif
};

const bool settingsEnableFlagRect[SETTINGS_ITEMS] = {
    //Ректификация
    0, //  200: "Max_t_Tst=%5i"
    1, //  201: "Power TEN=%5u"
    0, //  202: "Power Reg=%5u"
    0, //  203: "ParamUSART=%u"
    USE_GSM_WIFI, //  204: "ParamGSM=%u"
    0, //  205: "dtTermostat=%3i"
    0, //  206: "Temp 1 Nedrobn Distill=%3i"
    0, //  207: "Temp 2 Drobn Distill=%3i"
    0, //  208: "Temp 3 Drobn Distill=%3i"
    1, //  209: "Temp Razgon Rect (+Kub,-Kol)=%3i"
    1, //  210: "Power Rectif=%3i"
    1, //  211: "Vvod Popravok ds18b20 "
    0, //  212: "Temp Okon Otbor Glv Rectif=%3i"
    1, //  213: "CHIM Otbor GLV Rectif=%5u"
    1, //  214: "%% CHIM Otbor GLV Rectif=%3i"
    1, //  215: "CHIM Otbor SR Rectif=%5u"
    1, //  216: "Delta Otbor SR Rectif=%3u"
    1, //  217: "Temp Okon Otbor SR Rectif=%3i"
    1, //  218: "Temp Okon Rectif=%3i"
    0, //  219: "Power GLV simple Distill=%4i"
    0, //  220: "Power simple Distill=%4i"
    0, //  221: "Temp Begin Dist (+Def -Kub)=%3i"
    0, //  222: "Temp Distill With Defl=%3i"
    0, //  223: "Delta Distill With Defl=%3i"
    0, //  224: "Temp Kub Okon DistWithDefl=%3i"
    1, //  225: "BeepEndProc=%1u"
    1, //  226: "BeepStateProc=%1u"
    1, //  227: "BeepKeyPress=%1u"
    0, //  228: "Power Razvar Zerno=%4i"
    0, //  229: "Power Varka Zerno=%4i"
    0, //  230: "Period Refresh Server(sec)=%3u"
    1, //  231: "U Peregrev=%3uV"
    0, //  232: "Urv Barda=%4i Barda(%4u)"
    1, //  233: "Provod SR=%4i"
    1, //  234: "Time Stab (+/-) Kolonna=%5isec"
    1, //  235: "Edit T & CHIM Count CHIM=%3i"
    1, //  236: "Auto - CHIM=%3i"
    1, //  237: "Auto + CHIM=%3i"
    1, //  238: "Time Auto + CHIM=%5isec"
    1, //  239: "Time reStab(+/-) Kolonna=%5isec"
    0, //  240: "Beer Pause Count =%3i"
    1, //  241: "Power correct ASC712 =%3i"
    USE_GSM_WIFI, //  242: "Server adr= %3u.%3u.%3u.%3u"
    USE_GSM_WIFI, //  243: "Server port= %u"
    USE_GSM_WIFI, //  244: "ID Device= %s"
    USE_GSM_WIFI, //  245: "My Phone= %s"
    1, //  246: "Alarm Pressure MPX5010=%i"
    0, //  247: "Use Avtonom HLD=%i"
    0, //  248: "Time Open BRD=%i"
    0, //  249: "PID Paramters %4i %4i %4i"
    1, //  250: "min %% CHIM  Otbor SR=%2i"
    1, //  251: "Edit Temp Stab Rectif=%3i"
    1, //  252: "Beg %% CHIM  Otbor SR=%2i"
    1, //  253: "Popr MPX=%4i %4i/%4i"
    0, //  254: "Power NBK=%4i"
    USE_GSM_WIFI, //  255: "Wi-Fi AP= %s"
    USE_GSM_WIFI, //  256: "Wi-Fi Password= %s"
    0, //  257: "Fraction Dist Count =%3i"
    1, //  258: "Fraction Rectif Count =%3i"
    0, //  259: "Temp Zasyp Zator=%3i"
    0, //  260: "Temp Osahariv Zator=%3i"
    0, //  261: "Temp Brogenia Zator=%3i"
    0, //  262: "PhasePW %5i= %4i+%4i+%4i"
    0, //  263: "Phase%% %3i= %3i + %3i + %3i"
    0, //  264: "min Pressure NBK=%3i+%3i=%3i"
    0, //  265: "delta Pressure NBK=%3i+%3i=%3i"
    0, //  266: "time Pressure NBK=%3i"
    0, //  267: "Upravl Nasos NBK=%3i"
    1 //  268: "%% otbor Tsarga Paster(+/-)=%3i"
#if ENABLE_SENSOR_SORTING
    ,
    1 //  269: Поправки к датчикам
#endif
#if USE_BMP280_SENSOR
    ,
    1 // case 270: Датчик давления
#endif
};

const bool settingsEnableFlagNBK[SETTINGS_ITEMS] = {
    //НБК
    0, //  200: "Max_t_Tst=%5i"
    1, //  201: "Power TEN=%5u"
    0, //  202: "Power Reg=%5u"
    0, //  203: "ParamUSART=%u"
    USE_GSM_WIFI, //  204: "ParamGSM=%u"
    0, //  205: "dtTermostat=%3i"
    0, //  206: "Temp 1 Nedrobn Distill=%3i"
    0, //  207: "Temp 2 Drobn Distill=%3i"
    0, //  208: "Temp 3 Drobn Distill=%3i"
    0, //  209: "Temp Razgon Rect (+Kub,-Kol)=%3i"
    0, //  210: "Power Rectif=%3i"
    1, //  211: "Vvod Popravok ds18b20 "
    0, //  212: "Temp Okon Otbor Glv Rectif=%3i"
    0, //  213: "CHIM Otbor GLV Rectif=%5u"
    0, //  214: "%% CHIM Otbor GLV Rectif=%3i"
    0, //  215: "CHIM Otbor SR Rectif=%5u"
    0, //  216: "Delta Otbor SR Rectif=%3u"
    0, //  217: "Temp Okon Otbor SR Rectif=%3i"
    0, //  218: "Temp Okon Rectif=%3i"
    0, //  219: "Power GLV simple Distill=%4i"
    0, //  220: "Power simple Distill=%4i"
    0, //  221: "Temp Begin Dist (+Def -Kub)=%3i"
    0, //  222: "Temp Distill With Defl=%3i"
    0, //  223: "Delta Distill With Defl=%3i"
    0, //  224: "Temp Kub Okon DistWithDefl=%3i"
    1, //  225: "BeepEndProc=%1u"
    1, //  226: "BeepStateProc=%1u"
    1, //  227: "BeepKeyPress=%1u"
    0, //  228: "Power Razvar Zerno=%4i"
    0, //  229: "Power Varka Zerno=%4i"
    0, //  230: "Period Refresh Server(sec)=%3u"
    1, //  231: "U Peregrev=%3uV"
    0, //  232: "Urv Barda=%4i Barda(%4u)"
    0, //  233: "Provod SR=%4i"
    0, //  234: "Time Stab (+/-) Kolonna=%5isec"
    0, //  235: "Edit T & CHIM Count CHIM=%3i"
    0, //  236: "Auto - CHIM=%3i"
    0, //  237: "Auto + CHIM=%3i"
    0, //  238: "Time Auto + CHIM=%5isec"
    0, //  239: "Time reStab(+/-) Kolonna=%5isec"
    0, //  240: "Beer Pause Count =%3i"
    1, //  241: "Power correct ASC712 =%3i"
    USE_GSM_WIFI, //  242: "Server adr= %3u.%3u.%3u.%3u"
    USE_GSM_WIFI, //  243: "Server port= %u"
    USE_GSM_WIFI, //  244: "ID Device= %s"
    USE_GSM_WIFI, //  245: "My Phone= %s"
    1, //  246: "Alarm Pressure MPX5010=%i"
    0, //  247: "Use Avtonom HLD=%i"
    0, //  248: "Time Open BRD=%i"
    0, //  249: "PID Paramters %4i %4i %4i"
    0, //  250: "min %% CHIM  Otbor SR=%2i"
    0, //  251: "Edit Temp Stab Rectif=%3i"
    0, //  252: "Beg %% CHIM  Otbor SR=%2i"
    1, //  253: "Popr MPX=%4i %4i/%4i"
    1, //  254: "Power NBK=%4i"
    USE_GSM_WIFI, //  255: "Wi-Fi AP= %s"
    USE_GSM_WIFI, //  256: "Wi-Fi Password= %s"
    0, //  257: "Fraction Dist Count =%3i"
    0, //  258: "Fraction Rectif Count =%3i"
    0, //  259: "Temp Zasyp Zator=%3i"
    0, //  260: "Temp Osahariv Zator=%3i"
    0, //  261: "Temp Brogenia Zator=%3i"
    0, //  262: "PhasePW %5i= %4i+%4i+%4i"
    0, //  263: "Phase%% %3i= %3i + %3i + %3i"
    1, //  264: "min Pressure NBK=%3i+%3i=%3i"
    1, //  265: "delta Pressure NBK=%3i+%3i=%3i"
    1, //  266: "time Pressure NBK=%3i"
    1, //  267: "Upravl Nasos NBK=%3i"
    0 //  268: "%% otbor Tsarga Paster(+/-)=%3i"
#if ENABLE_SENSOR_SORTING
    ,
    1 //  269: Поправки к датчикам
#endif
#if USE_BMP280_SENSOR
    ,
    0 // case 270: Датчик давления
#endif
};

const bool settingsEnableFlagDistill[SETTINGS_ITEMS] = {
    //Дистилляция
    0, //  200: "Max_t_Tst=%5i"
    1, //  201: "Power TEN=%5u"
    0, //  202: "Power Reg=%5u"
    0, //  203: "ParamUSART=%u"
    USE_GSM_WIFI, //  204: "ParamGSM=%u"
    0, //  205: "dtTermostat=%3i"
    1, //  206: "Temp 1 Nedrobn Distill=%3i"
    0, //  207: "Temp 2 Drobn Distill=%3i"
    0, //  208: "Temp 3 Drobn Distill=%3i"
    0, //  209: "Temp Razgon Rect (+Kub,-Kol)=%3i"
    0, //  210: "Power Rectif=%3i"
    1, //  211: "Vvod Popravok ds18b20 "
    0, //  212: "Temp Okon Otbor Glv Rectif=%3i"
    0, //  213: "CHIM Otbor GLV Rectif=%5u"
    0, //  214: "%% CHIM Otbor GLV Rectif=%3i"
    0, //  215: "CHIM Otbor SR Rectif=%5u"
    0, //  216: "Delta Otbor SR Rectif=%3u"
    0, //  217: "Temp Okon Otbor SR Rectif=%3i"
    0, //  218: "Temp Okon Rectif=%3i"
    1, //  219: "Power GLV simple Distill=%4i"
    1, //  220: "Power simple Distill=%4i"
    1, //  221: "Temp Begin Dist (+Def -Kub)=%3i"
    1, //  222: "Temp Distill With Defl=%3i"
    1, //  223: "Delta Distill With Defl=%3i"
    1, //  224: "Temp Kub Okon DistWithDefl=%3i"
    1, //  225: "BeepEndProc=%1u"
    1, //  226: "BeepStateProc=%1u"
    1, //  227: "BeepKeyPress=%1u"
    0, //  228: "Power Razvar Zerno=%4i"
    0, //  229: "Power Varka Zerno=%4i"
    0, //  230: "Period Refresh Server(sec)=%3u"
    1, //  231: "U Peregrev=%3uV"
    0, //  232: "Urv Barda=%4i Barda(%4u)"
    0, //  233: "Provod SR=%4i"
    0, //  234: "Time Stab (+/-) Kolonna=%5isec"
    0, //  235: "Edit T & CHIM Count CHIM=%3i"
    0, //  236: "Auto - CHIM=%3i"
    0, //  237: "Auto + CHIM=%3i"
    0, //  238: "Time Auto + CHIM=%5isec"
    0, //  239: "Time reStab(+/-) Kolonna=%5isec"
    0, //  240: "Beer Pause Count =%3i"
    1, //  241: "Power correct ASC712 =%3i"
    USE_GSM_WIFI, //  242: "Server adr= %3u.%3u.%3u.%3u"
    USE_GSM_WIFI, //  243: "Server port= %u"
    USE_GSM_WIFI, //  244: "ID Device= %s"
    USE_GSM_WIFI, //  245: "My Phone= %s"
    1, //  246: "Alarm Pressure MPX5010=%i"
    1, //  247: "Use Avtonom HLD=%i"
    0, //  248: "Time Open BRD=%i"
    0, //  249: "PID Paramters %4i %4i %4i"
    0, //  250: "min %% CHIM  Otbor SR=%2i"
    0, //  251: "Edit Temp Stab Rectif=%3i"
    0, //  252: "Beg %% CHIM  Otbor SR=%2i"
    1, //  253: "Popr MPX=%4i %4i/%4i"
    0, //  254: "Power NBK=%4i"
    USE_GSM_WIFI, //  255: "Wi-Fi AP= %s"
    USE_GSM_WIFI, //  256: "Wi-Fi Password= %s"
    0, //  257: "Fraction Dist Count =%3i"
    0, //  258: "Fraction Rectif Count =%3i"
    0, //  259: "Temp Zasyp Zator=%3i"
    0, //  260: "Temp Osahariv Zator=%3i"
    0, //  261: "Temp Brogenia Zator=%3i"
    0, //  262: "PhasePW %5i= %4i+%4i+%4i"
    0, //  263: "Phase%% %3i= %3i + %3i + %3i"
    0, //  264: "min Pressure NBK=%3i+%3i=%3i"
    0, //  265: "delta Pressure NBK=%3i+%3i=%3i"
    0, //  266: "time Pressure NBK=%3i"
    0, //  267: "Upravl Nasos NBK=%3i"
    0 //  268: "%% otbor Tsarga Paster(+/-)=%3i"
#if ENABLE_SENSOR_SORTING
    ,
    1 //  269: Поправки к датчикам
#endif
#if USE_BMP280_SENSOR
    ,
    1 // case 270: Датчик давления
#endif
};

// =======================================================================================================
// ОТКЛЮЧЕНИЕ ЭКРАНОВ

// Здесь можно отключить ненужные нам экраны
#define SCREEN_ITEMS 11

// Edited by Phisik on 2018-07-03
// Обновил алгоритм зацикливания. Теперь не надо задавать LAST_ITEM/FIRST_ITEM
// Просто расставить  1/0

const bool screenEnableFlag[SCREEN_ITEMS] = {
    1, // 1
    1, // 2
    1, // 3
    1, // 4
    1, // 5
    1, // 6
    0, // 7
    1, // 8
    0, // 9
    1, // 10
    0 // 11
};

// Update 2018-09-15
// Phisik: сортировка датчиков температуры по пресетам
// NB! Раньше сравнивались только первые датчики, теперь сравниваются все датчики в пресете
#define ENABLE_DS18B20_PRESET 0

#if ENABLE_DS18B20_PRESET
// Added by Phisik on 2017-08-16
// Ниже мы займемся сортировкой датчиков DS18B20, если датчики зафиксированы и никак их не поменять местами.
// Если у вас 1 комплект оборудования, то можно поменять выше TEMP_KUB/TEMP_RK20/TEMP_TSA и не включать сортировку.
// Если комплектов оборудования несколько, то придется сортировать

// Количество пресетов для датчиков, по ним потом будет сортировать те, что найдем
// Тут надо указать сколько у вас наборов
#define DS18B20_PRESET_NUM 2

// В этой переменной хранятся адреса нужных нам датчиков
// Я оставил здесь свои номера, вы ставьте свои.

// Адреса надо предварительно получить сканнером. Брать его в примерах File->Example->OneWire->ds18x20_temperature,
// здесь https://www.pjrc.com/teensy/td_libs_OneWire.html или  искать мои сообщения на HomeDistillers.
// Меняем пин OneWire на 37 - (OneWire  ds(37);), заливаем его на наш контроллер и топаем к колонне.
// Числа должны быть в шестнадцатеричной системе исчисления, т.е. дописываем 0x (ноль-икс)
// к тому, что выплюнет сканнер. Смотрим пример:

// Это получено от сканера:
//      ROM = 28 FF 30 6A A2 16 3 5A
//        Chip = DS18B20
//        Data = 1 B0 1 4B 46 7F FF C 10 9B  CRC=9B
//        Temperature = 27.00 Celsius, 80.60 Fahrenheit
//      ROM = 28 FF D5 FB A1 16 4 5B
//        Chip = DS18B20
//        Data = 1 AF 1 4B 46 7F FF C 10 35  CRC=35
//        Temperature = 26.94 Celsius, 80.49 Fahrenheit
//      ROM = 28 FF A7 34 A2 16 5 AC
//        Chip = DS18B20
//        Data = 1 B1 1 4B 46 7F FF C 10 D8  CRC=D8
//        Temperature = 27.06 Celsius, 80.71 Fahrenheit
//      No more addresses.

// Видим 3 датчика, их адреса "ROM = x x x x x x x ", и температуру каждого,
// греем те, до которых дотянемся, выясняем какой куда приварен. Далее формируем массив адресов
// датчиков, он трехмерный, стараемcя не запутаться в скобках, помним про ноль-икс!

const unsigned char dsSensorPreset[DS18B20_PRESET_NUM][MAX_DS1820][8] = {
    {
        // первый набор
        { 0x28, 0xFF, 0xA7, 0x34, 0xA2, 0x16, 0x5, 0xAC }, // Первый датчик из первого набора
        { 0x28, 0xFF, 0xD5, 0xFB, 0xA1, 0x16, 0x4, 0x5B }, // Второй датчик из первого набора
        { 0x28, 0xFF, 0x30, 0x6A, 0xA2, 0x16, 0x3, 0x00 }, // Третий датчик из первого набора
        { 0, 0, 0, 0, 0, 0, 0, 0 }, // Если каких-то датчиков нет, то ставим нули
        { 0, 0, 0, 0, 0, 0, 0, 0 } // Запятую в конце перед фигурной скобкой не надо
    },
    { // Второй набор
        { 0x28, 0xFF, 0x30, 0x6A, 0xA2, 0x16, 0x3, 0x5A }, // Первый датчик из второго набора
        { 0x28, 0xFF, 0xA7, 0x34, 0xA2, 0x16, 0x5, 0xAC }, // Дальше я думаю все понятно...
        { 0x28, 0xFF, 0xD5, 0xFB, 0xA1, 0x16, 0x4, 0x5B },
        { 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0, 0, 0 } } // Тут тоже без запятой
}; // dsSensorPreset[][][]

#endif

// IspReg mode constants
#define ISPREG_DISPLAY 101 // Мониторинг
#define ISPREG_THERMOSTAT 102 // Термостат
#define ISPREG_POWER_REGULATOR 103 // Регулятор мощности
#define ISPREG_1_DIST 104 // Первый (недробный) отбор
#define ISPREG_OTBOR_GOLOV 105 // Отбор голов
#define ISPREG_2_DIST 106 // Второй дробный отбор
#define ISPREG_3_DIST 107 // Третий дробный отбор
#define ISPREG_ZATOR 108 // Затор зерна
#define ISPREG_RECT 109 // Ректификация
#define ISPREG_DEFL_DIST 110 // Дистилляция с дефлегматором
#define ISPREG_NDRF 111 // НДРФ
#define ISPREG_NBK 112 // Непрерывная бражная колонна (НБК)
#define ISPREG_ZATOR_MUKI 113 // Мучно-солодовый затор (без варки)
#define ISPREG_RAZVAR 114 // Разваривание зерна с чиллером и миксером
#define ISPREG_TIMER 115 // Таймер + регулятор мощности
#define ISPREG_BRAUMASTER 116 // Пивоварня (клон Braumaster)
#define ISPREG_FRACTION_DIST 117 // Фракционная перегонка
#define ISPREG_FRACTION_RECT 118 // Фракционная ректификация
#define ISPREG_VALVE_TEST 129 // Тест клапанов
