//use DFPlayerMini_Fast, 
//add shedule and temp automation, 
//add preferences menu section, 
//add voice temp information, add read BUSY pin +
//!!!!edit LiquidMenu_config.h first!!!!

#define DEBUGGING

const char PROG_VERSION = '2';
const uint8_t RELAY_PIN = A3;                                                                             //pin for Relay
const uint8_t GSM_PIN = 9;                                                                              //pin for GSM
const uint8_t TEMP_SENSOR_PIN = 12;                                                                     //pin for DS18B20
const uint8_t ENCODER_PIN1 = A1;                                                                        //pin for encoder +
const uint8_t ENCODER_PIN2 = A0;                                                                        //pin for encoder -
const uint8_t ENCODER_PIN3 = A2;                                                                        //pin for encoder button
const uint8_t GSM_SERIAL_RX = 2;                                                                        //pin for GSM serial
const uint8_t GSM_SERIAL_TX = 3;                                                                        //pin for GSM serial
const uint8_t MP3_SERIAL_RX = 4;                                                                        //pin for mp3 serial
const uint8_t MP3_SERIAL_TX = 5;                                                                        //pin for mp3 serial
const uint8_t MP3_BUSY = 7;                                                                             //mp3 busy pin
const char PHOME_NUM1[] PROGMEM = {"79227754426"};
const char PHOME_NUM2[] PROGMEM = {"79044719617"};                                                   //pap
const char PHOME_NUM3[] PROGMEM = {"79822219033"};                                                   //pap
const char PHOME_NUM4[] PROGMEM = {"79226505965"};                                                   //tat
const char *const phone_table[] PROGMEM = {PHOME_NUM1, PHOME_NUM2, PHOME_NUM3, PHOME_NUM4}; // phones table 

#include <MemoryFree.h> //https://github.com/maniacbug/MemoryFree TODO
#include <CircularBuffer.h> // https://github.com/rlogiacco/CircularBuffer //TODO del
#include <OneWire.h>             //for DS18B20
#include <DFRobotDFPlayerMini.h> //DF MP3 Player mini
//#include <DFPlayerMini_Fast.h> //DFPlayer MP3 mini https://github.com/scottpav/DFPlayerMini_Fast
#include <ClickEncoder.h>   //encoder with button https://github.com/0xPIT/encoder
#include <TimeLib.h>        //timekeeping https://github.com/PaulStoffregen/Time
#include <SoftwareSerial.h> //for GSM modem A6
#include <SimpleTimer.h>    // Handy timers with callback parameters and polling https://github.com/marcelloromani/Arduino-SimpleTimer
#include <LiquidMenu.h>     //The menu wrapper library https://github.com/microdimmer/LiquidMenu and https://github.com/johnrickman/LiquidCrystal_I2C
#include <EEPROM.h>         //saving log data to EEPROM and send SMS every 2.5 month
#include <LiquidMenu.h>     //The menu wrapper library https://github.com/microdimmer/LiquidMenu and https://github.com/johnrickman/LiquidCrystal_I2C

struct Log {
  int8_t home_temp; // temp +- 63, 1 byte, and sentinel, see this https://sites.google.com/site/dannychouinard/Home/atmel-avr-stuff/eeprom-longevity
  int8_t heater_temp; // heater +- 128, 1 byte
  int16_t balance; // balance +-32967, 2 bytes
  time_t unix_time; // current POSIX time, 4 bytes
}; //total 8 bytes (64 bits)

OneWire ds(TEMP_SENSOR_PIN);
byte DSaddr[2][8]; // first and second DS18B20 addresses, home 28FFE44750170473 heater 28FF2FDAC11704DE

ClickEncoder encoder(ENCODER_PIN1, ENCODER_PIN2, ENCODER_PIN3, 2); //changed to 2 at one turn
ClickEncoder::Button button;
int16_t last_val = 0;
int16_t enc_val = 0;

