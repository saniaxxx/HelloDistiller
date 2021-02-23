// Последнее обновление 2017-05-31 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Все о клапанах тут
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "configuration.h"
#include "declarations.h"

// Процедура открывает клапана, которые необходимо открыть.

void OpenKLP()
{
  for (byte i=0;i<MAX_KLP;i++) {
    // Если клапан в режиме ШИМ и Состояние клапана включен, то включаем его 
    #if SIMPLED_VERSION<30
        if (KlReg[i]==1 && KlState[i]==1) digitalWrite(PIN_KLP_BEG+i, KLP_HIGH); 
    #else
        if (KlReg[i]==1 && KlState[i]==1) digitalWrite(PIN_KLP_BEG+i+i, KLP_HIGH); 
    #endif 
  }
}

void CloseAllKLP()
{ 
  // Все клапана зануляем.
  for(byte i=0;i<MAX_KLP;i++) {
    KlOpen[i]=0;    
    KlClose[i]=10;   
    KlState[i]=0;          // Начальное состояние - клапан закрыт, счетчик 0
    KlCount[i]=0;  
    KlReg[i]=PEREGREV_ON;
  }
  
  // Если автономная система охлаждения, то у клапана дефлегматора отключаем защиту от перегрева.
  if (FlAvtonom&B00000001){
    KlReg[KLP_DEFL]=0; // Клапан дефлегматора и клапан холодильника отключим от системы защиты от перегрева если используется автономка.
    KlReg[KLP_HLD]=0;
  }
}

// процесс 129

// Процедура по-очереди открывает клапана на несколько секунд
# define KPL_OPEN_TIME 2000

void ScanKLP()
{
    // return;
  
    int i;
    #if SIMPLED_VERSION<30
      if (SIMPLED_VERSION!=5 && SIMPLED_VERSION!=20)
      {
        lcd.clear();
        lcd.print(F("KLP NPG ON"));
        for(i=0;i<KPL_OPEN_TIME;i++)
        { 
          digitalWrite(PIN_KLP_BEG+KLP_NPG,KLP_HIGH);
          delay(1);
    #if USE_WDT
          wdt_reset();
    #endif

    }

    lcd.clear();
    lcd.print(F("KLP VODA ON"));
    time3=10;
    for(i=0;i<KPL_OPEN_TIME;i++)
    { 
      digitalWrite(PIN_KLP_BEG+KLP_VODA,KLP_HIGH);
      delay(1);
    }
    lcd.clear();
    lcd.print(F("KLP HLD DIST ON"));
    for(i=0;i<KPL_OPEN_TIME;i++)
    { 
      digitalWrite(PIN_KLP_BEG+KLP_HLD,KLP_HIGH);
      delay(1);
    #if USE_WDT
          wdt_reset();
    #endif
    }
  }

  lcd.clear();
  lcd.print(F("KLP DEFL ON"));
  for(i=0;i<KPL_OPEN_TIME;i++)
  { 
    digitalWrite(PIN_KLP_BEG+KLP_DEFL,KLP_HIGH);
    delay(1);
    #if USE_WDT
        wdt_reset();
    #endif
  }

  lcd.clear();
  lcd.print(F("KLP GLV HVST ON"));
  for(i=0;i<KPL_OPEN_TIME;i++)
  { 
    digitalWrite(PIN_KLP_BEG+KLP_GLV_HVS,KLP_HIGH);
    delay(1);
    #if USE_WDT
        wdt_reset();
    #endif
  }

  lcd.clear();
  lcd.print(F("KLP SR ON"));
  for(i=0;i<KPL_OPEN_TIME;i++)
  { 
    digitalWrite(PIN_KLP_BEG+KLP_SR,KLP_HIGH);
    delay(1);
#if USE_WDT
    wdt_reset();
#endif
  }
#else
  lcd.clear();
  lcd.print(F("KLP DEFL ON"));
  for(i=0;i<KPL_OPEN_TIME;i++)
  { 
    digitalWrite(PIN_KLP_BEG+KLP_DEFL+KLP_DEFL,KLP_HIGH);
    delay(1);
#if USE_WDT
    wdt_reset();
#endif
  }

  lcd.clear();
  lcd.print(F("KLP GLV HVST ON"));
  for(i=0;i<KPL_OPEN_TIME;i++)
  { 
    digitalWrite(PIN_KLP_BEG+KLP_GLV_HVS+KLP_GLV_HVS,KLP_HIGH);
    delay(1);
#if USE_WDT
    wdt_reset();
#endif
  }

  lcd.clear();
  lcd.print(F("KLP SR ON"));
  for(i=0;i<KPL_OPEN_TIME;i++)
  { 
    digitalWrite(PIN_KLP_BEG+KLP_SR+KLP_SR,KLP_HIGH);
    delay(1);
#if USE_WDT
    wdt_reset();
#endif
  }
#endif 
}

