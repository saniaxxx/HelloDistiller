// Последнее обновление 2017-05-31 by Phisik
// Основной файл программы
// После разгрузки тут остались только вспомогательные функции и обработчики прерываний
#include "configuration.h"
#include "declarations.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Прерывание вызывается 125 раз в секунду
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER2_COMPA_vect) 
{
      boolean UprNBK;
      // Сброс таймера
      TCNT2=0x00;
    
    
      Counter1++;
      if (Counter1==62)
      {
        NeedDisplaying=true;
        flNeedReadAlarm=true;
        NeedGPRS=1;
      }
    
      // Насосом НБК все равно управляем ( в режим НБК кстати не такие высокие требования к мощности)
      if (SpeedNBK>0) 
      {  
        if (timeNBK>0)
        { 
          if (!(UprNasosNBK&B00000100)) // Насосом управляем через брезенхем 
          {
            b_value[1]=SpeedNBK;
            UprNBK=bresenham_getNext(1);
            if (!(UprNasosNBK&B00000001)) UprNBK=!UprNBK;
            digitalWrite(PIN_NASOS_NBK,UprNBK);
          }
          else
          {
            // Насосом управляем через ШИМ
            if (b_value[1]<=SpeedNBK) digitalWrite(PIN_NASOS_NBK,HIGH);
            else digitalWrite(PIN_NASOS_NBK,LOW);
            b_value[1]++;
            if (b_value[1]>b_size[1]) b_value[1]=0;
          }
    
        }
      }
      // Насосом бардоотводчика управляем всегда, если он включен.
      if (timeOpenBRD<0 && timeBRD>0)
      {
        b_value[2]=(int)SpeedNBKDst*(int)(-timeOpenBRD)/10;
        UprNBK=bresenham_getNext(2);
        if (!(UprNasosNBK&B00000010)) UprNBK=!UprNBK;
        digitalWrite(PIN_NASOS_NBK_BRD,UprNBK);
      }
      else
      {
        UprNBK=LOW;
        if (!(UprNasosNBK&B00000010)) UprNBK=!UprNBK;
        digitalWrite(PIN_NASOS_NBK_BRD,UprNBK);  
      }
    
    
      if (StateVolts==1) return;
    
      if (Counter1 % 25 ==0)
      {
        //    Sec5++;
      }
    
    
    
    #ifdef DEBUG
      if (DEBUG_SERIAL.available()) return;
    #endif
      // Сканируем клавиатуру три раза секунду или если находимся не в режиме отображения информации, то постоянно, то есть 125 раз в секунду
    
    
      if (BeepTime>1) BeepTime--;
      else
      {
        if (BeepTime==1) analogWrite(PIN_SOUND, 255);
        BeepTime=0;
      }
    
      //  
    
      // Следим за защитой от дребезга клавиш
      if (Counter1 % DEBOUNCE_CYCLES ==0)
      {          
        if (CountKeys>0 && flScanKbd==0) CountKeys--; 
        flNeedScanKbd=1; 
    
      }
    
    
      //  if (Counter1 % 32 ==0)
      //  {
      //    if (StateVolts==4) 
      //    {
      //      StateVolts=0; // Раз в 1/3 секунды запускаем измерение среднеквадратичного (в дальнейшем можно будет настроить, 
      //      // чтобы оно делалось так часто, насколько позволяют возможности контроллера).
      //      if (CorrectASC712)  flCorrASC712=!flCorrASC712; // Читаем через раз, то мощность, то напряжение если, конечно установлено что надо считать мощность.
      //      
      //    }
      //
      //  }
    
      if (Counter1>=125)
      {
    #if USE_WDT
        wdt_reset();
    #endif
    
        //    analogWrite(PIN_LIGHT,100);
        if (CountKeys>0) 
        { 
          if (secPressKeys>0) { 
            AddPressKeys=1;  
            AddPressKeys10=10;
          }
          if (secPressKeys>3) {
            AddPressKeys=5;   
            AddPressKeys10=20;
          }
          if (secPressKeys>5) {
            AddPressKeys=10;  
            AddPressKeys10=50;
          }
          if (secPressKeys>10) {
            AddPressKeys=100;
            AddPressKeys10=100;
          }
          if (secPressKeys>15) {
            AddPressKeys=200;
            AddPressKeys10=250;
          }
          if (secPressKeys<200) secPressKeys++;
        }
        else
        {
          secPressKeys=0;
          AddPressKeys=1;
          AddPressKeys10=10;
        }
        zPSOut=zPS;
        zPS=0;
        if (time1>0) time1--;
        else time1=0;
        if (time2>0) time2--;
        else time2=0;
        if (time3>0) time3--;
        else time3=0;
        if (timeBRD!=0) timeBRD--;

        
        #if USE_GSM_WIFI == 1
              if (timeGPRS!=0) timeGPRS--;
              if (timeWaitGPRS!=0) timeWaitGPRS--;
              if (timeRefrServer!=0) 
                  timeRefrServer--;
              else {
                  // Если не находимся в режиме обновления сервера, тогда установим необходимость сессии GPRS  
                  // Если GPRS используется и не переведено в режим отправки SMS и сессия GPRS не активирована и задан период опроса сервера.
                  if (FlToGSM>1 && !SMSOnCall && timeGPRS==0 && PeriodRefrServer!=0 && (flGPRSState==0 || flGPRSState==98))
                  {
                    ErrGPRSConnectInernet=0;
                    ErrGPRSConnectServer=0;
                    if (flGPRSState==0) flGPRSState=14; // Активируем сессию GPRS
                    flNeedRefrServer=1;
                    timeGPRS=60;
                  }
              }
        #endif //        #if USE_GSM_WIFI == 1
    
    
        if (timeNBK>0) 
        {  
          // Задаем скорость насосу. 
          timeNBK--;
        }
        else
        {  
          // убираем насос.
          timeNBK=0;
        }
    
        if (timeNBK==0 || SpeedNBK==0 ) 
        {  
          // убираем насос НБК.
          UprNBK=LOW;
          if (!(UprNasosNBK&B00000001)) UprNBK=!UprNBK;
          digitalWrite(PIN_NASOS_NBK,UprNBK);
        }
    
        if (timeMIXER>0)
        { 
          timeMIXER--;
          digitalWrite(PIN_MIXER,RELAY_HIGH);
          if (IspReg== 108 || IspReg== 113 || IspReg== 114 || IspReg==112 || IspReg==104) // Если идет процесс разварки зерна,  то включаем мешалку, которую возможно подключить через реле на клапан голов
          {
            KlOpen[KLP_GLV_HVS]=40;
            KlClose[KLP_GLV_HVS]=0;
          }
        }
        else
        {
          timeMIXER=0;
          digitalWrite(PIN_MIXER,!RELAY_HIGH);
          if (IspReg== 108 || IspReg== 113 || IspReg== 114 || IspReg== 112  || IspReg==104) // Если идет процесс разварки зерна, то отключаем мешалку, которую возможно подключить через реле на клапан голов
          {
            KlOpen[KLP_GLV_HVS]=0;
            KlClose[KLP_GLV_HVS]=40;
          }
        }
    
        if (CountState>0) 
        {   
          if (CountState==1 && ENABLE_LCD_CLEAR) bLCDclearFlag = true;
          CountState--;
        }
        Seconds++;
	#if USE_BMP280_SENSOR
		flReadPress++;
	#endif // USE_BMP280_SENSOR
        Counter1=0;
        NeedDisplaying=true;
        NeedGPRS=1;
        flNeedReadAlarm=true;
        flCrossZero++;

#if !defined(NO_DETECT_ZERO_WARNING) || NO_DETECT_ZERO_WARNING!=1
        if (flCrossZero>80)
        {
          // Если находимся не в режиме термостата или просмотра, и пропадание напряжения не появилось в результате нормального отключения дифавтомата,то переводим в режим тревоги.
          // Если не находимся в режиме тестирования
	
		#ifndef TEST
		#ifndef TESTRM
		#ifndef DEBUG
		#ifndef TESTGSM
		#ifndef TESTGSM1
			  if (IspReg>102 && (!USE_DIFAVTOMAT || !flAllOff))
			  {
				// Выдаем ошибку детектирования нуля.
				IspReg=249;
    
			  }
		#endif
		#endif
		#endif
		#endif
		#endif
	
          StateVolts=0;
          flCrossZero=0;
        }
#endif
    
    
        // Если контроллер используется как термостат, или это уже чтение готового результата, или при чтении информации датчиков произошла ошибка то читаем раз в секунду.
        // Или мы сейчас вводим поправки.
        if (IspReg==102 || StepOut!=0 || FlState==300 || CntErrDs18) 
        {
          flNeedTemp=1; // Устанавливаем флаг того, что надо читать температуру
        }
        else
        {
          // Иначе запускаем расчет один раз в 5 секунд (чтоб поменьше мограл регулятор мощности).
          if (Seconds % 5==0) flNeedTemp=1; 
        }
    
      }
      // Выполняем процесс.
    
      if (CountState<=0 && flScanKbd==0) 
      {
        FlState=0;
        CountState=0;
      }  
}  // ISR(timer2)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Функции для 20ой версии
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if SIMPLED_VERSION==20
    // Прерывание управления симистором.
    ISR(TIMER3_COMPA_vect) 
    {
        // Включаем симистор
        if (UstPower>0 && StateMachine<100)
        {
          // Если НПГ инициализирован, токда подаем питание на ТЭНы, чтобы их не спалить при не заполненном НПГ
          if (StateNPG>1) digitalWrite(PIN_KLP_BEG+KLP_DEFL, HIGH);  
          // ждем 20 мкс и выключаем симистор
        }
        delayMicroseconds(20);
        if (TimeOpenTriacFact!=9999)
        {
          digitalWrite(PIN_KLP_BEG+KLP_DEFL, LOW);
        }
    }
    
    // Прерывание управления симистором клапанов
    ISR(TIMER4_COMPA_vect) 
    {
        // Включаем симистор
        if (UstPower>0 && StateMachine<100)
        {
          // Если НПГ инициализирован, токда подаем питание на ТЭНы, чтобы их не спалить при не заполненном НПГ
          if (StateNPG>1) digitalWrite(PIN_KLP_BEG+KLP_GLV_HVS, HIGH);  
          // ждем 20 мкс и выключаем симистор
        }
        delayMicroseconds(20);
        if (TimeOpenTriacFact!=9999)
        {
          digitalWrite(PIN_KLP_BEG+KLP_GLV_HVS, LOW);
        }
    }
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Прерывание управления основным симистором
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER5_COMPA_vect) 
{
    #if USE_BRESENHAM_ASC712==0
      // Включаем симистор
      if (UstPower>0 && StateMachine<100)
      {
        // Если НПГ инициализирован, токда подаем питание на ТЭНы, чтобы их не спалить при не заполненном НПГ
        if (StateNPG>1) digitalWrite(PIN_TRIAC, HIGH);  
        // ждем 20 мкс и выключаем симистор
      }
      delayMicroseconds(20);
      if (TimeOpenTriacFact!=9999)
      {
        digitalWrite(PIN_TRIAC, LOW);
      }
    #else
      // Отключаем симистор только если установленная мощность меньше заданной
      if (UstPwrPH1<PowerPhase[0]) digitalWrite(PIN_TRIAC, LOW);
    #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Прерывание управления симистором клапанов
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER5_COMPB_vect) 
{
    #if USE_12V_PWM
        // Просто включаем здесь клапана по таймеру, имитируя ШИМ 100Гц
        for (byte i=0; i<MAX_KLP; i++)
            if (KlReg[i]==1 && KlState[i]==1) 
                digitalWrite(PIN_KLP_BEG+i, KLP_HIGH);   
	#elif USE_BRESENHAM_ASC712==0 && SIMPLED_VERSION!=20
          byte i;
          for (i=0;i<MAX_KLP;i++)
          {   // Если клапан в режиме ШИМ и Состояние клапана включен, то включаем его 
              #if SIMPLED_VERSION<30
                  if (KlReg[i]==1 && KlState[i]==1) digitalWrite(PIN_KLP_BEG+i, KLP_HIGH); 
              #else
                  if (KlReg[i]==1 && KlState[i]==1) digitalWrite(PIN_KLP_BEG+i+i, KLP_HIGH);    
              #endif 
          }
        
          if (TimeOpenKLP!=9999 && !flNoPhase) // Если клапана не в режиме максимальной мощности или фазовое управление отключено
          {
              delayMicroseconds(20);       
          
              for (i=0;i<MAX_KLP;i++)
              { // Если клапан в режиме ШИМ и Состояние клапана включен, то выключаем его 
                #if SIMPLED_VERSION<30
                      if (KlReg[i]==1 && KlState[i]==1) digitalWrite(PIN_KLP_BEG+i, !KLP_HIGH);  
                #else
                      if (KlReg[i]==1 && KlState[i]==1) digitalWrite(PIN_KLP_BEG+i+i, !KLP_HIGH);  
                #endif
              }
          }
          //  digitalWrite(PIN_KLP_BEG+1, HIGH);  
          //  // ждем 20 мкс и выключаем симистор
          //  delayMicroseconds(20);       
          //  digitalWrite(PIN_KLP_BEG+1, LOW); 
    #endif
     
    
    #if SIMPLED_VERSION==20
        // Включаем симистор сразу если мощность более номинальной  (TimeOpenTriacFact==9999)
        if (UstPower>0 && StateMachine<100 && TimeOpenTriacFact==9999 && StateNPG>1) digitalWrite(PIN_KLP_BEG+KLP_DEFL, HIGH);  
        TCNT3H=0x00;
        TCNT3L=0x00;
        OCR3AH=(char)(TimeOpenTriacFact>>8);
        OCR3AL=(char)TimeOpenTriacFact;
    #endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Прерывание управления симистором двигателя
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ISR(TIMER5_COMPC_vect) 
{
    // Взводим таймер открытия фазы голов
    #if SIMPLED_VERSION==20
      // Включаем симистор сразу если мощность более номинальной  (TimeOpenTriacFact==9999)
      if (UstPower>0 && StateMachine<100 && TimeOpenTriacFact==9999 && StateNPG>1) digitalWrite(PIN_KLP_BEG+KLP_GLV_HVS, HIGH);  
      TCNT4H=0x00;
      TCNT4L=0x00;
      OCR4AH=(char)(TimeOpenTriacFact>>8);
      OCR4AL=(char)TimeOpenTriacFact;
      // Отладка включаем в любом случае.
    #endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Функция вызывается по прерыванию нуля от нуля сети 220 вольт
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean br_on; // Признак включения по брезенхему
unsigned char CntPeriodPause=0; // Число полу-периодов паузы

#define KPL_FULL_POWER 65   // Число циклов в течение которых клапана полностью запитаны для надежного включения
                            // time_full_power = KPL_FULL_POWER / 125 секунд

void zero_cross_int()  // function to be fired at the zero crossing to dim the light
{
    #ifdef DEBUG
      if (DEBUG_SERIAL.available()) return;
    #endif
    
      unsigned char i;
      unsigned char flOpenClose;
      zPS++;
      flCrossZero=0;
    
      TicZero=(int) TCNT5L | (int) (TCNT5H<<8); // Расчитывааем текущее значение таймера
      // Сбрасываем 5-й таймер.
      TCNT5H=0x00;
      TCNT5L=0x00;
    
      if (StateVolts==1 ) {
          // Если за полу-период не выбрано допустимое количество значений, тогда пропускаем эти выборки и пытаемся выбрать следующий полу-период.
          // также пропускаем выборку если управление по брезенхему и предыдущий период симистор был отключен
      
          if (index_input-prev_index_input>MAX_INDEX_BY_PERIOD || index_input-prev_index_input<MIN_INDEX_BY_PERIOD || !br_on) 
          {
            index_input=prev_index_input;
            SqNaprT=SqNaprPrevT;
          }
          else 
            CntPeriod++; // Если число в выборке допустимо, то прибавляем счетчик полу-периодов выборки
      
          // Если считали достаточное количество полу-периодов, тогда расчет закончен.
          if (CntPeriod>=CNT_PERIOD)
          {
            if (flCorrASC712==0) indexOut=index_input;
            StateVolts=2;// Если был расчет тока (напряжения), то ставим признак его окончания
            CntPeriodPause=0; 
          }
      } 
      else
          CntPeriodPause++;
    
      if (StateVolts==4) {
        // Если пропустили два по необходимому числу полу-периодов, запускаем расчет.
        // Еще одно условие, чтобы мы не находились в режиме ввода клавиш, тогда расчет не запускаем, чтобы не тормозить отчаянно 
        // при вводе информации
        if (CntPeriodPause > CNT_PERIOD && CountKeys==0) { 
          StateVolts=0;
          if (CorrectASC712)  
              flCorrASC712=!flCorrASC712; // Читаем через раз, то мощность, то напряжение если, конечно установлено что надо считать мощность.
        }
      }
    
      if (StateVolts==0) {
          index_input=0;
          StateVolts=1; 
          //     MaxVolts=0;
          #if SIMPLED_VERSION!=20
              TCNT4H=0x00;
              TCNT4L=0x00;
          #endif
          CntPeriod=0;
          SqNaprT=0;
      }
    
      prev_index_input=index_input;
      SqNaprPrevT=SqNaprT;
      br_on=true;    
    
      #if USE_BRESENHAM_ASC712==0
          if (UstPwrPH1>=PowerPhase[0]) {
            // Если мощность больше или равна номинальной, включаем триак всегда, при этом прерывание на закрытие никогда не выполнится.  
            TimeOpenTriac=9999;
            TimeOpenTriacFact=9999;
            // Если НПГ инициализирован, токда подаем питание на ТЭНы, чтобы их не спалить при не заполненном НПГ
            if (StateNPG>1) 
                digitalWrite(PIN_TRIAC, HIGH);  
          } else 
              TimeOpenTriacFact = TimeOpenTriac;
        
          // Если мощность не установлена отключена, или состояние машины конечное, то на всякий случай отключаем
          if (UstPower<=0 || StateMachine>=100) 
              digitalWrite(PIN_TRIAC, LOW);
        
          if (TimeOpenTriac<=10) {
              // Если НПГ инициализирован, токда подаем питание на ТЭНы, чтобы их не спалить при не заполненном НПГ
              if (StateNPG>1 && UstPower>0) digitalWrite(PIN_TRIAC, HIGH);  
              TimeOpenTriacFact=9999;
          }
        
          //    TimeOpenTriacFact=TimeOpenTriac;
          //TimeOpenTriac=2000;
          // В прерывании нуля настраиваем, чтобы прерывание на 5-м таймере выдалось через то время, которое мы ранее рассчитали.
          OCR5AH=(char)(TimeOpenTriacFact>>8);
          OCR5AL=(char) TimeOpenTriacFact;
        
#if USE_12V_PWM != 1
          // Настраиваем таймер клапанов
          if (TimeOpenKLP==0) TimeOpenKLP=9999;
          OCR5BH=(char)(TimeOpenKLP>>8);
          OCR5BL=(char)TimeOpenKLP;
#endif
        
          // Если считаем ток(напрряжение), тогда 
          #if SIMPLED_VERSION==20
            if (UstPower<=0 || StateMachine>=100) 
            {
              digitalWrite(PIN_TRIAC, LOW);
              digitalWrite(KLP_DEFL, LOW);
              digitalWrite(KLP_GLV_HVS, LOW);
            }
            // Взводим таймеры запуска таймеров дефлегматора и голов.
            OCR5BH=(char)(timeChimRectOtbSR>>8);
            OCR5BL=(char)timeChimRectOtbSR;
          
            OCR5CH=(char)(timeChimRectOtbGlv>>8);
            OCR5CL=(char)timeChimRectOtbGlv;
          #endif
    
      #else  //  #if USE_BRESENHAM_ASC712==0
          // Устанавливаем закрытие симистора по прерыванию таймера через некоторое время (отступить от таблицы мощности на 2 шага вполе достаточно).
          TimeOpenTriacFact=tableS10[MAX_TABLE_T-2];
          TimeOpenTriacFact*=10;
          TimeOpenTriac=TimeOpenTriacFact;
          OCR5AH=(char)(TimeOpenTriacFact>>8);
          OCR5AL=(char)TimeOpenTriacFact;
          
          // Если процент брезенхема не рассчитан, а мощность установлена, то запускаем измерительный период
          if (UstPower>0 && b_value[0]==0 || UstPwrPH1>=PowerPhase[0]) 
              digitalWrite(PIN_TRIAC,HIGH);
          else if (UstPower<=0) 
              digitalWrite(PIN_TRIAC,LOW);    // Если мощность не установлена, то на реле подаюм нулевой сигнал
          else {
              br_on=bresenham_getNext(0);
              digitalWrite(PIN_TRIAC,br_on);  // Иначе стандартно вызываем алгоритм брезехема.
          }
      #endif //  #if USE_BRESENHAM_ASC712==0
    
    
      // фазовое управление давигателем пока не используем 
      //    TimeOpenDvigatel=5000; 
      //    if (SpeedNBK>0)
      //    {
      //      TimeOpenDvigatel=TicZero/100;
      //      TimeOpenDvigatel=TimeOpenDvigatel*SpeedNBK;
      //      TimeOpenDvigatel=TicZero-TimeOpenDvigatel;
      //      if (SpeedNBK>=100) digitalWrite(PIN_DVIGATEL, HIGH);  
      //    }
      //
      //    OCR5CH=(char)(TimeOpenDvigatel>>8);
      //    OCR5CL=(char)TimeOpenDvigatel;
    
      // В прерывании нуля управляем открытием клапанов (а их лучше открывать по 0, чтобы не было помех)
      // Синхронизацию пока отключим
      //    if (!flSyncKLP) // Если клапана не находятся в режиме синхронизации
      {
          for (i=0;i<MAX_KLP;i++) {
            // Разбираемся с состоянием клапана
            if (KlState[i]==0) {
                flOpenClose=0;
                // Состояние клапана - выключен            
                if (KlCount[i]>KlClose[i]) { // Если количество тиков превышает установленное для состояния "закрыто", переводим клапан в состояние "открыто"
                  // Клапан переводим в состояние-включен
                  KlState[i]=1;
                  KlCount[i]=0;
                  if(KlOpen[i]) flOpenClose=1;
                } 
            } else { 
                flOpenClose=1;
                // Состояние клапана - выключен
                if (KlCount[i]>KlOpen[i]) { // Если количество тиков превышает установленное для состояния "открыто", переводим клапан в состояние "открыто"
                    KlState[i]=0;
                    KlCount[i]=0;
                    if(KlClose[i]) flOpenClose=0;
                } 
            }
      
            // Если клапан в принципе не должен выключаться то всегда переводим в состояние влкючен
            if (KlClose[i]==0) { 
                flOpenClose=1;
                KlState[i]=1; 
            }
            // Если клапан в принципе не должен включатся, то всегда переводим  в состояние "выключено"
            if (KlOpen[i]==0) {
                flOpenClose=0;
                KlState[i]=0; 
                KlCount[i]=0;
            }
      
            // С состоянием клапана разобрались, теперь в зависимости от состояния включаем или выключаем его
      
            if (!flOpenClose){
                // Если клапан-выключен, то выключаем его в любом случае
                #if SIMPLED_VERSION<30
                        digitalWrite(PIN_KLP_BEG+i, !KLP_HIGH);  
                #else
                        digitalWrite(PIN_KLP_BEG+i+i, !KLP_HIGH);  
                #endif
            } else {
              // Если состояние клапана - включен, включаем его только в том случае, если клапан в находится в режиме ШИМ
              // Или клапан находится в режиме фазового управления, но включен менее, чем полсекунды назад, или время фазового управления еще не рассчитано, то включаем его тоже,
              // иначе для режима фазового управления клапан включится по принципу фазового управления в прерывании. Или временно фазовое управления убрано
              if (KlReg[i]==0 || KlCount[i]<KPL_FULL_POWER || TimeOpenKLP==9999 || flNoPhase)
                  #if SIMPLED_VERSION<30
                      digitalWrite(PIN_KLP_BEG+i, KLP_HIGH);
                  #else
                      digitalWrite(PIN_KLP_BEG+i+i, KLP_HIGH);
                  #endif
            }
      
            KlCount[i]++;
          } // for (i=0;i<MAX_KLP;i++)    
      } // if (!flSyncKLP) 
      
     #if USE_12V_PWM
          // К этому моменту все стандартные процедуры завершились и клапана включены на постоянку или ждут фазового управления
          // При фазовом управлении кланана надо сначала обесточить, т.к. полевик не симистор - сам он не закроется

          boolean bNeed2FireTimer5 = false;
          for (byte i=0; i<MAX_KLP; i++)
              if( KlReg[i]==1 && KlState[i]==1 && KlCount[i]>=KPL_FULL_POWER ) {
                  bNeed2FireTimer5 = true;
                  digitalWrite(PIN_KLP_BEG+i, !KLP_HIGH);   
              }
        
          if (bNeed2FireTimer5){
              // таймер работает в диапазоне 0-2500
              TimeOpenKLP = int(8.33333*(300-NaprPeregrev));
             
              if(TimeOpenKLP<0) 
                  TimeOpenKLP =0;
              if(TimeOpenKLP>2500) 
                 TimeOpenKLP =2500;
                 
              OCR5BH = (char)(TimeOpenKLP>>8);
              OCR5BL = (char) TimeOpenKLP;
          }
     #endif

}  // zero_сross_int()


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Алгоритм Брезенхема
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char b_size[3];  // Размер брезинхема
unsigned char b_value[3]; // Текущее значение алгоритма
int           b_error[3]; // Переменная
unsigned char b_stepNumber[3]; // Номер шага

void bresenham_Init(char n, unsigned char sz)
{
    b_size[n] = sz;
    b_error[n] = b_size[n]/2;
    b_stepNumber[n] = 0;
    b_value[n] = 0;
}

boolean bresenham_getNext(char n) 
{
    boolean result;
    b_error[n] -= b_value[n];
    if ( b_error[n] < 0 ) 
    {
      b_error[n] += b_size[n];
      result = HIGH;
    }
    else 
    {
      result = LOW;
    }
  
    if ( ++b_stepNumber[n] >= b_size[n]) {
      b_stepNumber[n] = 0;
      b_error[n] = b_size[n]/2;
    }
    return result;
}		

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Функция расчета квадратного корня
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Значения корней от 0 до 90 и от 256 до 512 считаются по очень приблизительным формулам, поскольку такое значение среднеквадратичного в нормальной сети 220 вольт недопустимо
// Больше 512 значение не определено.
float my_sqrt(unsigned long tsQT)
{  
	return sqrt(tsQT);


	//// no more intere here
 // char i;
 // unsigned int sQT; // Временная переменная, чтобы поменьше сравнивать 4-байтных чисел.
 // int res=666;
 // char fl2=0;
 // if (tsQT<64) 
 // {
 //   tsQT=tsQT<<10;
 //   fl2=5;
 // }
 // else
 //   if (tsQT<256) 
 //   {
 //     tsQT=tsQT<<8;
 //     fl2=4;
 //   }
 //   else
 //     if (tsQT<1024) 
 //     {
 //       tsQT=(tsQT<<6);
 //       fl2=3;
 //     }
 //     else
 //       if (tsQT<4096) 
 //       {
 //         tsQT=(tsQT<<4);
 //         fl2=2;
 //       }
 //       else
 //         if (tsQT<tableSQ[0])
 //         {
 //           tsQT=tsQT<<2;
 //           fl2=1;
 //         }

 // // Если попадаем в диапазон
 // if (tsQT<tableSQ[0]) 
 // {
 //   // Если меньше минимального значения, тогда расчет по крайне приближенной формуле
 //   res=tsQT*tableSQRT[0]/tableSQ[0];
 // }
 // else
 //   if (tsQT<=tableSQ[ MAX_TABLE_SQRT-1])
 //   {
 //     sQT=tsQT;
 //     for(i=1;i<MAX_TABLE_SQRT;i++)
 //     {

 //       if (tableSQ[i-1]<=sQT && tableSQ[i]>=sQT)
 //       {
 //         if (tableSQ[i-1]==sQT) res=tableSQRT[i-1]; // На крайних значениях берем готовые цифры
 //         else
 //           if (tableSQ[i]==sQT) res=tableSQRT[i]; // На крайних значениях берем готовые цифры
 //           else res=(sQT-tableSQ[i-1])*(tableSQRT[i]-tableSQRT[i-1])/(tableSQ[i]-tableSQ[i-1]) + tableSQRT[i-1] +1; // На промежуточных вычисляем.
 //         break;
 //       }
 //     }
 //   }
 //   else
 //   {
 //     // Если больше максимального считаем по крайне приближенной формуле, исходя из максимума значений в 512
 //     res=(tsQT-tableSQ[MAX_TABLE_SQRT-1])*(512-tableSQRT[MAX_TABLE_SQRT-1])/(262144-tableSQ[MAX_TABLE_SQRT-1]) + tableSQRT[MAX_TABLE_SQRT-1] +1;
 //   }

 // // Если перед корнем умножали на 4, результат уменьшим на 2
 // return  res >> fl2;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Разные вспомогательные функции
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Функция устанавливает пищалку на заданное время (в 1/125 секунды), то есть 125-это одна секунда.
void my_beep(unsigned int mBeepTime)
{
  analogWrite(PIN_SOUND, 10);  
  BeepTime=mBeepTime;
}

// Процедура осуществляет ПИД регулирование температуры.
// В качестве результата возвращается коэффициент мощности, умноженный на 1000, то есть
// 1000 - это 100% подача мощности на ТЭНы.
void ProcessPIDTemp(int NeedTemp, int FactTemp)
{   
      NewErrTemp=(NeedTemp-FactTemp);
      DtTemp=NewErrTemp-OldErrTemp;
      ItTemp=ItTemp+NewErrTemp;
    
      if (ItTemp<-1000) ItTemp=-1000;
      else 
        if (ItTemp>1000) ItTemp=1000;
    
    
      OldErrTempOut=OldErrTemp; // Запоминаем старую ошибку.
      OldErrTemp=NewErrTemp; // Запоминаем старую ошибку.
      // Считаем коэффициент температуры
      KtT=((long)PIDTemp[0]*NewErrTemp*8 + (long)PIDTemp[1]*ItTemp/10+(long)PIDTemp[2]*DtTemp*10)/10;
    #ifdef TEST
      DEBUG_SERIAL.print(F("Ktt:"));
      DEBUG_SERIAL.println(KtT);
    #endif
      if (KtT>1000) KtT=1000;
      if (KtT<0) KtT=0;
      // Если значение температуры превысило нужную, то сбрасываем интегральный коэффицциент
      // и убираем мощность.
      if (FactTemp>=NeedTemp) {
        ItTemp=0 ;
        KtT=0;
      }

}

void ProcessPIDPress(int NeedTemp, int FactTemp)
{   
    NewErrTemp=(NeedTemp-FactTemp);
    DtTemp=NewErrTemp-OldErrTemp;
    ItTemp=ItTemp+NewErrTemp;
  
    if (ItTemp<-1000) ItTemp=-1000;
    else 
      if (ItTemp>1000) ItTemp=1000;
  
  
    OldErrTempOut=OldErrTemp; // Запоминаем старую ошибку.
    OldErrTemp=NewErrTemp; // Запоминаем старую ошибку.
    // Считаем коэффициент температуры
    KtT=((long)PIDTemp[0]*NewErrTemp*8 + (long)PIDTemp[1]*ItTemp/10+(long)PIDTemp[2]*DtTemp*10)/10;
  #ifdef TEST
    DEBUG_SERIAL.print(F("Ktt:"));
    DEBUG_SERIAL.println(KtT);
  #endif
    if (KtT>1000) KtT=1000;
    if (KtT<0) KtT=0;
    // Если значение температуры превысило нужную, то сбрасываем интегральный коэффицциент
    // и убираем мощность.

}



//#define MSerial1 GSM_SERIAL




#ifdef DEBUG
  void my_debug()
  {
    char s_rx;
    char *st1;
    char *stb;
    int val1;
    static char NoScan=0;
    if (!NoScan)
    {
      while (DEBUG_SERIAL.available() > 0) 
      {  //если есть доступные данные
        // считываем байт
        s_rx=DEBUG_SERIAL.read();
        delay(10);
        if (s_rx==10)
        {
          my_rx_buffer[pos_rx_buffer-1]=0;
          NoScan=1;
          pos_rx_buffer=0;
          // передаем данные в следующем формате:
          // сколькоприбавитьсекунд,Темпереатура1,Температура2, Температура3
          DEBUG_SERIAL.print(F("dbl:"));
          DEBUG_SERIAL.println(my_rx_buffer);
          stb=my_rx_buffer;
          st1=strchr(stb,',');
          if (st1)
          {
            st1[0]=0;
            //      DEBUG_SERIAL.println(stb);
            val1=atoi(stb);
            Seconds=Seconds+val1;
            timeNBK-=val1;   // Время работы насоса НБК в секундах со скоростью SpeedNKB. По истечении этого времени насос остановится
            timeMIXER-=val1; // Время работы двигателя миксера в секундах. По истечении этого времени насос остановится
            time1-=val1;   // Таймер для отсчета секунд 1
            time2-=val1;   // Таймер для отсчета секунд 2
            time3-=val1;   // Таймер для отсчета секунд 3
  
            stb=st1+1;
          }
          st1=strchr(stb,',');
          if (st1)
          {
            st1[0]=0;
            //      DEBUG_SERIAL.println(stb);
            val1=atoi(stb);
            DS_TEMP(TEMP_KUB)=val1;
            stb=st1+1;
          }
          st1=strchr(stb,',');
          if (st1)
          {
            st1[0]=0;
            //       DEBUG_SERIAL.println(stb);
            val1=atoi(stb);
            DS_TEMP(TEMP_DEFL)=val1;
            stb=st1+1;
          }
          //      DEBUG_SERIAL.println(stb);
          val1=atoi(stb);
          DS_TEMP(TEMP_TSA)=val1;
          flNeedAnalyse=1;
          NoScan=0;
        }
        else
        {
          if (pos_rx_buffer>=MY_RX_BUFFER_SIZE) pos_rx_buffer=0;
          my_rx_buffer[pos_rx_buffer]=s_rx;
          pos_rx_buffer++;
        }
      }
    }
    // Окончание анализа отладки
  }
#endif

//static unsigned int SecondsRing;


// Установка угла на серве с помощью delay
void SetAngle(unsigned char Angl)
{
    int TAngl;
    unsigned char i;
    TAngl= map(Angl, 0, 180, 600, 2400);
    noInterrupts();
    for(i=0;i<50;i++)
    { 
      digitalWrite(PIN_NASOS_NBK,HIGH);
      delayMicroseconds(TAngl);
      digitalWrite(PIN_NASOS_NBK,LOW);
      delayMicroseconds(10000-TAngl);
      delayMicroseconds(10000);
    }
    interrupts();

}




// Функция возвращает значение ШИМ для отборв по "шпоре" (температуре в кубе)
char GetCHIMOtbor()
{
    // Процент отбора задается в табличном виде, например в данном случае 
    // от 0 до 88 в кубе процент отбора не меняем, от 88.0 до 96.0 в кубе процент отбора ставить в промежутке от 68 до 25
    // от 960 до 1000 процент отбора понижаем до нуля.
    // здесь храним количество значений в таблице;
    char FindCHIM;
    char i;
  #ifdef TEST
    DEBUG_SERIAL.print(F("KUB="));
    DEBUG_SERIAL.print(DS_TEMP(TEMP_KUB));
  #endif
  
    FindCHIM=ProcChimSR;
    for(i=1;i<CntCHIM;i++)
    {
      if (tempK[i-1]<=DS_TEMP(TEMP_KUB) && tempK[i]>DS_TEMP(TEMP_KUB))
      {
        // Если отбор при этом диапазоне температур производим
        if (CHIM[i-1]>0) 
        {
          FindCHIM=(DS_TEMP(TEMP_KUB)-tempK[i-1])*(int) (CHIM[i]-CHIM[i-1])/(tempK[i]-tempK[i-1])+(int) CHIM[i-1];
  #ifdef TEST
          DEBUG_SERIAL.print(F(" CHIM="));
          DEBUG_SERIAL.println((int)FindCHIM);
  #endif
          return FindCHIM; // Теперь возвращаем новое значение ШИМ в любом случае. 
        }
  
      }
    }
  
  #ifdef TEST
    DEBUG_SERIAL.println(F(" noFindCHIM"));
  #endif
  
    return ProcChimSR; 
}




void ScanDS18b20Slave()
{
  char maxDs;
  maxDs=0;
  // Первый этап-читаем датчики три раза.
  while (nPopr<3)
  {
    nPopr++;
    delay(1500);
    ds1820_devices=0;
    while(ds.search(ds1820_rom_codes[ds1820_devices])) {
      ds1820_devices++;
      if (ds1820_devices>=MAX_DS1820) break;
    }
    ds.reset_search();
    if (ds1820_devices>maxDs) maxDs=ds1820_devices;
    if (ds1820_devices>=3) return; 
  }
  // Если за время чтения количество датчиков больше 1,то читаем датчики еще 7 раз
  if (maxDs>0)
  {
    while (nPopr<10)
    {
      nPopr++;
      delay(1500);
      ds1820_devices=0;
      while(ds.search(ds1820_rom_codes[ds1820_devices])) {
        ds1820_devices++;
        if (ds1820_devices>=MAX_DS1820) break;
      }
      ds.reset_search();
      if (ds1820_devices>maxDs) maxDs=ds1820_devices;
      if (ds1820_devices>=3) return; 
    }    
  }

}

void RaspredPowerByPhase()
{
  #if NUM_PHASE==0
    if (PowerPhase[0]<=0) PowerPhase[0]=Power;
    // Корректировка мощности если установлено ограничение мощности по фазе
    if (UstPower>Power) UstPower=Power;
    UstPwrPH1=UstPower;  
  #endif
}

#ifdef TESTMEM 
  int freeRam () 
  {
    extern int __heap_start, *__brkval; 
    int v; 
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
  }
#endif