SoftwareSerial gsmSerial(GSM_SERIAL_RX, GSM_SERIAL_TX);
SoftwareSerial mp3Serial(MP3_SERIAL_RX, MP3_SERIAL_TX);
//DFPlayerMini_Fast mp3Player;
DFRobotDFPlayerMini mp3Player;
SimpleTimer timer;
LiquidCrystal_I2C lcd(0x3F, 16, 2);

char GSMstring[64] = {'\0'}; //GSM read string buffer
char string_buff[8] = {'\0'}; //string buffer, used for showing info on display
int8_t temp[2] = {-99, -99}; // temp home, temp heater
// int8_t t_heater_set = 0;
// int8_t t_home_set = 0;
uint8_t signalStrength = 0;
bool GSMonAirFlag = false; //answer call flag
bool GSMwaitReqFlag = false; // waiting request command flag
uint8_t readResponseCounter = 0; // count response commands to run, max 20 times x 500ms
const uint8_t readResponseNum = 20;  //limit 20 times to wait
const uint16_t readResponseTimeout = 500;  //responce timeout to wait
int16_t backlightTimerID = 0;

uint16_t memoryFree = 0;
int16_t balance = -32768; //min uint
CircularBuffer<uint8_t,16> audioQueue;     // audio sequence size, can play five files continuously CircularBuffer<uint8_t,6> audioQueue;
bool relayFlag = false; 
bool backlightFlag = true;
bool updateMainScreenFlag = true;
bool clearMainSreenFlag = false;
bool SIMreadyOK = false;
bool GSMinitOK = false;
bool timeSyncOK = false;
bool signalSyncOK = false;
bool balanceSyncOK = false;
uint8_t currentMenu = 0; //0 - homepage, 1 - main menu, 2 - info menu, 3 - temp menu

#include "menu.h"
#include "debug.h"

void func() // Blank function, it is attached to the lines so that they become focusable.
{
  PRINTLNF("hello!");
}

void go_switch_relay()
{
  relayFlag = !relayFlag;
  digitalWrite(RELAY_PIN, relayFlag);
  PRINTLNF("relay switch!");
  menu_system.switch_focus();
  menu_system.switch_focus();
  updateMainScreenFlag = true;
  clearMainSreenFlag = true;
  currentMenu = 0;
}

void go_info_menu()
{
  PRINTLNF("changing to info menu!");
  menu_system.change_menu(info_menu);
  menu_system.change_screen(1);
  menu_system.switch_focus();
  currentMenu = 2;
}

//void go_temp_menu()
//{
//  PRINTLNF("changing to temp menu!");
//  menu_system.change_menu(temp_menu);
//  menu_system.change_screen(1);
//  menu_system.switch_focus();
//  currentMenu = 3;
//}

void go_main_menu()
{
  PRINTLNF("changing to main menu!");
  menu_system.switch_focus();
  menu_system.change_menu(main_menu);
  currentMenu = 1;
}

void go_main_screen()
{
  PRINTLNF("changing to main screen!");
  menu_system.switch_focus();
  updateMainScreenFlag = true;
  clearMainSreenFlag = true;
  currentMenu = 0;
}

void loadingAnimation(uint32_t a_delay, uint8_t count = 1) //loading animation TODO
{
  for (uint8_t i = 0; i < count; i++)
  {
    PRINTINFO(".");
    static char string_buff[6] = {0}; //TODO del static
    lcd.setCursor(11, 1);
    lcd.print(string_buff);
    delay(a_delay);
//    PRINTLN("string_buff=", string_buff);
    char *p = strrchr(string_buff,'.');
    if (p == NULL)
      string_buff[0] = '.';
    else
      *++p = '.';

    if (string_buff[5]=='.')
      strcpy_P(string_buff, PSTR("     "));
      // memset(string_buff,0,sizeof(string_buff));
  }
}

