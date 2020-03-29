void initGSM() {//TODO
 
  gsmSerial.setTimeout(100); // sets the maximum number of milliseconds to wait for readString()
  
  memset(GSMstring,0,sizeof(GSMstring));
  GSMstring[0] = '\0';
  lcd.setCursor(11, 1);

  #ifdef DEBUGGING
  gsmSerial.begin(9600);
  PRINTINFO("already initialized?");
  for (uint8_t i = 0; i < 4; i++)  {
    gsmSerial.println(F("AT")); 	
    loadingAnimation();
    gsmSerial.println(F("ATE0")); //echo off
    if (gsmSerial.find(strcpy_P(string_buff, PSTR("OK")))) {//copy PROGMEM to buff and find answer in GSM serial
      PRINTLNF("AT ok");
      GSMinitOK = true;
      cleanSerialGSM();
      return;
    }
  }
  #endif
  
  PRINTINFO("switching GSM");
  pinMode(GSM_PIN, OUTPUT);
  digitalWrite(GSM_PIN, LOW);
  loadingAnimation(4); //wait 2 sec
  digitalWrite(GSM_PIN, HIGH);

  PRINTLNF("");
  PRINTLNF("trying to set 9600");
  gsmSerial.begin(115200);
  for (size_t i = 0; i < 4; i++) {
    gsmSerial.println(F("AT+IPR=9600")); //trying to set 9600
    delay(50);
  }
  gsmSerial.begin(9600);
  
  cleanSerialGSM();
  PRINTINFO("initialize GSM");  
  for (uint8_t i = 0; i < 20; i++) {
    gsmSerial.println(F("AT+CPAS"));
    loadingAnimation();// display loading animation
    gsmSerial.println(F("ATE0")); //echo off
    // gsmSerial.readBytesUntil('\n', GSMstring, sizeof(GSMstring));
    // if (strstr_P(GSMstring, PSTR("+CPAS:0")) != NULL) {
    //   PRINTLNF("AT ok");
    //   GSMinitOK = true;
    //   break;
    // }
    // else {
    //   PRINTLN("GSMstring=",GSMstring);
    // }
    if (gsmSerial.find(strcpy_P(string_buff, PSTR("+CPAS:0")))) {
      PRINTLNF("AT ok");
      GSMinitOK = true;
      break;
    }
  }

  if (!GSMinitOK) {
    PRINTINFO("GSM init error");
    cleanSerialGSM();
    return;
  }

  GSMinitOK = false;
  gsmSerial.println(F("ATE0")); //echo off
  loadingAnimation();
  gsmSerial.println(F("AT+CSCS=\"GSM\"")); // "GSM","HEX","PCCP936","UCS2
  PRINTLNF("waiting for SIM");
  loadingAnimation(10); //5 sec waiting if init first time need
  cleanSerialGSM();
  for (uint8_t i = 0; i < 4; i++)  {
    gsmSerial.println(F("AT+CPIN?")); // Query SIM PIN status
    loadingAnimation(4); //2 sec
    gsmSerial.readBytesUntil('\n', GSMstring, sizeof(GSMstring));
    if (strstr_P(GSMstring, PSTR("+CPIN:READY")) != NULL) { // SIM ok
      PRINTLNF("SIM ok");
      GSMinitOK = true;
      break;
    }
    else if (strstr_P(GSMstring, PSTR("+CME ERROR:10")) != NULL) { //no SIM card
      PRINTLNF("no SIM");
      break;
    }
    else {
      PRINTLN("GSMstring=",GSMstring);
    }
  }
  PRINTLNF("");
  if (GSMinitOK) {
    PRINTLNF("init GSM ok");
  }
  else {
    PRINTINFO("GSM init error");
  }
  
  cleanSerialGSM();
}

void requestTime() { //request time from GSM
  PRINTLNF("request time");
  if (GSMwaitReqFlag || GSMonAirFlag) {
    PRINTINFO("GSM is busy");
    timer.setTimeout(readResponseTimeout,  requestTime);
    return;  
  }
  else {
    GSMwaitReqFlag = true;
    timeSyncOK = false;
    readResponseCounter = 0;
    gsmSerial.println(F("AT+CCLK?"));
    PRINTINFO("send time request");
    timer.setTimeout(readResponseTimeout,  readUntilOK, (void*) &timeSyncOK );
  }
}