// процесс 129
// Тут есть проблема с клапаном барды, он все всемя включается, см. settings.h
void ProcessTestKLP()
{
  switch  (StateMachine)
  {
  case 0: // Процесс не запущен
    break;
  case 1: // Начало процесса
    PrepareProcess();
    ProcChimSR=2;
    ScanKLP();
    StateMachine=2;
    if (BeepStateProcess) my_beep(BEEP_LONG);
    SecTempPrev=Seconds; // Запоминаем дату последнего изменения температуры
    if (SIMPLED_VERSION==0 || SIMPLED_VERSION>=5) ProcChimSR=50;
    else ProcChimSR=4; // Если используется простая версия, то ШИМ всегда 50%, а процент ШИМ-Это номер тестируемого в данный момент клапана.
    // Чтобы при одновременном включении реле не происходил перезапуск контроллера. 
  case 2: // Клапана открыты
    SpeedNBK=5;
    timeNBK=20;   
    UstPower=500; 
    if (SIMPLED_VERSION==0 || SIMPLED_VERSION>=5) // В обычной версии тестируем клапана одновременно, с разным ШИМ
    {
      // Клапан общей подачи воды открыт
      KlOpen[KLP_VODA]=((timeChimRectOtbSR/10)*ProcChimSR)/10;
      KlClose[KLP_VODA]=timeChimRectOtbSR-KlOpen[KLP_SR];
      // Клапан НПГ откырт
      KlOpen[KLP_NPG]=KlOpen[KLP_VODA];
      KlClose[KLP_NPG]=KlClose[KLP_VODA];
      // Клапан Холодильника откырт
      KlOpen[KLP_HLD]=KlOpen[KLP_VODA];
      KlClose[KLP_HLD]=KlClose[KLP_VODA];
      // Клапан дефлегматора откырт
      KlOpen[KLP_DEFL]=KlOpen[KLP_VODA];
      KlClose[KLP_DEFL]=KlClose[KLP_VODA];
      // Клапан отбора спирта откырт
      KlOpen[KLP_GLV_HVS]=KlOpen[KLP_VODA];
      KlClose[KLP_GLV_HVS]=KlClose[KLP_VODA];
      // Клапан отбора спирта откырт
      KlOpen[KLP_SR]=KlOpen[KLP_VODA];
      KlClose[KLP_SR]=KlClose[KLP_VODA];

    }
    else
    { // В упрощенной версии тестируем клапана по-очереди
      // ШИМ всегда 50%, в проценте ШИМ записан номер клапана.
      KlOpen[ProcChimSR]=500;
      KlClose[ProcChimSR]=500;
    }
    if (Seconds-SecTempPrev>600)  // Если с момента запуска прошло более 10 минут, то переходим в режим паузы - 1 минута.
    {
      if (BeepStateProcess) my_beep(BEEP_LONG);
      SecTempPrev=Seconds; // Запоминаем время паузы.
      StateMachine=3;
      // Закрываем все клапана.
      if (SIMPLED_VERSION==0 || SIMPLED_VERSION>=5)
      {
        ProcChimSR-=5;
        if (ProcChimSR<=0) ProcChimSR=100;
      }
      else
      { // Тестируем клапана.
        ProcChimSR++;
        if (ProcChimSR>4) ProcChimSR=2;
      }
      CloseAllKLP();
    }
    break;
  case 3: // Клапана закрыты, ждем 1 минуту
    if (Seconds-SecTempPrev>60)  // Если с момента запуска прошло более 20 минут, то переходим в режим паузы - 1 минута.
    {
      if (BeepStateProcess) my_beep(BEEP_LONG);
      StateMachine=2;
    }
    break;
  case 100:
    CloseAllKLP();
    break;
  }
}