void initGSM() //TODO
{  
  lcd.setCursor(11, 1);

  memset(GSMstring,0,sizeof(GSMstring));
  GSMstring[0] = '\0';

  PRINTINFO("check already initialized");
  gsmSerial.begin(9600);
  for (uint8_t i = 0; i < 4; i++)  {
    gsmSerial.println(F("AT")); 	
    loadingAnimation(500);
    if (gsmSerial.find(strcpy_P(string_buff, PSTR("OK")))) {//copy PROGMEM to buff and find answer in GSM serial
      for (uint8_t i = 0; i < 4; i++) {
        gsmSerial.println(F("AT+CPIN?")); // Query SIM PIN status
        loadingAnimation(500,4);
        gsmSerial.readBytesUntil('\n',GSMstring,sizeof(GSMstring));
        if (strstr_P(GSMstring, PSTR("+CPIN:READY")) != NULL) {// SIM ok
          PRINTLNF("GSM already initialized");
          GSMinitOK = true;
          return;
        }
        else if (strstr_P(GSMstring, PSTR("+CME ERROR:10")) != NULL) { //no SIM card
          PRINTLNF("no SIM");
          cleanSerialGSM();
          return;
        }
      }
    }
  }

  cleanSerialGSM();
  
  gsmSerial.begin(115200);
  PRINTINFO("switching on GSM");
  pinMode(GSM_PIN, OUTPUT);
  digitalWrite(GSM_PIN, LOW);

  loadingAnimation(500,4);
  digitalWrite(GSM_PIN, HIGH);
//  loadingAnimation(500,2);

  PRINTLNF("");
  PRINTLNF("trying to set 9600");
  for (size_t i = 0; i < 20; i++)
  {
    gsmSerial.println(F("AT+IPR=9600")); //trying to set 9600
    delay(50);
  }
  gsmSerial.begin(9600);
  
  PRINTINFO("initialize GSM");  
  for (uint8_t i = 0; i < 20; i++)
  {
    gsmSerial.println(F("AT+CPAS"));
    if (gsmSerial.find(strcpy_P(string_buff, PSTR("+CPAS:0")))) {
      for (uint8_t i = 0; i < 4; i++) {
        gsmSerial.println(F("AT+CPIN?")); // Query SIM PIN status
        loadingAnimation(500,4);
        gsmSerial.readBytesUntil('\n',GSMstring,sizeof(GSMstring));
        if (strstr_P(GSMstring, PSTR("+CPIN:READY")) != NULL) {// SIM ok
          PRINTLNF("GSM already initialized");
          GSMinitOK = true;
          break;
        }
        else if (strstr_P(GSMstring, PSTR("+CME ERROR:10")) != NULL) { //no SIM card
          PRINTLNF("no SIM");
          PRINTINFO("GSM init failed!");
          return;
        }
      }
    }
    if (GSMinitOK) 
      break;
    readStringGSM();
    loadingAnimation(500);// display loading animation
  }
    
  gsmSerial.println(F("ATE0")); //echo off
  loadingAnimation(500);
  gsmSerial.println(F("AT+CSCS=\"GSM\"")); // "GSM","HEX","PCCP936","UCS2
  cleanSerialGSM();
  PRINTLNF("waiting for SIM ready");
  loadingAnimation(500,10); //5 sec waiting if init first time
  PRINTLNF("");
  PRINTLNF("init GSM finished");
  cleanSerialGSM();
}