void requestSignalAndRAM() {//request signal quality from GSM
  memoryFree = freeMemory();
  //PRINTLN("freeMemory()=", memoryFree);

  //PRINTLNF("request signal strength");
  if (GSMwaitReqFlag || GSMonAirFlag) {
    PRINTINFO("GSM is busy");
    //timer.setTimeout(readResponseTimeout,  requestSignalAndRAM);
    return;  
  }
  else {
    GSMwaitReqFlag = true;
    signalSyncOK = false;
    readResponseCounter = 0;
    gsmSerial.println(F("AT+CSQ")); //request signal quality from GSM
    //PRINTINFO("send signal strength.");
    timer.setTimeout(readResponseTimeout,  readUntilOK, (void*) &signalSyncOK );
  }
}

void requestBalance() {//request balance from GSM
  PRINTLNF("request balance");
  if (GSMwaitReqFlag || GSMonAirFlag) {
    PRINTINFO("GSM is busy");
    timer.setTimeout(readResponseTimeout,  requestBalance);
    return;  
  }
  else {
    GSMwaitReqFlag = true;
    balanceSyncOK = false;
    readResponseCounter = 0;
    gsmSerial.println(F("AT+CUSD=1,\"#105#\",15")); //for requst balance TELE2
    PRINTINFO("send balance request");
    timer.setTimeout(readResponseTimeout,  readUntilOK, &balanceSyncOK ); //PRINTLNHEX("balanceSyncOK hex1=",(long)&balanceSyncOK);
  }
}

void readUntilOK(void* syncOKflag) {//wait and read data response TODODOTODOTODO
  readStringGSM(); //PRINTLNHEX("after readStringGSM hex1=",(long)syncOKflag);
  bool sync_ok_flag = *(reinterpret_cast<bool (*)>(syncOKflag));
  if (sync_ok_flag || (readResponseCounter >= readResponseNum) ) { // max <readResponseNum> times to wait
    GSMwaitReqFlag = false;
    return;
  }
  PRINTINFO("wait");
  timer.setTimeout(readResponseTimeout,  readUntilOK, syncOKflag );
  readResponseCounter++;
}

void waitAndReadUntilOK(const bool &syncOK) { //wait and read data response
    for (uint8_t i = 0; i<21; i++) { 
      timer.run(); //running readUntilOK
      if (syncOK) {
      //  PRINTLNF(".syncOK!");
       return;
      }
      loadingAnimation();
  }
}

void readStringGSM() {//read data from GSM module
  memset(GSMstring,0,sizeof(GSMstring));
  GSMstring[0] = '\0';
  if (gsmSerial.available()) {
    if (gsmSerial.readBytesUntil('\n',GSMstring,sizeof(GSMstring)) < 4 ) { // \n  line feed - new line 0x0a 
//      PRINTLN("GSMstring=", GSMstring);
      return;
    }
    else if (strstr_P(GSMstring, PSTR("RING")) != NULL) {//return ring signal (if somebody ringing)
      gsmSerial.println(F("AT+CLCC")); //returns list of current call numbers
      backlightON();
      PRINTLNF("ringin");
    }
    else if (strstr_P(GSMstring, CSQ) != NULL) {//return signal quality, command like +CSQ: 22,99 (if received signal quality data)
      strncpy(GSMstring,strstr_P(GSMstring, CSQ)+6,2);
      GSMstring[2] ='\0';// must return like '22'
      signalStrength = atoi(GSMstring) * 100 / 31; // convert to percent, didnt need to check number, if needed, use sscanf or strtol
      signalSyncOK = true; //maybe return data
      PRINTLN("signal=", signalStrength);
    }
    else if (strstr_P(GSMstring, CCLK) != NULL) {//return time, command like +CCLK: "18/11/29,07:34:36+05" (if received time data), also GSM return this line on init +CTZV:17/07/28,19:03:51,+06 but we didnt process it
      strncpy(GSMstring,strstr_P(GSMstring, CCLK)+8,20); 
      GSMstring[20] ='\0'; //must return 18/11/29,07:34:36+05\0     20/01/26,04:59:38+05
      PRINTLN("time string=", GSMstring);// Day, Month, Year, Hour, Minute, Second, timeZone
      uint8_t i = 0;
      static uint8_t parse_time_arr[7];// Day, Month, Year, Hour, Minute, Second, timeZone, set zeros
      char *search_p = strtok_P(GSMstring, DELIMETERS);
      memset(parse_time_arr, 0, sizeof(parse_time_arr)); //set zeros
      while (search_p != NULL) {
        parse_time_arr[i++] = atoi(search_p);
        search_p = strtok_P(NULL, DELIMETERS);
      }
      if (parse_time_arr[0] < 20) { //year is < 2020 - wrong datetime!
        PRINTLNF("sync clock error");
        return;
      }
      setTime(parse_time_arr[3], parse_time_arr[4], parse_time_arr[5], parse_time_arr[2], parse_time_arr[1], parse_time_arr[0]); //set time and date  setTime(int hr,int min,int sec,int dy, int mnth, int yr)
      adjustTime(parse_time_arr[6] * SECS_PER_HOUR);                                                                             //set timezone
      timeSyncOK = true;
      PRINTLNF("sync clock OK");
    }
    else if (strstr_P(GSMstring, CUSD) != NULL) {//return USSD balance command like +CUSD: 2, "⸮!5H}.A⸮Z⸮⸮⸮⸮." ,1  (if received balance data)
      strncpy(GSMstring,strstr_P(GSMstring, CUSD)+11,64); //cut string will be like OCTATOK 151.8 p." ,1
      decode7Bit(GSMstring, sizeof(GSMstring));
      if (sscanf(GSMstring,"%*[^-0123456789]%d",&balance) == 1){  //find int
        PRINTLN("balance=",balance);
        balanceSyncOK = true; // PRINTLNHEX("balanceSyncOK hex=",(long)&balanceSyncOK);
      }
      else {
        PRINTLNF("check balance error"); 
        balanceSyncOK = false;
      }
    }
    else if ((strstr_P(GSMstring, PSTR("+CLCC")) != NULL) && !GSMonAirFlag) { //if received phone number calling data
      PRINTLNF("received call ID");
      if (checkNumber(GSMstring)) {//check phone number +CLCC:
        GSMonAirFlag = true;
        workFlag = !workFlag;
        PRINTLNF("work switch");
        if (!thermostatFlag) {
          relayFlag = !relayFlag;
          digitalWrite(RELAY_PIN, relayFlag);
        }
        cleanSerialGSM();
        gsmSerial.println(F("ATA"));                       //answer call
        PRINTLNF("answer call");
        timer.setTimeout(1500, playGSM);
        if (currentMenu == 0)
        {
          lcd.clear();
          updateMainScreenFlag = true;
        }
        else
          menu_system.update();
  
        //cleanSerialGSM();
      }
    }
    else if ((strstr_P(GSMstring, PSTR("+CIEV: \"CALL\",0")) != NULL) && GSMonAirFlag) { //+CIEV "CALL",0 - if call is in progress (true/false)
      PRINTLNF("call ended");
      audioQueue.clear();
      //timer.deleteTimer(hangup_timer_id);
      hangUpGSM();
    }
    else
      PRINTLN("GSMstring=", GSMstring);
  }
}

