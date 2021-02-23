// Последнее обновление 2017-05-31 by Phisik
// В рамках разгрузки кода раскидываем функции по файлам
// Тут будет связь с внешним миром

#include "configuration.h"
#include "declarations.h"


#if USE_GSM_WIFI == 1
  void SendWiFi8266()
  {
    switch(flGPRSState)
    {
    case 0:  // Состояние не запущено
      break;
    case 1: // Активируем отправку SMS
  
    case 2: // С состояний, соответсвующих отправке GPRS активируем wi-fi соединение
    case 14: 
      flGPRSState=158;
      break;
    case 40: // Состояние 40 - это дозвон до телефона, но у wi-fi телефон не подключен, поэтому просто передаем информацию 
      flGPRSState=158;
      flNeedCall=3; // ставим признак что дозвонились
    case 98: // Инициализация GSM
      // Включаем уведомления о SMS
      timeWaitGPRS=10;
      GSM_SERIAL.println(F("AT"));
      flGPRSState=99;    
    case 99:
      if (timeWaitGPRS==0) flGPRSState=98;
      break;
    case 150:
      timeWaitGPRS=10;
      GSM_SERIAL.println(F("AT+CWMODE=1"));
      flGPRSState=151;    
    case 151:
      if (timeWaitGPRS==0) flGPRSState=150;
      break;
    case 152:
      // Отключаемся от сети
      timeWaitGPRS=10;
      GSM_SERIAL.println(F("AT+CWQAP"));
      flGPRSState=153;
    case 153:
      if (timeWaitGPRS==0) flGPRSState=152;
      break;
    case 154:
      // Проверка подключения
      timeWaitGPRS=10;
      GSM_SERIAL.println(F("AT+CIFSR"));
      flGPRSState=155;
    case 155:
      if (timeWaitGPRS==0) flGPRSState=154;
      break;
    case 168: // Промежуточное состояние - к серверу подключены, идет ожидание OK
      if (timeWaitGPRS==0) flGPRSState=154;
      break;
  
    case 156:
      timeWaitGPRS=10;
      // Подключение
      sprintf_P(my_gprs_buffer,PSTR("AT+CWJAP=\"%s\",\"%s\""),WiFiAP,WiFiPass);
      GSM_SERIAL.println(my_gprs_buffer); 
      flGPRSState=157;
    case 157:
      if (timeWaitGPRS==0) flGPRSState=156;
      break;
    case 158:
      timeWaitGPRS=10;
      // проверка соединения с сервером.
      GSM_SERIAL.println(F("AT+CIPSTATUS"));
      flGPRSState=159;
    case 159:
      if (timeWaitGPRS==0) flGPRSState=158;
      break;
    case 167: // Промежуточное состояние - к серверу подключены, идет ожидание OK
      if (timeWaitGPRS==0) flGPRSState=158;
      break;
  
  
    case 160:
      timeWaitGPRS=10;
      if (FlToGSM==10) sprintf_P(my_gprs_buffer,PSTR("AT+CIPSTART=\"UDP\",\"%u.%u.%u.%u\",%u"),(unsigned int)ip[0],(unsigned int)ip[1],(unsigned int)ip[2],(unsigned int)ip[3],ipPort);
      if (FlToGSM==11) sprintf_P(my_gprs_buffer,PSTR("AT+CIPSTART=\"TCP\",\"%u.%u.%u.%u\",%u"),(unsigned int)ip[0],(unsigned int)ip[1],(unsigned int)ip[2],(unsigned int)ip[3],ipPort);
      // Подключение
      GSM_SERIAL.println(my_gprs_buffer);
      flGPRSState=161;
    case 161:
      if (timeWaitGPRS==0) flGPRSState=160;
      break;
    case 162:
      // Отправка
      timeWaitGPRS=10;
      //GSM_SERIAL.println(F("AT+CIPSEND=54"));
      GSM_SERIAL.println(F("AT+CIPSEND=56"));
      flGPRSState=163;
    case 163:
      if (timeWaitGPRS==0) flGPRSState=162;
      break;
    case 164:
      timeWaitGPRS=20;
      DisplayData();
      break;
    case 165:
      if (timeWaitGPRS==0) flGPRSState=158;
      // Сообщение отправлено
      break;
  
  
    case 180: // Закрываем TCP-UDP соединение и завершаем сеанс
      timeWaitGPRS=10;
      GSM_SERIAL.println(F("AT+CIPCLOSE"));
      flGPRSState=181;    
    case 181:
      flGPRSState=0; // Ничего не делаем, ждем.
      break;
    default:
      // Если состояние неизвестно, то переходим на инициализацию.
      flGPRSState=98;
      break;
  
  
    }
  
  }


    char flNeedSendGPRS=0;
    //char flStatusGPRS=0;      // Флаг выполнения опроса состояния.
    // 0  - неопределено
    // 1  + CREG: 0,1 - зарегестрировано в сети
    // 2  + CREG: 0,1 - не зарегестрировано в сети.
    // 3 +XIIC:    1, 10.104.82.242 успешное подключение к интернету
    // 4 +XIIC:    1, 0.0.0.0       неуспешное подключение к интернету.
    
    
    static char *vid_sms;   // Вид СМС - память или Сим-карта
    static char num_sms[10];// Номер СМС.
    static char flSMS=0; // Признак того, что приходила SMS и она еще не удалена.
    static char fAnGSM=0; // Состояние анализа входящего звонка
    
    
    void ClearBuffer()
    {
      while (GSM_SERIAL.available() > 0) GSM_SERIAL.read();
      pos_rx_buffer=0;
      my_rx_buffer[pos_rx_buffer]=0;
    }
    
    void toUSARTPDU(char *s)
    { 
      char ot[20];
      unsigned char i; 
      char pdu_header[]="0011000B91"; // Заголовок ПДУ.
      char pdu_poster[]="0008FF"; // Заключительная часть ПДУ.
    
      // Выводим заголовок ПДУ
      GSM_SERIAL.print(pdu_header);
      GSM_SERIAL.print(pdu_phone);
      GSM_SERIAL.print(pdu_poster);
      // Выводим длину сообщения в шестнадцатеричном формате.
      sprintf_P(ot,PSTR("%02X"),strlen(s)*2);
      GSM_SERIAL.print(ot);
      // Выводим само сообщение
    
      for(i=0;s[i];i++)
      {
        sprintf_P(ot,PSTR("00%02X"),s[i]);
        GSM_SERIAL.print(ot);
      }
    
      GSM_SERIAL.write(0x1A);
    
    }



    void GetPhonePDU()
    {
      unsigned char i;
      for(i=1;i<=9;i+=2)
      {
        pdu_phone[i-1]=my_phone[i+1];
        pdu_phone[i]=my_phone[i];
      }
      pdu_phone[10]='F';
      pdu_phone[11]=my_phone[11];
      pdu_phone[12]=0;
    }
    
    void GetState()
    {
      int tic1;
      unsigned char hour,minute;
      hour=Seconds / 3600;
      tic1=Seconds % 3600;
      minute=tic1 / 60;
      sprintf_P(my_tx_buffer,PSTR("%02u:%02u I=%3u S=%3u T=%3u,%3u,%3u V=%4u,%4u OFF=%1i,%4uW,U=%3u,%3imm"),hour,minute,IspReg, StateMachine,DS_TEMP(TEMP_KUB),DS_TEMP(TEMP_DEFL),DS_TEMP(TEMP_TSA),U_VODA,U_UROVEN, (int) flAllOff,UstPower,MaxVoltsOut,U_MPX5010);
    
    }
    
    void StateToSMS()
    {
        #ifndef TESTERR
          if (FlToGSM<2 || SMSOnCall==1)  
          { 
            if (flGPRSState!=98) flGPRSState=70;
            timeGPRS=15;
            //    toUSARTPDU(my_tx_buffer);
            lastSMSState=StateMachine;
          }
          else
          {
            // Активизируем разовую отправку состояния на сервер
            if (FlToGSM>1 && !SMSOnCall && timeGPRS==0 && flGPRSState==0)
            {
              flGPRSState=2; // Активируем сессию GPRS
              flNeedRefrServer=1;
              timeGPRS=60;
              lastSMSState=StateMachine;
            }
          }
        
        #else
          DEBUG_SERIAL.print(F("SMS: "));
          DEBUG_SERIAL.println(my_tx_buffer);
        #endif
        
          // Запоминаем последнее переданное состояние на сотовый.
    
    }
    
    void SendGPRS()
    {
      if (FlToGSM==10 || FlToGSM==11)  SendWiFi8266();
    #ifdef TESTGSM
      //     if (timeWaitGPRS % 5 ==0)
      //     {
      //      DEBUG_SERIAL.print(F("Sec:"));
      //      DEBUG_SERIAL.print(Seconds);
      //      DEBUG_SERIAL.print(F(" StGPRS:"));
      //      DEBUG_SERIAL.println((int)flGPRSState);
      //     }
    #endif
    
      switch(flGPRSState)
      {
      case 0:  // Состояние не запущено
        break;
      case 1: // Активируем отправку SMS
    
    
      case 2: // Активирукм GPRS
        //    InitGSM();
        // Проверка подключения
        if (FlToGSM<2)
        {
          flGPRSState=0;
          break;
        }     
        GSM_SERIAL.println(F("AT+XISP=0"));
        timeWaitGPRS=15;
        flGPRSState=3;
      case 3:
        if (timeWaitGPRS==0) flGPRSState=2;
        break;
      case 4:
        timeWaitGPRS=10;
        GSM_SERIAL.println(F("AT+CLCC"));
        flGPRSState=5;
      case 5:
        if (timeWaitGPRS==0) flGPRSState=4;
        break;
      case 6:
        timeWaitGPRS=10;
    #ifdef TESTGSM
        DEBUG_SERIAL.println(F("SendCMD:AT+CHUP"));
    #endif
        GSM_SERIAL.println(F("AT+CHUP")); // Отклоняем вызов.
        flGPRSState=7;
      case 7:
        if (timeWaitGPRS==0) flGPRSState=6;
        break;
      case 10:  // Пытаемся подключить интернет
        if (FlToGSM<2)
        {
          flGPRSState=0;
          break;
        }     
        if (FlToGSM==2)  sprintf_P(my_gprs_buffer,PSTR("at+cgdcont=1,\"IP\",\"internet\""));
        else
          if (FlToGSM==3)  sprintf_P(my_gprs_buffer,PSTR("at+cgdcont=1,\"IP\",\"internet.beeline.ru\""));
          else
            if (FlToGSM==4)  sprintf_P(my_gprs_buffer,PSTR("at+cgdcont=1,\"IP\",\"internet.mts.ru\""));
            else
              if (FlToGSM==5)  sprintf_P(my_gprs_buffer,PSTR("at+cgdcont=1,\"IP\",\"internet.rt.ru\""));
              else
                if (FlToGSM==6)  sprintf_P(my_gprs_buffer,PSTR("at+cgdcont=1,\"IP\",\"internet.tele2.ru\""));
    
        GSM_SERIAL.println(my_gprs_buffer);
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("SendCMD:"));
        DEBUG_SERIAL.println(my_gprs_buffer);
    #endif
        timeWaitGPRS=15;
        flGPRSState=11; // Ничего не делаем, ждем.
      case 11: // Ждем выполнения команды.
        if (timeWaitGPRS==0) flGPRSState=10;
        break;
      case 12: // Не Подключено к интернет - посылаем команду подключения
        GSM_SERIAL.println(F("at+xiic=1")); // Подключаем IP
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("SendCMD:"));
        DEBUG_SERIAL.println(F("at+xiic=1"));
    #endif
        timeWaitGPRS=15;
        flGPRSState=13; // Ничего не делаем, ждем.
      case 13: // Ждем выполнения команды IP
        if (timeWaitGPRS==0) flGPRSState=12;
        break;
      case 14: // Не Подключено к интернет - посылаем команду подключения
        // Проверка подключения
        if (FlToGSM<2)
        {
          flGPRSState=0;
          break;
        }     
    
        // Проверка подключения
        GSM_SERIAL.println(F("at+xiic?")); // Проверка подключения
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("SendCMD:"));
        DEBUG_SERIAL.println(F("at+xiic?"));
    #endif
        timeWaitGPRS=15;
        flGPRSState=15; // Ничего не делаем, ждем.
      case 15: // Ждем выполнения команды IP
        if (timeWaitGPRS==0) flGPRSState=14;
        break;
      case 16: // Не Подключено к интернет - посылаем команду подключения к серверу
    
    #ifndef UDP_PROTOKOL     
        sprintf_P(my_gprs_buffer,PSTR("at+tcpsetup=0,%u.%u.%u.%u,%u"),(unsigned int)ip[0],(unsigned int)ip[1],(unsigned int)ip[2],(unsigned int)ip[3],ipPort);
    #else
        sprintf_P(my_gprs_buffer,PSTR("AT+UDPSETUP=0,%u.%u.%u.%u,%u"),(unsigned int)ip[0],(unsigned int)ip[1],(unsigned int)ip[2],(unsigned int)ip[3],ipPort);
    #endif
    
        GSM_SERIAL.println(my_gprs_buffer);\
    
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("SendCMD:"));
        DEBUG_SERIAL.println(my_gprs_buffer);
    #endif
    
        timeWaitGPRS=15;
        flGPRSState=17; // Ничего не делаем, ждем.
      case 17: // Ждем выполнения команды IP
        if (timeWaitGPRS==0) flGPRSState=16;
        break;
      case 50: // Закрываем TCP соединение
    
    #ifndef UDP_PROTOKOL     
        GSM_SERIAL.println(F("AT+TCPCLOSE=0"));
    #else
        GSM_SERIAL.println(F("AT+UDPCLOSE=0"));
    #endif
    
        timeWaitGPRS=15;
        flGPRSState=51; // Ничего не делаем, ждем.
      case 51:
        if (timeWaitGPRS==0) flGPRSState=50;
        break;
      case 18: // Посылаем команду для подключения к серверу.
        if (NeedGPRS==0) break;
    
    #ifndef UDP_PROTOKOL     
        GSM_SERIAL.println(F("at+tcpsend=0,54"));
    #else
        GSM_SERIAL.println(F("AT+UDPSEND=0,54"));
    #endif
        timeWaitGPRS=15;
        flGPRSState=19; // Ничего не делаем, ждем.
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("SendCMD:"));
        DEBUG_SERIAL.println(F("at+TcUdSend=0,54"));
    #endif
        NeedGPRS=0;
      case 19:
        if (timeWaitGPRS==0) flGPRSState=30;
        break;
      case 20:
        timeWaitGPRS=20;
        DisplayData();
        // Можно отправлять сообщение
        // Отправляем сообщение оно состоит из
        // <id устройства 10 символов>:<содержимое экрана 32 символа>:<состояние контроллера 3 символа># - признак окончания вывода.
        // Итого 48 символов.
        break;
      case 21:
        if (timeWaitGPRS==0) flGPRSState=30;
        // Сообщение отправлено
        break;
      case 25:
        // Отправлено, готовим следующюю отправку
        flGPRSState=18;
        break;
      case 30: // Проверяем доступность соединения
        if (FlToGSM<2)
        {
          flGPRSState=0;
          break;
        }     
        timeWaitGPRS=15;
        GSM_SERIAL.println(F("AT+IPSTATUS=0"));
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("Status:"));
        DEBUG_SERIAL.println(F("AT+IPSTATUS=0"));
    #endif
        flGPRSState=31;
        break;
      case 31: // Проверяем доступность соединения
        if (timeWaitGPRS==0) flGPRSState=30;
        break;
      case 40: // Звонок по аварии
        //Если телефон на задан или первый знак в номере не, тогда считаем что дозвонились.
        if (!strcmp(my_phone,"+77777777777") || my_phone[0]!='+')
        {  
          flNeedCall=3;       // Ставив признак дозвона.
          flGPRSState=2;      // Переходим в состояние отправки информации по GPRS.  
          break;
        }
    
        GSM_SERIAL.print(F("ATD "));
        GSM_SERIAL.print(my_phone);
        GSM_SERIAL.println(F(";"));
        flGPRSState=41;
        flNeedCall=2; // Ставим признак дозвона
        timeWaitGPRS=20; // Надо дозвониться за 20 секунд
      case 41:
    
        if (timeWaitGPRS==0) 
        { 
          flGPRSState=0; // Не дозвонились за указанное время, тогда
          GSM_SERIAL.println(F("ATH")); // Сбрасываем звонок.
          flNeedCall=1;  // Устанавливаем признак необходимости дозвона.
        }; // Ставим признак дозвона
        break;
      case 60: // Чтение SMS сообщения
        timeWaitGPRS=10;
        GSM_SERIAL.print(F("AT+CPMS="));
        GSM_SERIAL.println(vid_sms);
        flGPRSState=61;
      case 61: // Ждем чтения сообщения
        if (timeWaitGPRS==0) flGPRSState=60;
        break;
      case 62: // Чтение SMS сообщения
        // Очищаем буфер приемки
        pos_rx_buffer=0;
        timeWaitGPRS=5;
        GSM_SERIAL.print(F("AT+CMGR="));
        GSM_SERIAL.println(num_sms);
        flGPRSState=63;
      case 63: // Ждем чтения сообщения
        if (timeWaitGPRS==0) flGPRSState=62;
        break;
      case 64: // Удаление сообщения
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("DELETING:"));
        DEBUG_SERIAL.println(num_sms);
    #endif 
    
        GSM_SERIAL.print(F("AT+CMGD="));
        GSM_SERIAL.println(num_sms);
        timeWaitGPRS=10;
        flGPRSState=65;
      case 65: // Удаление сообщения
        if (timeWaitGPRS==0) flGPRSState=64;
        break;
      case 66: // Отправка сообщения - перевод в PDU режим - пока пользуемся старой функцией отправки SMS
      case 67:
      case 68: // Отправка сообщения - перевод в формат GSM
      case 69:
        flGPRSState=0;
        break;
      case 70: // Отправка сообщения - команда на отправку, ожидание >
        GetState();
        sprintf_P(my_gprs_buffer,PSTR("AT+CMGS=%2i"),14+strlen(my_tx_buffer)*2);// выводим количество окетов
        GSM_SERIAL.println(my_gprs_buffer);
        timeWaitGPRS=15;
        flGPRSState=71;
      case 71:
        // Ждем >
        if (timeWaitGPRS==0) flGPRSState=70;
        break;
      case 72: // Отправка сообщения - собственно отправка
        //    GetStateTXT();
        toUSARTPDU(my_tx_buffer);
        timeWaitGPRS=10;
        flGPRSState=73;
      case 73:
        flGPRSState=0;
        if (timeWaitGPRS==0) flGPRSState=0;
        break;
      case 90: // Закрываем TCP соединение
        timeWaitGPRS=10;
    
    #ifndef UDP_PROTOKOL     
        GSM_SERIAL.println(F("AT+TCPCLOSE=0"));
    #else
        GSM_SERIAL.println(F("AT+UDPCLOSE=0"));
    #endif
    
        flGPRSState=91;    
      case 91:
        flGPRSState=0; // Ничего не делаем, ждем.
        break;
      case 98: // Инициализация GSM
        // Включаем уведомления о SMS
        timeWaitGPRS=10;
        GSM_SERIAL.println(F("AT"));
        flGPRSState=99;    
      case 99:
        if (timeWaitGPRS==0) flGPRSState=98;
        break;
      case 100: // Инициализация GSM
        // Включаем уведомления о SMS
        timeWaitGPRS=10;
        GSM_SERIAL.println(F("AT+CNMI=1,1"));
        flGPRSState=101;    
      case 101:
        if (timeWaitGPRS==0) flGPRSState=100;
        break;
      case 102: // Инициализация GSM
        // Включаем внутренний стек модема.
        timeWaitGPRS=10;
        GSM_SERIAL.println(F("AT+XISP=0"));
        flGPRSState=103;    
      case 103:
        if (timeWaitGPRS==0) flGPRSState=102;
        break;
      case 104: // Инициализация GSM
        // Включаем уведомления о SMS
        timeWaitGPRS=10;
        sprintf_P(my_gprs_buffer,PSTR("AT+CSCS=\"GSM\""));
        GSM_SERIAL.println(my_gprs_buffer);
        flGPRSState=105;    
      case 105:
        if (timeWaitGPRS==0) flGPRSState=104;
        break;
      case 106: // Инициализация GSM
        // Включаем формат SMS в виде PDU
        timeWaitGPRS=10;
        GSM_SERIAL.println(F("AT+CMGF=0"));
        flGPRSState=107;    
      case 107:
        if (timeWaitGPRS==0) flGPRSState=106;
        break;
      case 108: // Инициализация GSM
        // Включаем формат SMS в виде PDU
        timeWaitGPRS=10;
        GSM_SERIAL.println(F("AT+CMGD=1"));
        flGPRSState=109;    
      case 109:
        if (timeWaitGPRS==0) flGPRSState=108;
        break;
      case 110: // Инициализация GSM
        // Включаем формат SMS в виде PDU
        timeWaitGPRS=10;
        GSM_SERIAL.println(F("AT+CMGD=2"));
        flGPRSState=111;  
        stateAfterDelete=10;  
      case 111:
        if (timeWaitGPRS==0) flGPRSState=110;
        break;
        // Это системное состояние - пауза нужное колчичество времени, после паузы программа перейдет на заданое состояние
      case 80:
        if (timeWaitGPRS==0) flGPRSState=stateAfterDelete;
        break;
        // Это состояние переводит в паузу перед повторным вызовом состояния подключения к серверу
      case 81:
        timeWaitGPRS=2;
        flGPRSState=80;
        stateAfterDelete=14;
        break;
    
      }
    }
    
    
    void  StateToGPRS()
    { 
      flNeedSendGPRS=1;
    }
    
    void AnalyseCommand()
    {
    #ifdef TESTGSM
      // Дублируем информацию на Serial1
      DEBUG_SERIAL.print(F("GetComm:"));
      DEBUG_SERIAL.println(my_rx_buffer);
      DEBUG_SERIAL.print(F(" State: "));
      DEBUG_SERIAL.print((int)flGPRSState);
      DEBUG_SERIAL.print(F(" Time: "));
      DEBUG_SERIAL.println((int)timeWaitGPRS);
    #endif
    
    #ifdef TESTGSM1
      //      DEBUG_SERIAL.print(F("S:"));
      //      DEBUG_SERIAL.println(Seconds);
    #endif
    
    
    
      if(!strcmp("OK",my_rx_buffer)) // Получили результат OK, значит анализируем состояние автомата.
      {
    
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("Get OK flGPRSState:"));
        DEBUG_SERIAL.println((int)flGPRSState);
    #endif
    
        switch(flGPRSState)
        {
    
        case 3:
          flGPRSState=10;
          break;
        case 5:
          flGPRSState=6; // 
          break;
        case 7:
          // Если сотояние сети зарегестрирован, то считаем успешно, если нет, то тоже успешно, поскольку пока не знаю, что с этим состоянием делать
          // А если состояние другое, значит что-то не определилось, пробуем заново.
          // получили ОК после выполнения команды 
          //at+creg?
          // Перед ОК было вывод состояния в виде результата
          // +CREG: 0,1
          // Отклонили вызов если звонили с нужного номера, то либо активизируем сессию GPRS либо отправляем информацию по SMS
          if (flRing)
          {
            if (FlToGSM<2 || SMSOnCall==1) 
            {
    
              flGPRSState=70; // Переводим в режим отрпавки SMS.
              timeGPRS=30;
    #ifdef TESTGSM
              DEBUG_SERIAL.print(F("Actvate SMS:"));
              DEBUG_SERIAL.println((int)flGPRSState);
    #endif
            }
            else
            { 
              flGPRSState=14; // Активируем сессию GPRS
    
              timeGPRS=120; // Установив внемя для отправки GPRS сообщений
              flNeedRefrServer=0; // Снмаем флаг необходимости разовой отправки сведений на сервер
              ErrGPRSConnectInernet=0;
              ErrGPRSConnectServer=0;
            }
    
          }
          else 
            flGPRSState=0; 
    
          break;
    
        case 8:
          // Если сотояние сети зарегестрирован, то считаем успешно, если нет, то тоже успешно, поскольку пока не знаю, что с этим состоянием делать
          // А если состояние другое, значит что-то не определилось, пробуем заново.
          // получили ОК после выполнения команды 
          //at+creg?
          // Перед ОК было вывод состояния в виде результата
          // +CREG: 0,1
          flGPRSState=10;
          break;
        case 11:
          // получили ОК после выполнения команды 
          //at+cgdcont=1,"IP","internet"
          flGPRSState=12;
          break;
        case 13:
          // получили ОК после выполнения команды 
          //at+xiic=1
          // После команды посылки связи с интернетом ждем 2 секунды
          timeWaitGPRS=2; // Делаем паузу 2 секунды, затем переходим на проверку связи.
          flGPRSState=80;
          stateAfterDelete=14;
          break;
        case 15:
          // получили ОК после выполнения команды 
          //at+xiic?
          // Перед ОК было вывод состояния в виде результата
          // +XIIC:    1, 10.104.82.242
          // ErrGPRSConnectInernet=0;
          break;
        case 17:
          // получили ОК после выполнения команды 
          // at+tcpsetup=0,09.23.22.16,1211
          // Но состояние после этой команды не изменяем, поскольку ОК идет до того, как ответит сервер.
          flGPRSState=17;
          break;
        case 21:
          //flGPRSState=25;
          break;
        case 65: // Удалили SMS сообщение
          flGPRSState=stateAfterDelete;
          break;
        case 67: // 
          flGPRSState=68;
          break;
        case 69: // 
          flGPRSState=70;
          break;
        case 73: // 
          flGPRSState=0;
          break;
        case 99:
          flGPRSState=100;
          //
          if (FlToGSM==10 || FlToGSM==11) flGPRSState=150; 
    
          break;
        case 101:
          flGPRSState=102;
          break;
        case 103:
          flGPRSState=104;
          break;
        case 105:
          flGPRSState=106;
          break;
        case 107:
          flGPRSState=108;
          break;
        case 109:
          flGPRSState=110;
          break;
        case 111:
          flGPRSState=stateAfterDelete;
          if (flGPRSState==0) timeGPRS=10;          
          break;
        case 151:
          flGPRSState=152;
          break;
        case 153:
          flGPRSState=154;
          break;
        case 157: // Подключились
          flGPRSState=154; // Переход на передачу информации.
          break;
        case 159: // Проверка подключения с серверу, если дошли до ОК, значит не подключены, вызываем подключение
          // Дошли до 
          flGPRSState=160; // Переход на подключение к серверу.
          break;
        case 161: // Подключены к серверу по TCP или UDP
          // Дошли до 
          flGPRSState=162; // Переход на передачу данных.
          break;
        case 167: // Проверка подключения с серверу выполнена вызываем отправку данных
          // Дошли до 
          flGPRSState=160; // Переход на отправку данных.
          break;
        case 168: // Проверка подключения к wi-fi выполнена вызываем отправку данных
          // Дошли до 
          flGPRSState=stateAfterDelete; // Переход на отправку данных.
          break;
    
        default:
          break;
        }
        return;
      }
    
      if(!strcmp("ERROR",my_rx_buffer)) // Получили результат OK, значит анализируем состояние автомата.
      {
        ClearBuffer(); 
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("Get ERROR State:"));
        DEBUG_SERIAL.println((int)flGPRSState);
    #endif
    
        switch(flGPRSState)
        {
        case 65:
          flGPRSState=0;
          break;
        case 101: // при этих ошибках возвращаемся снова на команду AT
        case 103:
        case 105:
        case 107:
        case 109:
        case 111:
          flGPRSState=98;
          break;
        case 157:
          flGPRSState=156;
          break;
        case 162:
          flGPRSState=158;
          break;
        case 168: // прогнозируемая ошибка, например повторное подключение к серверу.
          flGPRSState=stateAfterDelete; // Переход на отправку данных.
          break;
        default:
          ClearBuffer();
          if (FlToGSM<2)
          {
            break;
          }     
          flGPRSState=30;
          if (FlToGSM==10 || FlToGSM==11)  flGPRSState=150;
          break;
        }
    
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("Change ERROR State:"));
        DEBUG_SERIAL.println((int)flGPRSState);
    #endif
    
        return;
      }
    
      if (strstr(my_rx_buffer,"+CREG: ")) // Информация о состоянии подключения
      {  
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("State +CREG:"));
        DEBUG_SERIAL.println((int)flGPRSState);
    #endif
        return;
      }
    
    
      if (strstr(my_rx_buffer,"+XIIC:")) // Информация о состоянии подключения
      {  
    
        if (strstr(my_rx_buffer,"0.0.0.0"))  
        {  
          ErrGPRSConnectInernet++; 
          stateAfterDelete=81; 
          flGPRSState=168; 
        }// Неуспешное подключение к инернету - переодим в режим ожидания 2 сек перед повторым вызовом состояния подклчения
        else  
        {  
          stateAfterDelete=16; 
          flGPRSState=168;
          ErrGPRSConnectInernet=0;  
        } //  Успешное подключение к интернету
    
    
        if (ErrGPRSConnectInernet>40)
        {
    #ifdef TESTGSM
          DEBUG_SERIAL.print(F("Err conn failed internet:"));
          DEBUG_SERIAL.println((int)ErrGPRSConnectInernet);
    #endif
          flGPRSState=0;
        }
        else
          if (ErrGPRSConnectInernet>20)
          {
    #ifdef TESTGSM
            DEBUG_SERIAL.print(F("Err conn internet:"));
            DEBUG_SERIAL.println((int)ErrGPRSConnectInernet);
    #endif
            stateAfterDelete=2;
            flGPRSState=168;
          }
    
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("State +XIIC:"));
        DEBUG_SERIAL.println((int)flGPRSState);
    #endif
        return;
      }
    
      if (strstr(my_rx_buffer,"+CIFSR:STAIP")) // Информация о состоянии подключения wi-fi
      {  
    
        if (strstr(my_rx_buffer,"0.0.0.0"))  {  
          flGPRSState=168; 
          stateAfterDelete=156; 
          ErrGPRSConnectInernet++;
        } // Неуспешное подключение к wi-fi
        else  {
          flGPRSState=168;
          stateAfterDelete=158; 
          ErrGPRSConnectInernet=0;
        } //  Успешное подключение к wi-fi, переход к ожиданию ОК.
    
        if (ErrGPRSConnectInernet>40)
        {
    #ifdef TESTGSM
          DEBUG_SERIAL.print(F("Err conn failed Wi-FI:"));
          DEBUG_SERIAL.println((int)ErrGPRSConnectInernet);
    #endif
          flGPRSState=0;
        }
        else
          if (ErrGPRSConnectInernet>20)
          {
    #ifdef TESTGSM
            DEBUG_SERIAL.print(F("Err conn Wi-Fi:"));
            DEBUG_SERIAL.println((int)ErrGPRSConnectInernet);
    #endif
            flGPRSState=168; 
            stateAfterDelete=150;
          }
    
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("+CIFSR:STAIP"));
        DEBUG_SERIAL.println((int)flGPRSState);
    #endif
        return;
      }
    
    
    #ifndef UDP_PROTOKOL
      if (strstr(my_rx_buffer,"+TCPSETUP:")) // Информация о состоянии к серверу
    #else
        if (strstr(my_rx_buffer,"+UDPSETUP:")) // Информация о состоянии к серверу
    #endif  
    
        {
          if (strstr(my_rx_buffer,":0,OK"))  { 
            flGPRSState=30; 
            ErrGPRSConnectServer=0;
          }// Успешно подключено к серверу
          else 
            if (strstr(my_rx_buffer,":Error")) { 
            flGPRSState=50; 
            ErrGPRSConnectServer++;
          }// Ошибка подключения, закрываем соединение и пробуем еще раз
          else  
          { 
            flGPRSState=14; 
            ErrGPRSConnectServer++;
          }// Неуспешно подключено пробуем подключение еще раз.
    
          if (ErrGPRSConnectServer>20) 
          {
    #ifdef TESTGSM
            DEBUG_SERIAL.print(F("Error connect to Server count="));
            DEBUG_SERIAL.println((int)ErrGPRSConnectServer);
    #endif
            flGPRSState=2;
          } 
    
          return;
        }
    
      if (strstr(my_rx_buffer,"+IPSTATUS:")) // Информация о состоянии к серверу
      {
        //            +IPSTATUS:0,CONNECT,TCP,2047
        if (strstr(my_rx_buffer,":0,CONNECT,"))  { 
          flGPRSState=18; 
        }// Успешно подключено к серверу, можно отправлять данные. 
        else 
        { 
          flGPRSState=16;
        }// Неуспешно подключено пробуем подключение еще раз.
        return;
      }
    
      if (strstr(my_rx_buffer,"+CIPSTATUS:")) // Информация о состоянии к серверу для wi-fi
      {
        if (strstr(my_rx_buffer,":0,"))  
        { 
          // состояние Ожидание ОК
          flGPRSState=167; 
        }// Успешно подключено к серверу, можно отправлять данные. 
        else 
        { 
          flGPRSState=160;
        }// Неуспешно подключено пробуем подключение еще раз.
        return;
      }
    
    
    #ifndef UDP_PROTOKOL
      if (strstr(my_rx_buffer,"+TCPSEND:")) // Информация о состоянии к серверу - отправка ошибка, проверяем коннект к серверу и пробуем еще.
    #else
        if (strstr(my_rx_buffer,"+UDPSEND:")) // Информация о состоянии к серверу - отправка ошибка, проверяем коннект к серверу и пробуем еще.
    #endif
        {
    
          if (strstr(my_rx_buffer,":Error")) 
          {
            // Если ошибка, тогда идем на проверку связи
            flGPRSState=30; 
          }
          else
          {
            // Если состояние - сообщение отправлено, решаем что делать дальше
            if (flGPRSState==21) 
            { 
              if (flNeedRefrServer) 
              {  // Если это разовая отправка
                // Постоим в режиме ожидания, не разывая соединения, ожидая возможной команыд от серверка
                timeWaitGPRS=10;
                timeGPRS=5;
                flNeedRefrServer=0; // Снмаем флаг необходимости отправки сведений на сервер
                timeRefrServer=PeriodRefrServer;// Взводим таймер на следующую отправку
              }   
              else
                flGPRSState=25;  // Если отправка не разовая, тогда переводим в режим следующей отправки.
            }
          }
          return;
        }
    
    
      if (strstr(my_rx_buffer,"SEND OK")) // информация отправлена.
      {
    
        // Если состояние - сообщение отправлено, решаем что делать дальше
        if (flGPRSState==165) 
        { 
          if (flNeedRefrServer) 
          {  // Если это разовая отправка
            // Постоим в режиме ожидания, не разывая соединения, ожидая возможной команыд от серверка
            timeWaitGPRS=10;
            timeGPRS=5;
            flNeedRefrServer=0; // Снмаем флаг необходимости отправки сведений на сервер
            timeRefrServer=PeriodRefrServer;// Взводим таймер на следующую отправку
          }   
          else
            flGPRSState=162;  // Если отправка не разовая, тогда переводим в режим следующей отправки.
        }
        return;
      }
    
    
    
    #ifndef UDP_PROTOKOL
      if (strstr(my_rx_buffer,"+TCPRECV:")) // Информация о состоянии к серверу
    #else
        if (strstr(my_rx_buffer,"+UDPRECV:")) // Информация о состоянии к серверу
    #endif
        {
          // Команда состоит из двух частей первая - это символ команды, вторая это номер по порядку
          // например U1 - нажата кнопка вверх, номер команды 1
          cmdGPRS=my_rx_buffer[strlen(my_rx_buffer)-2];
    
          cmdGPRSIsp=my_rx_buffer[strlen(my_rx_buffer)-1]; // э
    
    #ifdef TESTGSM
          DEBUG_SERIAL.print(F("Get cmd keys:"));
          DEBUG_SERIAL.print(cmdGPRS);
          DEBUG_SERIAL.println(cmdGPRSIsp);
    #endif
    
          if (cmdGPRS>'A')
          {
    
            if (cmdGPRS=='T') StateMachine=1;        // Старт процесса
            if (cmdGPRS=='P') StateMachine=100;     // Стоп процесса
            if (cmdGPRS=='E') flNeedRefrServer=1;     // Если подана команда прекратить постоянный обмен,то включаем режим разовой отправки, после которой прекратится постоянный обмен.
            else  flNeedRefrServer=0;     // Иначе отключаем режим разовой отправки.
            ScanKbd();
            timeGPRS=120;
    
          }
    
          // Пробуем закрывать сессию после получения данных
          return;
    
        }
    
    
      if (strstr(my_rx_buffer,"+IPD,")==my_rx_buffer) // получили команду от сервера (учитываем команду только если она начинается на +IPD,
      {
        // Команда состоит из двух частей первая - это символ команды, вторая это номер по порядку
        // например U1 - нажата кнопка вверх, номер команды 1
        //      cmdGPRS=my_rx_buffer[strlen(my_rx_buffer)-2];
        //      
        //      cmdGPRSIsp=my_rx_buffer[strlen(my_rx_buffer)-1]; // э
        cmdGPRS=my_rx_buffer[7];
        cmdGPRSIsp=my_rx_buffer[8]; // э
    
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("Get cmd keys:"));
        DEBUG_SERIAL.print(cmdGPRS);
        DEBUG_SERIAL.println(cmdGPRSIsp);
    #endif
    
        if (cmdGPRS>'A')
        {
    
          if (cmdGPRS=='T') StateMachine=1;        // Старт процесса
          if (cmdGPRS=='P') StateMachine=100;     // Стоп процесса
          if (cmdGPRS=='E') flNeedRefrServer=1;     // Если подана команда прекратить постоянный обмен,то включаем режим разовой отправки, после которой прекратится постоянный обмен.
          else  flNeedRefrServer=0;     // Иначе отключаем режим разовой отправки.
          ScanKbd();
          if (flGPRSState==0) flGPRSState=14;
          timeGPRS=250;
        }
        return;
      }
    
    
      if(!strcmp("+PBREADY",my_rx_buffer) || !strcmp("MODEM:STARTUP",my_rx_buffer)) // Получили готовность модема, значит все сначала.
      {
        timeGPRS=240;
        stateAfterDelete=0;
        // Если сотояние не начальное,то переходим на инициализацию модема, после инициализации активируем отправку GPRS.
        if (flGPRSState>0)  stateAfterDelete=2;
        // Переходим на инициализацию модема.
        flGPRSState=98;
        return;
      }  
    
      if(!strcmp("invalid",my_rx_buffer)) // Пере-включение Wi-Fi
      {
        timeGPRS=120;
        stateAfterDelete=0;
        // Переходим на инициализацию GPRS.
        flGPRSState=98;
        return;
      }
    
      if(!strcmp("NO CARRIER",my_rx_buffer)) // Получили отказ связи
      {
        if (flGPRSState==41) // Если состояние - дозвон
        {
          flNeedCall=3;       // Ставив признак дозвона.
          flGPRSState=2;      // Переходим в состояние отправки информации по GPRS.  
        }
        return;
      }  
    
      if(!strcmp("ALREADY CONNECTED",my_rx_buffer)) // Получили готовность модема, значит все сначала.
      {
        // Переходим на отправку данных 
        if (flGPRSState>150)   
        { 
          stateAfterDelete=162; 
          flGPRSState=168;
        }
        return;
      }  
    
    
    
    #ifndef UDP_PROTOKOL
      if (strstr(my_rx_buffer,"+TCPCLOSE:")) // Сервер отключился.
    #else
        if (strstr(my_rx_buffer,"+UDPCLOSE:")) // Сервер отключился.
    #endif
    
        {
          // Если находимся в режиме закрытия
          if (flGPRSState!=0) flGPRSState=16; // 
    #ifdef TESTGSM
          DEBUG_SERIAL.print(F("State +TCP_UDP_CLOSE:"));
          DEBUG_SERIAL.println((int)flGPRSState);
    #endif
          return;
    
        }
    
      if (strstr(my_rx_buffer,"CLOSED")) // Сервер отключился.
      {
        // Если находимся в режиме закрытия
        if (flGPRSState!=0) flGPRSState=160; // 
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("State WI-FI CLOSE:"));
        DEBUG_SERIAL.println((int)flGPRSState);
    #endif
        return;
      }
    
    
      //+CLCC: 1,1,4,0,0,"+77777777777",145
      if (strstr(my_rx_buffer,"+CLCC:")) // получили номер телефона
      {
        flRing=0;
        if (strstr(my_rx_buffer,my_phone)) flRing=1;
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("State +CLCC:"));
        DEBUG_SERIAL.println((int)flRing);
    #endif
        return;
    
    
      }
    
      if (strstr(my_rx_buffer,"+CPMS:")) // получили номер телефона
      {
        flRing=0;
        flGPRSState=62; // 
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("State +CMPS:"));
        DEBUG_SERIAL.println((int)flRing);
    #endif
        return;
      }
    
      // Если ожидаем SMS, тогда это SMS
      if (flGPRSState==63 && strlen(my_rx_buffer)>40)
      {
        // Если сообщение больше, чем 60 символом, считмаем, что это SMS, полученный из телефона.
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("GETSMS:"));
        DEBUG_SERIAL.println(my_rx_buffer);
    #endif 
        // Ищем в СМС свой номер телефона
        vid_sms=strstr(my_rx_buffer,pdu_phone);
    
    #ifdef TESTGSM
        DEBUG_SERIAL.print(F("addr:"));
        DEBUG_SERIAL.println((int)vid_sms,HEX);
    #endif 
    
        if (vid_sms)
        {// Если найден номер телефона, тогда выделяем информацию из него
          // Информацией считаем последние четыре принятых байта (то есть один символ в формате Unicode)
          //   
          stateAfterDelete=0;
          vid_sms=my_rx_buffer+strlen(my_rx_buffer)-4;
    #ifdef TESTGSM
          DEBUG_SERIAL.print(F("INFOSMS:"));
          DEBUG_SERIAL.println(vid_sms);
    #endif 
          // Если получили команду на включение котроллера, устанавливаем состояние машины в первое рабочее.
          if(!strcmp(sms_start,vid_sms)) 
          {
    #ifdef TESTGSM
            DEBUG_SERIAL.println(F("Start on SMS"));
    #endif 
            StateMachine=1;
          }
          // Если получили команду на включение котроллера, устанавливаем состояние машины в конечное.
          if(!strcmp(sms_stop,vid_sms)) 
          {
            StateMachine=100;
            digitalWrite(PIN_ALL_OFF,ALL_OFF_HIGH);
            flAllOff=1;
    
    #ifdef TESTGSM
            DEBUG_SERIAL.println(F("Stop on SMS"));
    #endif 
          }
          if(!strcmp(call_sms,vid_sms)) 
          {
            SMSOnCall=1;
    #ifdef TESTGSM
            DEBUG_SERIAL.println(F("Sms on call"));
    #endif 
          }
    
          if(!strcmp(call_gprs,vid_sms)) 
          {
            SMSOnCall=0;
    #ifdef TESTGSM
            DEBUG_SERIAL.println(F("GPRS on call"));
    #endif 
          }
    
        }
        // Переходим в состояние удаления SMS.
        flGPRSState=64;
        return;
      }
    
      if (!strcmp("RING",my_rx_buffer)) // Если входящий звонок, то определяем номер.
      {
    
        timeGPRS=60;  
        flGPRSState=4; // 
    #ifdef TESTGSM
        DEBUG_SERIAL.println(F("Get RING:"));
    #endif
        return;
    
      }
      else
      {
        vid_sms=strstr(my_rx_buffer,"+CMTI:");
        if (vid_sms)
        {
          vid_sms+=6;
          vid_sms[5]=0;
          // Запоминаем номер СМС.
          strcpy(num_sms,&vid_sms[6]);
          // Выдаем запрос на чтение смс из того места, откуда она пришла.
    #ifdef TESTGSM
          DEBUG_SERIAL.print(F("Get num SMS:"));
          DEBUG_SERIAL.print(num_sms); 
          DEBUG_SERIAL.print(F("Vid SMS:"));                  
          DEBUG_SERIAL.println(vid_sms); 
    #endif
          timeGPRS=40;
          flGPRSState=60;
          flSMS=1;
        }
        return;
      }
    }

   


    void ProcessGSM()
    {   
      char s_rx;
    
      while (GSM_SERIAL.available() > 0)     
      {
        s_rx=GSM_SERIAL.read();
    
    
    #ifdef TESTGSM1
        // Дублируем информацию на Serial1
        DEBUG_SERIAL.write(s_rx);
    #endif
    
        // Получили приглашение ко вводу SMS или GPRS или Wi-Fi
        if (s_rx=='>' && (flGPRSState==19 || flGPRSState==71 || flGPRSState==163 || flGPRSState==73)) 
        {
    #ifdef TESTGSM
          // Дублируем информацию на Serial1
          DEBUG_SERIAL.print(F("Get>:"));
          DEBUG_SERIAL.println((int)flGPRSState);
    #endif
    #ifdef TESTGSM1
          DEBUG_SERIAL.println(F("Get>:"));
          DEBUG_SERIAL.print(F("S:"));
          DEBUG_SERIAL.println(Seconds);
    #endif
    
          if (flGPRSState!=73)  flGPRSState++;
          else  flGPRSState=72;
          SendGPRS();
        }
        else
          if (s_rx==10)
          {
            my_rx_buffer[pos_rx_buffer-1]=0;
            pos_rx_buffer=0;
            AnalyseCommand();
            SendGPRS();
          }
          else
          {
            if (pos_rx_buffer>=MY_RX_BUFFER_SIZE) pos_rx_buffer=0;
            my_rx_buffer[pos_rx_buffer]=s_rx;
            pos_rx_buffer++;
          }
      }
    
      //если есть доступные данные
      // считываем байт
      // Если время выдачи информацции закончилось и информация выдана
      if (timeGPRS!=0)
      {
        // Если слишком долго ничего не получаем, и в буфере что-то есть, то анализируем то, что есть.
        //    if (timeWaitGPRS==0 && pos_rx_buffer>0)
        //    {
        //      my_rx_buffer[pos_rx_buffer-1]=0;
        //      AnalyseCommand();
        //    }
        SendGPRS();
      }
      else
      {
        if  (flGPRSState==20 || flGPRSState==164)  SendGPRS();
        else
          if  (flGPRSState!=0) 
          {
            // Если соединение не закрыто, то закроем его, но только в том случае если модем прошел инициализацию.
            if (flGPRSState==98 || flGPRSState==99)
            {
              flGPRSState=98;
            }
            else
            {
              flGPRSState=90;
              if (FlToGSM==10 || FlToGSM==11) flGPRSState=180;
              SendGPRS();
            }
          } 
      }
    
      // Окончание анализа сотового  
    }
#endif // #if USE_GSM_WIFI == 1