void requestTime() //request time from GSM
{
  PRINTLNF("request time");
  if (GSMwaitReqFlag || GSMonAirFlag) {
    PRINTINFO("GSM is busy, waiting");
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

void requestSignalAndRAM() //request signal quality from GSM
{
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

void requestBalance() //request balance from GSM
{
  PRINTLNF("request balance");
  if (GSMwaitReqFlag || GSMonAirFlag) {
    PRINTINFO("GSM is busy, waiting");
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

void readUntilOK(void* syncOKflag) //wait and read data response TODODOTODOTODO
{
  readStringGSM(); //PRINTLNHEX("after readStringGSM hex1=",(long)syncOKflag);
  bool sync_ok_flag = *(reinterpret_cast<bool (*)>(syncOKflag));
  if (sync_ok_flag || (readResponseCounter >= readResponseNum) ) { // max <readResponseNum> times to wait
    GSMwaitReqFlag = false;
    return;
  }
  PRINTINFO("waiting");
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
      loadingAnimation(500);
  }
}

void readStringGSM() //read data from GSM module
{
  memset(GSMstring,0,sizeof(GSMstring));
  GSMstring[0] = '\0';
  if (gsmSerial.available()) {
    if (gsmSerial.readBytesUntil('\n',GSMstring,sizeof(GSMstring)) < 4 ) { // \n  line feed - new line 0x0a 
//      PRINTLN("GSMstring=", GSMstring);
      return;
    }
    else if (strstr_P(GSMstring, PSTR("RING")) != NULL) //return ring signal (if somebody ringing)
    {
      gsmSerial.println(F("AT+CLCC")); //returns list of current call numbers
      backlightON();
      PRINTLNF("ringin!");
    }
    else if (strstr_P(GSMstring, PSTR("+CSQ: ")) != NULL) //return signal quality, command like +CSQ: 22,99 (if received signal quality data)
    {
      strncpy(GSMstring,strstr_P(GSMstring, PSTR("+CSQ: "))+6,2);
      GSMstring[2] ='\0';// must return like '22'
      signalStrength = atoi(GSMstring) * 100 / 31; // convert to percent, didnt need to check number, if needed, use sscanf or strtol
      signalSyncOK = true; //maybe return data
      PRINTLN("signal strength=", signalStrength);
    }
    else if (strstr_P(GSMstring, PSTR("+CCLK: ")) != NULL) //return time, command like +CCLK: "18/11/29,07:34:36+05" (if received time data), also GSM return this line on init +CTZV:17/07/28,19:03:51,+06 but we didnt process it
    {
      strncpy(GSMstring,strstr_P(GSMstring, PSTR("+CCLK: "))+8,20); 
      GSMstring[20] ='\0'; //must return 18/11/29,07:34:36+05\0     20/01/26,04:59:38+05
      PRINTLN("time string=", GSMstring);// Day, Month, Year, Hour, Minute, Second, timeZone
      uint8_t i = 0;
      static uint8_t parse_time_arr[7];// Day, Month, Year, Hour, Minute, Second, timeZone, set zeros
      char *search_p = strtok_P(GSMstring, PSTR("/,:+"));
      memset(parse_time_arr, 0, sizeof(parse_time_arr)); //set zeros
      while (search_p != NULL)
      {
        parse_time_arr[i++] = atoi(search_p);
        search_p = strtok_P(NULL, PSTR("/,:+"));
      }
      if (parse_time_arr[0] < 20) { //year is < 2020 - wrong datetime!
        PRINTLNF("sync clock failed!");
        return;
      }
      setTime(parse_time_arr[3], parse_time_arr[4], parse_time_arr[5], parse_time_arr[2], parse_time_arr[1], parse_time_arr[0]); //set time and date  setTime(int hr,int min,int sec,int dy, int mnth, int yr)
      adjustTime(parse_time_arr[6] * SECS_PER_HOUR);                                                                             //set timezone
      timeSyncOK = true;
      PRINTLNF("sync clock OK");
    }
    else if (strstr_P(GSMstring, PSTR("+CUSD: ")) != NULL) //return USSD balance command like +CUSD: 2, "⸮!5H}.A⸮Z⸮⸮⸮⸮." ,1  (if received balance data)
    {                                                     //                                  +CUSD: 2, "OCTATOK 151.8 p." ,1
      strncpy(GSMstring,strstr_P(GSMstring, PSTR("+CUSD: "))+11,64); //cut string will be like OCTATOK 151.8 p." ,1
      decode7Bit(GSMstring, sizeof(GSMstring));
      if (sscanf(GSMstring,"%*[^-0123456789]%d",&balance) == 1){  //find int
        PRINTLN("balance=",balance);
        balanceSyncOK = true; // PRINTLNHEX("balanceSyncOK hex=",(long)&balanceSyncOK);
      }
      else {
        PRINTLNF("check balance error!"); 
        balanceSyncOK = false;
      }
    }
    else if ((strstr_P(GSMstring, PSTR("+CLCC")) != NULL) && !GSMonAirFlag) { //if received phone number calling data
      PRINTLNF("received calling ID");
      if (checkNumber(GSMstring)) //check phone number +CLCC:
      {
        GSMonAirFlag = true;
  
        relayFlag = !relayFlag;
        digitalWrite(RELAY_PIN, relayFlag);
        PRINTLNF("relay switch!");
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

void decode7Bit(char *in_str, uint8_t dataSize) //decode USSD 7bit response
{
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

bool checkNumber(const char * string_number) //check phone number +CLCC:
{
  static const uint8_t list_size = sizeof(phone_table) / sizeof(phone_table[0]); //size of phones list
  static char phone_buff[12];
  for (int i = 0; i < list_size; i++)                            //4 phone numbers
  {
    strcpy_P(phone_buff, (char *)pgm_read_word(&(phone_table[i]))); // Necessary casts and dereferencing, just copy.
    if (strstr(string_number, phone_buff) != NULL)
    {
      PRINTLN("calling number=", phone_buff);
      return true;
    }
  }
  return false;
}

void cleanSerialGSM()
{
  while (gsmSerial.available()) //clean UART buffer
    gsmSerial.read();
}

void hangUpGSM()
{
  cleanSerialGSM();
  gsmSerial.println(F("AT+CHUP")); //hang up all calls
  GSMonAirFlag = false;
  GSMwaitReqFlag = false;
  
}

void backlightOFF()
{
  if (backlightFlag)
  {
    PRINTLNF("switch backlight off");
    backlightFlag = false;
    lcd.setBacklight(backlightFlag);
    updateMainScreenFlag = true;
    clearMainSreenFlag = true;
  }
}

void backlightON()
{
  timer.restartTimer(backlightTimerID); //restart backlight timer, ID 2
  if (!backlightFlag)
  {
    PRINTLNF("switch backlight on");
    backlightFlag = true;
    lcd.setBacklight(backlightFlag);
    updateMainScreenFlag = true;
    clearMainSreenFlag = true;
  }
}

void drawMainSreen()
{
  static char two_digits_buff[4];

  if (!updateMainScreenFlag)
    return;
  if (clearMainSreenFlag)
  {
    lcd.clear();
    clearMainSreenFlag = false;
  }
  currentMenu = 0;
  updateMainScreenFlag = false;
  
  ////lcd.createChar(3, memory);
//  lcd.createChar(5, celsius);
//  lcd.createChar(6, home);
//  lcd.createChar(7, heater);

  //relay state show
  lcd.setCursor(0, 0);
  relayFlag ? lcd.print(F("BK\247")) : lcd.print(F("B\256K\247")); //ВКЛ ВЫКЛ
  
  //time show
  lcd.setCursor(5, 0);
  sprintf(two_digits_buff, strcpy_P(string_buff, PSTR("%02d")), hour());
  lcd.print(two_digits_buff);
  ((millis() / 1000) % 2) ? lcd.write(':') : lcd.write(' ');
  sprintf(two_digits_buff, string_buff, minute());
  lcd.print(two_digits_buff);
  
  for (uint8_t i = 0; i<=1; i++) {
    lcd.setCursor(12, i);
    lcd.write(6+i); //6 home sign, 7 heater sign
    if (temp[i]==-99) {
      lcd.print(strcpy_P(two_digits_buff, PSTR("--"))); //temp is invalid show --
    }
    else { 
      sprintf(two_digits_buff, strcpy_P(string_buff, PSTR("%+03d")), temp[i]);
      lcd.print(two_digits_buff);
    }
    lcd.write(5); //celsius
  }
    
  //balance show
  if (!GSMinitOK) {
    lcd.setCursor(0, 1);
    lcd.print(F("O\254\245\240KA GSM"));
    return;
  }
//  lcd.createChar(3, ruble);
//  lcd.createChar(4, gsm);
  lcd.setCursor(1, 1);
  if (balance==-32768) {
    lcd.print(strcpy_P(two_digits_buff, PSTR("--")));
  }
  else {
    sprintf(two_digits_buff, strcpy_P(string_buff, PSTR("%02d")), balance);
    lcd.print(two_digits_buff);
  }
  lcd.write(3); // russian currency sign
  lcd.setCursor(6, 1);
//  lcd.setCursor(12, 1);
  lcd.write(4); //GSM sign
  if (signalStrength==0) {
    lcd.print(strcpy_P(two_digits_buff, PSTR("--")));
  }
  else {  
    sprintf(two_digits_buff, string_buff, signalStrength);
    lcd.print(two_digits_buff);
  }
  lcd.write('%');

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
  PRINTLN("elapsedDays ",elapsed_days);
  PRINTLN("previous rec time ",log_data.unix_time);
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
    PRINTLN("write_byte_pos ",write_byte_pos);
    PRINTLN("sentinel ",sentinel);
    EEPROM.put(write_byte_pos, log_data );
    PRINTLNF("Writing to EEPROM done!");
    
    //send SMS
    gsmSerial.println("AT+CMGF=1"); // Configuring TEXT mode
    delay(100);
    gsmSerial.print(F("AT+CMGS=+")); //sms to phone number 
    char phone_date_buff[16]; //number 7XXXXXXXXXX
    strcpy_P(phone_date_buff, (char *)pgm_read_word(&(phone_table[0]))); // first phone number from table
    gsmSerial.println(phone_date_buff);   //   "AT+CMGS=\"+79227754426\""
    delay(100);
    gsmSerial.print(F("Balance: "));
    gsmSerial.print(balance);
    gsmSerial.print(F(" rub.; Home temp: "));
    if (temp[0]==-99) {
      gsmSerial.print(strcpy_P(string_buff, PSTR("--")));
    }
    else {
      sprintf(GSMstring, strcpy_P(string_buff, PSTR("%+03d")), temp[0]);
      gsmSerial.print(GSMstring);  
    }
    gsmSerial.print(F("; Heat temp: "));
    if (temp[1]==-99) {
      gsmSerial.print(strcpy_P(string_buff, PSTR("--")));
    }
    else {
      sprintf(GSMstring, strcpy_P(string_buff, PSTR("%+03d")), temp[1]);
      gsmSerial.print(GSMstring);  
    }
    gsmSerial.print(F("; Date: "));//print date and time like 17.12.2020 16:47 // sprintf(GSMstring, "Balance: %d; Home temp: %+03d; Heat temp: %+03d; Date: %02d.%02d.%04d %02d:%02d", balance,temp[0],temp[1],day(),month(),year(),hour(),minute());
    sprintf(GSMstring, strcpy_P(string_buff, PSTR("%02d")), day());
    gsmSerial.print(GSMstring);
    gsmSerial.print('.');
    sprintf(GSMstring, strcpy_P(string_buff, PSTR("%02d")), month());
    gsmSerial.print(GSMstring);
    gsmSerial.print('.');
    sprintf(GSMstring, strcpy_P(string_buff, PSTR("%04d")), year());
    gsmSerial.print(GSMstring);
    gsmSerial.print(' ');
    sprintf(GSMstring, strcpy_P(string_buff, PSTR("%02d")), hour());
    gsmSerial.print(GSMstring);
    gsmSerial.print(':');
    sprintf(GSMstring, strcpy_P(string_buff, PSTR("%02d")), minute());
    gsmSerial.print(GSMstring);
    gsmSerial.write(26); //Ctr+Z - end of SMS
    delay(100);
    gsmSerial.println("AT+CMGF=0"); // Configuring TEXT mode
    PRINTLNF("Sending SMS done");
    timer.setTimeout(SECS_PER_MIN * 5000L, requestBalance);//request balance 5 min later
  }
    GSMwaitReqFlag = false;
}

void setup()
{
  #ifdef DEBUGGING
  Serial.begin(9600);
  PRINTLNF("Debug on");
  #endif
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayFlag); //relay OFF

  initMenu();
  // lcd.init(); //init display
  // lcd.createChar(3, ruble);
  // lcd.createChar(4, gsm);
  // lcd.createChar(5, celsius);
  // lcd.createChar(6, home);
  // lcd.createChar(7, heater);
  // lcd.setBacklight(backlightFlag);
  // lcd.clear();
  // lcd.setCursor(1, 0);
  // lcd.print(F("GSM-pe\273e v.")); //GSM-реле v.
  // lcd.setCursor(12, 0);
  // lcd.print(F("0."));
  // lcd.print(PROG_VERSION);
  // lcd.setCursor(3, 1); // display loading message
  // lcd.print(F("\267a\264py\267\272a     ")); //загрузка
  
  initMP3(); //mp3 serial port by default is not listening
  initGSM(); //init GSM module, the last intialized port is listening
  initDS();  //init DS temp modules
  requestTemp();  //request temp

//   menu_system.set_focusPosition(Position::LEFT); //init menu system
//   main_line1.attach_function(1, go_switch_relay);
//   main_line2.attach_function(1, func);
//   //main_line3.attach_function(1, func);
//   main_line4.attach_function(1, go_info_menu);
//   main_line5.attach_function(1, go_main_screen);

//  temp_line1.attach_function(1, func);
//  temp_line2.attach_function(1, func);
//  temp_line3.attach_function(1, func);
//  temp_line4.attach_function(1, func);
//  temp_line5.attach_function(1, go_main_screen);
  
//   info_line1.attach_function(1, func);
//   info_line2.attach_function(1, func);
//   info_line3.attach_function(1, func);
//   info_line4.attach_function(1, go_main_menu);
//   // info_line5.attach_function(1, go_main_menu);

//   main_line1.set_asProgmem(1); //set PROGMEM menu lines
//   main_line2.set_asProgmem(1);
//   //main_line3.set_asProgmem(1);
//   main_line4.set_asProgmem(1);
//   main_line5.set_asProgmem(1);

//  temp_line1.set_asProgmem(1);
//  temp_line2.set_asProgmem(1);
//  temp_line3.set_asProgmem(1);
//  temp_line4.set_asProgmem(1);
//  temp_line5.set_asProgmem(1);

//   info_line1.set_asProgmem(1);
//   info_line2.set_asProgmem(1);
//   info_line3.set_asProgmem(1);
//   info_line4.set_asProgmem(1);
//   // info_line5.set_asProgmem(1);

  timer.setInterval(1000, requestTemp); //request temp once a second and update screen
  backlightTimerID = timer.setInterval(SECS_PER_MIN * 10000L, backlightOFF); //auto backlight off 10 mins
  
  lcd.clear();
  if (!GSMinitOK) {
    return;
  }
    
  requestBalance();  //request balance from GSM
  waitAndReadUntilOK(balanceSyncOK); //read data response
  requestSignalAndRAM();  //request signal quality from GSM
  waitAndReadUntilOK(signalSyncOK); //read data response
  requestTime(); //request time from GSM
  waitAndReadUntilOK(timeSyncOK); //read data response
  readDSresponse(); //read temp data response
  sendSMSBalance(); //check if its needed to send SMS and then send

  timer.setInterval(10000L, requestSignalAndRAM);        //request signal quality every 10 secs
  timer.setInterval(SECS_PER_HOUR * 6000L, requestTime); //sync time every 6 hours
  timer.setInterval(SECS_PER_DAY * 80L, sendSMSBalance); //send sms with balance every 80 days
  timer.setInterval(SECS_PER_HOUR * 6000L, requestBalance);//request balance every 6 hours

  readStringGSM();
  lcd.clear();
  drawMainSreen();

  gsmSerial.setTimeout(100); // sets the maximum number of milliseconds to wait for readString()
}
void loop()
{
  timer.run();
  readButton();
  readEncoder();
  drawMainSreen();
  readStringGSM();
}