void decode7Bit(char *in_str, uint8_t dataSize) {//decode USSD 7bit response
  char out_str[dataSize-1]  = {'\0'};
  memset(out_str,0,sizeof(out_str));
  byte reminder = 0;
  int bitstate = 7;
  for (byte i = 0; in_str[i] != '\0' || i <dataSize-1; i++) {
      byte b = in_str[i];
      char c = ((b << (7 - bitstate)) + reminder) & 0x7F;
      out_str[i] = c;
      reminder = b >> bitstate;
      bitstate--;
      if (bitstate == 0) {
        out_str[i] = reminder;
        reminder = 0;
        bitstate = 7;
      }
  }
  strcpy(in_str,out_str);
}

bool checkNumber(const char * string_number) {//check phone number +CLCC:
  static const uint8_t list_size = sizeof(phone_table) / sizeof(phone_table[0]); //size of phones list
  for (int i = 0; i < list_size; i++) {                           //4 phone numbers
    strcpy_P(phone_buff, (char *)pgm_read_word(&(phone_table[i]))); // Necessary casts and dereferencing, just copy.
    if (strstr(string_number, phone_buff) != NULL) {
      PRINTLN("call num=", phone_buff);
      return true;
    }
  }
  return false;
}

void cleanSerialGSM() {
  while (gsmSerial.available()) //clean UART buffer
    gsmSerial.read();
}

void hangUpGSM() {
  cleanSerialGSM();
  gsmSerial.println(F("AT+CHUP")); //hang up all calls
  GSMonAirFlag = false;
  GSMwaitReqFlag = false;
  
}

uint16_t addrToRead() { //EEPROM address to read from TODO test
  uint16_t read_byte_pos = 0;
  while(read_byte_pos < EEPROM.length()-sizeof(Log)) { // determine address to read data
    if (bitRead(EEPROM.read(read_byte_pos),7) !=  bitRead(EEPROM.read(read_byte_pos+sizeof(Log)),7)) //compare first bit in structs (sentinels), if not equal - we find address!
      break;
    read_byte_pos+=sizeof(Log); //go to next record
  }
  return read_byte_pos;
}

void sendSMSBalance() {
  PRINTLNF("check to send SMS");
  if (GSMwaitReqFlag || GSMonAirFlag || !timeSyncOK) {
    PRINTINFO("GSM is busy, waiting");
    timer.setTimeout(SECS_PER_MIN * 10000L,  sendSMSBalance); // try 10 min later
    return;  
  }
  GSMwaitReqFlag = true;
  uint16_t read_byte_pos = addrToRead(); //find EEPROM address to read from
  Log log_data;
  EEPROM.get(read_byte_pos, log_data); //read EEPROM data
  PRINTLN("read_byte_pos ",read_byte_pos);
  uint16_t elapsed_days = 0;
  if (now() > log_data.unix_time) {
    elapsed_days = elapsedDays(now()) - elapsedDays(log_data.unix_time);
  }
  PRINTLN("elapsedDays=",elapsed_days);
  // PRINTLN("previous rec time=",log_data.unix_time);
  if ((elapsed_days >= 80) || log_data.unix_time == 0xFFFFFFFF) { //if more than 80 days have passed from last sending OR if empty date (empty data, nothing to read) then write datetitme (data struct) to EEPROM and send new SMS
    uint16_t write_byte_pos = read_byte_pos + sizeof(Log);
    byte sentinel = bitRead(log_data.home_temp,7); //read sentinel bit
    PRINTLN("sentinel ",sentinel);
    if (write_byte_pos >= EEPROM.length()) { //reaching end of EEPROM go to begining (to 0), to the start of EEPROM
      write_byte_pos = 0;
      sentinel ^= 1 ; //invert sentintel bit
    }
    log_data.home_temp = temp[0]; //home temp must be <= 0b00111111, i.e. abs(home_temp) <= 63
    bitWrite(log_data.home_temp,7,sentinel); //set last bit, its sentinel (0b10000000)
    log_data.heater_temp = temp[1];
    log_data.balance = balance;
    log_data.unix_time = now();   
    PRINTLN("write_byte_pos=",write_byte_pos);
    PRINTLN("sentinel=",sentinel);
    EEPROM.put(write_byte_pos, log_data );
    PRINTLNF("write EEPROM OK");
    
    //send SMS
    gsmSerial.println(F("AT+CMGF=1")); // Configuring TEXT mode
    delay(100);
    gsmSerial.print(F("AT+CMGS=+")); //sms to phone number 
    strcpy_P(phone_buff, (char *)pgm_read_word(&(phone_table[0]))); // first phone number from table, number like 7XXXXXXXXXX
    gsmSerial.println(phone_buff);   //   "AT+CMGS=\"+79227754426\""
    delay(100);
    gsmSerial.print(F("Balance: "));
    gsmSerial.print(balance);
    gsmSerial.print(F(" rub.; Home temp: "));
    if (temp[0]==-99) {
      gsmSerial.print(strcpy_P(string_buff, CLEAR_LINE));
    }
    else {
      sprintf(GSMstring, strcpy_P(string_buff, FORMAT_DIGITS_3_SP), temp[0]);
      gsmSerial.print(GSMstring);  
    }
    gsmSerial.print(F("; Heat temp: "));
    if (temp[1]==-99) {
      gsmSerial.print(strcpy_P(string_buff, CLEAR_LINE));
    }
    else {
      sprintf(GSMstring, strcpy_P(string_buff, FORMAT_DIGITS_3_SP), temp[1]);
      gsmSerial.print(GSMstring);  
    }
    gsmSerial.print(F("; Date: "));//print date and time like 17.12.2020 16:47 // sprintf(GSMstring, "Balance: %d; Home temp: %+03d; Heat temp: %+03d; Date: %02d.%02d.%04d %02d:%02d", balance,temp[0],temp[1],day(),month(),year(),hour(),minute());
    sprintf(GSMstring, strcpy_P(string_buff, FORMAT_DIGITS_2), day());
    gsmSerial.print(GSMstring);
    gsmSerial.print('.');
    sprintf(GSMstring, strcpy_P(string_buff, FORMAT_DIGITS_2), month());
    gsmSerial.print(GSMstring);
    gsmSerial.print('.');
    sprintf(GSMstring, strcpy_P(string_buff, PSTR("%04d ")), year());
    gsmSerial.print(GSMstring);
    // gsmSerial.print(' ');
    sprintf(GSMstring, strcpy_P(string_buff, FORMAT_DIGITS_2), hour());
    gsmSerial.print(GSMstring);
    gsmSerial.print(':');
    sprintf(GSMstring, strcpy_P(string_buff, FORMAT_DIGITS_2), minute());
    gsmSerial.print(GSMstring);
    gsmSerial.write(26); //Ctr+Z - end of SMS
    delay(100);
    gsmSerial.println(F("AT+CMGF=0")); // Configuring TEXT mode
    PRINTLNF("Sending SMS done");
    timer.setTimeout(SECS_PER_MIN * 5000L, requestBalance);//request balance 5 min later
  }
    GSMwaitReqFlag = false;
}