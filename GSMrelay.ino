//use DFPlayerMini_Fast, 
//add shedule and temp automation, 
//add preferences menu section, 
//add voice temp information, add read BUSY pin +
//!!!!edit LiquidMenu_config.h first!!!!

#include <MemoryFree.h> //https://github.com/maniacbug/MemoryFree TODO

#define DEBUGGING
#ifdef DEBUGGING
// #include <MemoryFree.h> 
#undef PRINTLNF
#undef PRINTLN
#undef PRINTLNHEX
#undef PRINT
#define PRINTLNF(s)       \
  {                       \
    Serial.println(F(s)); \
  }
#define PRINTLN(s, v)   \
  {                     \
    Serial.print(F(s)); \
    Serial.println(v);  \
  }
#define PRINTLNHEX(s, v)    \
  {                         \
    Serial.print(F(s));     \
    Serial.println(v, HEX); \
  }
#define PRINT(s, v)     \
  {                     \
    Serial.print(F(s)); \
    Serial.print(v);    \
  }
#else
#undef PRINTLNF
#undef PRINTLN
#undef PRINTLNHEX
#undef PRINT
#define PRINTLNF(s)
#define PRINTLN(s, v)
#define PRINTLNHEX(s, v)
#define PRINT(s, v)
#endif

const char PROG_VERSION = '2';
const char PHOME_NUM1[] PROGMEM = {"79227754426"};
const char PHOME_NUM2[] PROGMEM = {"79044719617"};                                                   //pap
const char PHOME_NUM3[] PROGMEM = {"79822219033"};                                                   //pap
const char PHOME_NUM4[] PROGMEM = {"79226505965"};                                                   //tat
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
const uint8_t MP3_BUSY = 7; 
const char *const phone_table[] PROGMEM = {PHOME_NUM1, PHOME_NUM2, PHOME_NUM3, PHOME_NUM4}; // phones table

#include <CircularBuffer.h> // https://github.com/rlogiacco/CircularBuffer //TODO del
#include <OneWire.h>             //for DS18B20
#include <DFRobotDFPlayerMini.h> //DF MP3 Player mini
//#include <DFPlayerMini_Fast.h> //DFPlayer MP3 mini https://github.com/scottpav/DFPlayerMini_Fast
#include <ClickEncoder.h>   //encoder with button https://github.com/0xPIT/encoder
#include <TimeLib.h>        //timekeeping https://github.com/PaulStoffregen/Time
#include <SoftwareSerial.h> //for GSM modem A6
#include <SimpleTimer.h>    // Handy timers with callback parameters and polling https://github.com/marcelloromani/Arduino-SimpleTimer
#include <LiquidMenu.h>     //The menu wrapper library https://github.com/microdimmer/LiquidMenu and https://github.com/johnrickman/LiquidCrystal_I2C
#include <EEPROM.h>

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
int16_t last_val, enc_val;

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
int8_t gsm_signal = 0;
bool gsm_busy = false; //calling flag
bool is_busy = false; // waiting request command flag
unsigned long lastRequestTime = 0;
const uint32_t requestTimeDelay = 500; // separate GSM commands, time between command TODO min time 4 sec
uint8_t readResponseCounter = 0; // count response commands to run, max 20 times x 500ms

uint16_t memory_free = 0;
int16_t balance = -32768; //min uint
CircularBuffer<uint8_t,6> audio_queue;     // audio sequence size, can play five files continuously
bool relayFlag = false; //TODO заменить на битовые
bool backlightFlag = true;
bool updateMainScreen = true;
bool clearMainSreen = false;
bool timeSyncOK = false;
bool signalSyncOK = false;
bool balanceSyncOK = false;
uint8_t current_menu = 0; //0 - homepage, 1 - main menu, 2 - info menu, 3 - temp menu

const char heater[] PROGMEM = "\xF2\xE9\xF2\xE9\xE0\xF5\xFF\xFF"; //{0xF2, 0xE9, 0xF2, 0xE9, 0xE0, 0xF5, 0xFF,  0xFF}; heater sign
const char home[] PROGMEM = "\xE4\xEE\xFF\xFF\xF1\xF5\xF1\xFF"; //{0xE4, 0xEE, 0xFF, 0xFF, 0xF1, 0xF5, 0xF1, 0xFF};
const char celsius[] PROGMEM = "\x18\x18\x00\x07\x05\x04\x05\x07"; //{0x18, 0x18, 0x00, 0x07, 0x05, 0x04, 0x05, 0x07};
const char gsm[] PROGMEM = "\x01\x03\x1F\x11\x1F\x15\x1B\x1F"; //{0x01, 0x03, 0x1F, 0x11, 0x1F, 0x15, 0x1B, 0x1F};
const char memory[] PROGMEM = "\x0A\x1F\x0A\x1F\x0A\x1F\x0A\x00"; //{0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00};
const char ruble[] PROGMEM = "\x0E\x09\x09\x1E\x08\x1C\x08\x08"; //{ 0x0E,  0x09,  0x09,  0x1E,  0x08,  0x1C,  0x08, 0x08};

const char MENU_ON_OFF[] PROGMEM = {"B\272\273/B\303k\273"}; //Вкл/Выкл
//const char MENU_TEMP[] PROGMEM = {"Уст.темп"};       //TODO
//const char MENU_SCHEDULE[] PROGMEM = {"Расписание"}; //TODO
//const char MENU_PREFS[] PROGMEM = {"Настройки"};     //TODO
const char MENU_INFO[] PROGMEM = {"\245\275\344o"}; //Инфо
const char MENU_EXIT[] PROGMEM = {"B\303xo\343"}; //Выход

//const char MENU_TEMP_SET_HOME[] PROGMEM = {"Уст.т возд"};
//const char MENU_TEMP_SET_RADIATOR[] PROGMEM = {"Уст.т рад"};
//const char MENU_TEMP_HOME_HYSTERESIS[] PROGMEM = {"Гист возд."};
//const char MENU_TEMP_RADIATOR_HYSTERESIS[] PROGMEM = {"Гист рад."};

const char MENU_INFO_HOME[] PROGMEM = {"\340o\274      %+03d°C"}; //Дом
const char MENU_INFO_HEATER[] PROGMEM = {"Pa\343\270a\277op %+03d°C"}; //Радиатор
const char MENU_INFO_GSM[] PROGMEM = {"GSM c\270\264\275a\273  %02d%%"}; //GSM сигнал
//const char MENU_INFO_RAM[] PROGMEM = {"Память %03d%b"};

LiquidLine main_line1(1, 0, MENU_ON_OFF);
//LiquidLine main_line3(1, 1, MENU_TEMP);
//LiquidLine main_line2(1, 0, MENU_SCHEDULE);
LiquidLine main_line4(1, 1, MENU_INFO);
LiquidLine main_line5(1, 0, MENU_EXIT);

//LiquidScreen main_screen1(main_line1, main_line2);
//LiquidScreen main_screen2(main_line3, main_line4);
LiquidScreen main_screen1(main_line1, main_line4);
LiquidScreen main_screen3(main_line5);

//LiquidLine temp_line1(1, 0, MENU_TEMP_SET_HOME);
//LiquidLine temp_line3(1, 1, MENU_TEMP_SET_RADIATOR);
//LiquidLine temp_line2(1, 0, MENU_TEMP_HOME_HYSTERESIS);
//LiquidLine temp_line4(1, 1, MENU_TEMP_RADIATOR_HYSTERESIS);
//LiquidLine temp_line5(1, 0, MENU_EXIT);
//
//LiquidScreen temp_screen1(temp_line1, temp_line2);
//LiquidScreen temp_screen2(temp_line3, temp_line4);
//LiquidScreen temp_screen3(temp_line5);

LiquidLine info_line1(1, 0, MENU_INFO_HOME, temp[0]);
LiquidLine info_line2(1, 1, MENU_INFO_HEATER, temp[1]);
LiquidLine info_line3(1, 0, MENU_INFO_GSM, gsm_signal);
//LiquidLine info_line4(1, 1, MENU_INFO_RAM, memory_free);
LiquidLine info_line4(1, 1, MENU_EXIT);
LiquidScreen info_screen1(info_line1, info_line2);
LiquidScreen info_screen2(info_line3, info_line4);
// LiquidScreen info_screen3(info_line5);

LiquidMenu main_menu(lcd, main_screen1, main_screen3, 1);
//LiquidMenu temp_menu(lcd, temp_screen1, temp_screen2, temp_screen3, 1);
LiquidMenu info_menu(lcd, info_screen1, info_screen2, 1);

LiquidSystem menu_system(main_menu, info_menu, 1);
//LiquidSystem menu_system(main_menu, info_menu, temp_menu, 1);

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
  updateMainScreen = true;
  clearMainSreen = true;
  current_menu = 0;
}

void go_info_menu()
{
  PRINTLNF("changing to info menu!");
  menu_system.change_menu(info_menu);
  menu_system.change_screen(1);
  menu_system.switch_focus();
  current_menu = 2;
}

//void go_temp_menu()
//{
//  PRINTLNF("changing to temp menu!");
//  menu_system.change_menu(temp_menu);
//  menu_system.change_screen(1);
//  menu_system.switch_focus();
//  current_menu = 3;
//}

void go_main_menu()
{
  PRINTLNF("changing to main menu!");
  menu_system.switch_focus();
  menu_system.change_menu(main_menu);
  current_menu = 1;
}

void go_main_screen()
{
  PRINTLNF("changing to main screen!");
  menu_system.switch_focus();
  updateMainScreen = true;
  clearMainSreen = true;
  current_menu = 0;
}

void readEncoder()
{
  encoder.service();
  enc_val += encoder.getValue(); //read encoder
  if (enc_val == last_val)
    return;
  backlightON();
  if (current_menu == 0)
  {
    last_val = enc_val; //reset encoder values
    return;
  }
  if ((max(enc_val, last_val) - min(enc_val, last_val)) >= 2)
  { //if difference more than 2 -> one turn
    if (last_val > enc_val)
    {
      PRINTLNF("enc ++");
      if (!menu_system.switch_focus())
      { // end of menu lines
        menu_system++;
        menu_system.switch_focus();
      }
    }
    else
    {
      PRINTLNF("enc --");
      if (!menu_system.switch_focus(false))
      { // end of menu lines
        menu_system--;
        menu_system.switch_focus(false);
      }
    }
    last_val = enc_val;
    PRINTLNF("___");
  }
}

void readButton()
{
  encoder.service();
  button = encoder.getButton();
  if (button != ClickEncoder::Open)
  {
    PRINTLNF("button clicked");
    backlightON();
    if (current_menu == 0) //homepage
    {                      //go to main menu
      menu_system.change_menu(main_menu);
      menu_system.change_screen(1);
      menu_system.switch_focus();
      updateMainScreen = false;
      current_menu = 1; //change to main menu
    }
    else
    { // go to attached to menu function
      menu_system.call_function(1);
    }
  }
}

void requestTemp() //send request to temp sensors
{
  ds.reset();
  ds.write(0xCC, 0); // skip address (broadcast to all devices)
  ds.write(0x44, 0); // start conversion (start temp measurement)

  timer.setTimeout(200L, readDSresponse);
}

void readDSresponse() //read response from sensor
{
  static uint8_t buf[9];
  for (uint8_t i = 0; i < 2; i++)
  {
    ds.reset();
    ds.select(DSaddr[i]);
    ds.write(0xBE, 0); // read data from DS
    
    ds.read_bytes(buf, 9);
    if (OneWire::crc8(buf, 8) == buf[8] )  // check CRC
      temp[i] = ((int)buf[0] | (((int)buf[1]) << 8)) * 0.0625 + 0.03125; //first and second byte read, convert to int
    else 
      temp[i] = -99; // CRC is not valid
    //temp[i] = ((int)ds.read() | (((int)ds.read()) << 8)) * 0.0625 + 0.03125; //first and second byte read, convert to int, TODO add CRC check
  }

  if (current_menu == 0) //if main screen is displayed,
    updateMainScreen = true;
}

void loadingAnimation(uint32_t a_delay, uint8_t count = 1) //loading animation TODO
{
  for (uint8_t i = 0; i < count; i++)
  {
    PRINTLN("wait ", a_delay);
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
  gsmSerial.begin(115200);

  pinMode(GSM_PIN, OUTPUT);
  digitalWrite(GSM_PIN, LOW);
  lcd.setCursor(3, 1); // display loading message
  lcd.print(F("\267a\264py\267\272a     ")); // clear display //загрузка
  lcd.setCursor(11, 1);
  loadingAnimation(500,6);
  digitalWrite(GSM_PIN, HIGH);
  loadingAnimation(500,2);
  
  PRINTLNF("trying to set 9600");
  for (size_t i = 0; i < 20; i++)
  {
    gsmSerial.println(F("AT+IPR=9600")); //trying to set 9600
    delay(50);
  }
  gsmSerial.begin(9600);
  
  while (true)
  {
    gsmSerial.println(F("AT+CPAS"));
    if (gsmSerial.find(strcpy_P(string_buff, PSTR("+CPAS:0")))) //copy PROGMEM to buff and find answer in GSM serial 
      break;
    loadingAnimation(500);// display loading animation, joke about traktorista
    PRINTLNF("initialize GSM...");
  }
  gsmSerial.println(F("ATE0")); //echo off
  loadingAnimation(500);
  gsmSerial.println(F("AT+CSCS=\"GSM\"")); // "GSM","HEX","PCCP936","UCS2
  loadingAnimation(500);
  gsmSerial.println(F("AT+CMGF=1")); // Configuring TEXT mode
  loadingAnimation(500);
  cleanSerialGSM();
  PRINTLNF("init GSM OK");
}

bool isGSMBusy() //separate GSM commands
{
  if ((millis() - lastRequestTime) < requestTimeDelay) {
    timer.setTimeout(requestTimeDelay, requestTime);
    return true;
  }
  lastRequestTime = millis();
  return false;
}

void requestTime() //request time from GSM
{
  timeSyncOK = false;
  if (isGSMBusy())
    return;
  gsmSerial.println(F("AT+CCLK?"));
  PRINTLNF("request time from GSM");
  //timer.setTimeout(200L, read);
}

void requestTime2() //request time from GSM
{
  if (is_busy) {
    timer.setTimeout(500L,  requestTime2);
    return;  
  }
  else {
    is_busy = true;
    timeSyncOK = false;
    readResponseCounter = 0;
    gsmSerial.println(F("AT+CCLK?"));
    PRINTLNF("request time from GSM");
    timer.setTimeout(500L,  readUntilOK2, (void*) timeSyncOK );
  }
}


void readUntilOK2(void* syncOKflag) //wait and read data response TODODOTODOTODO
{
  // bool syncOK = (bool) syncOKflag;
  readStringGSM();
  if ((bool) syncOKflag || readResponseCounter >= 20) {
    is_busy = false;
    return;
  }
  PRINTLNF("waiting answer from GSM");
  timer.setTimeout(500L,  readUntilOK2, syncOKflag );
  readResponseCounter++;
}

void requestBalance() //request balance from GSM
{
  balanceSyncOK = false;
  if (isGSMBusy())
    return;
  gsmSerial.println(F("AT+CUSD=1,\"#105#\",15")); // test all
  PRINTLNF("request balance from GSM");
}

void requestSignalAndRAM() //request signal quality from GSM
{
  if (isGSMBusy())
    return;
  gsmSerial.println(F("AT+CSQ"));
  PRINTLNF("request signal quality from GSM");
  memory_free = freeMemory();
  PRINTLN("freeMemory()=", memory_free)
}

void readStringGSM() //read data from GSM module
{
  //static char GSMstring[64];
  memset(GSMstring,0,sizeof(GSMstring));
  GSMstring[0] = '\0';
//   while (gsmSerial.available())
//     Serial.write(gsmSerial.read()); //Forward what Software Serial received to Serial Port
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
      gsm_signal = atoi(GSMstring) * 100 / 31; // convert to percent, didnt need to check number, if needed, use sscanf or strtol
      signalSyncOK = true; //maybe return data
      PRINTLN("signal quality=", gsm_signal);
    }
    else if (strstr_P(GSMstring, PSTR("+CCLK: ")) != NULL) //return time, command like +CCLK: "18/11/29,07:34:36+05" (if received time data)
    {
      strncpy(GSMstring,strstr_P(GSMstring, PSTR("+CCLK: "))+8,20); 
      GSMstring[20] ='\0'; //must return 18/11/29,07:34:36+05\0
      PRINTLN("time string=", GSMstring);// Year, Month, Day, Hour, Minute, Second, timeZone
      uint8_t i = 0;
      static uint8_t parse_time_arr[7];// Year, Month, Day, Hour, Minute, Second, timeZone, set zeros
      char *search_p = strtok_P(GSMstring, PSTR("/,:+"));
      memset(parse_time_arr, 0, sizeof(parse_time_arr)); //set zeros
      while (search_p != NULL)
      {
        parse_time_arr[i++] = atoi(search_p);
        search_p = strtok_P(NULL, PSTR("/,:+"));
      }                    
      setTime(parse_time_arr[3], parse_time_arr[4], parse_time_arr[5], parse_time_arr[0], parse_time_arr[1], parse_time_arr[2]); //set time and date
      adjustTime(parse_time_arr[6] * SECS_PER_HOUR);                                                                             //set timezone
      timeSyncOK = true;
      PRINTLNF("sync clock OK");
    }
    else if (strstr_P(GSMstring, PSTR("+CUSD: ")) != NULL) //return USSD balance command like +CUSD: 2, "⸮!5H}.A⸮Z⸮⸮⸮⸮." ,1  (if received balance data)
    {                                                     //                                  +CUSD: 2, "OCTATOK 151.8 p." ,1
      strncpy(GSMstring,strstr_P(GSMstring, PSTR("+CUSD: "))+11,64); //cut string will be like OCTATOK 151.8 p." ,1
      decode7Bit(GSMstring);
      if (sscanf(GSMstring,"%*[^-0123456789]%d",&balance) == 1){  //find int
        PRINTLN("balance=",balance);
        balanceSyncOK = true;
      }
      else {
        PRINTLNF("check balance error!"); 
        balanceSyncOK = false;
      }
    }
    else if ((strstr_P(GSMstring, PSTR("+CLCC")) != NULL) && !gsm_busy) { //if received phone number calling data
      if (checkNumber(GSMstring)) //check phone number +CLCC:
      {
        gsm_busy = true;
        timer.setTimeout(15000L, hangUpGSM);
  
        relayFlag = !relayFlag;
        digitalWrite(RELAY_PIN, relayFlag);
        PRINTLNF("relay switch!");
  
        gsmSerial.println(F("ATA"));                       //answer call
        PRINTLNF("answer call");
        mp3Player.volume(30);                              //set volume value. From 0 to 30
        relayFlag ? addAudio(32) : addAudio(33); //play switch on/off
        playTemp(0); //play temp home
        playTemp(1); //play temp heater
        // gsmSerial.println(F("AT+CHUP"));
        if (current_menu == 0)
        {
          lcd.clear();
          updateMainScreen = true;
        }
        else
          menu_system.update();
  
        cleanSerialGSM();
      }
    }
    else
      PRINTLN("GSMstring=", GSMstring);
  }
}

void decode7Bit(char *in_str) //decode USSD 7bit response
{
  char out_str[64];
  byte reminder = 0;
  int bitstate = 7;
  for (byte i = 0; in_str[i] != '\0' || i <64; i++) {
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
  gsmSerial.println(F("AT+CHUP")); //hang up all calls
  gsm_busy = false;
  //cleanSerialGSM(); // ??
}

void initMP3()
{
  mp3Serial.begin(9600);
  if (mp3Player.begin(mp3Serial))
  { //Use softwareSerial to communicate with mp3.
    PRINTLNF("DFPlayer Mini init OK");
  }
  mp3Player.pause();
}

bool isMP3Busy() {
	return digitalRead(MP3_BUSY) == LOW;
}

bool addAudio(uint8_t num_track) {
  if (!audio_queue.isFull()) 
    return audio_queue.push(num_track);
  else { PRINTLNF("isFull"); 
    return false;}
}

void playAudio() {
  if (isMP3Busy() || audio_queue.isEmpty())
    return;

    mp3Player.play(audio_queue.shift());
}

void playTemp(uint8_t temp_sens_num) { // TODO
  if (temp[temp_sens_num]==-99) { // has no data
    return;
  }
  if (temp_sens_num==0) //play temp source (temp1 or temp2)
    addAudio(35); // home temp
  else
    addAudio(34); // heater temp
    
  if (temp[temp_sens_num]==0) {
    addAudio(28); // //play zero temp
    return;
  }
  bool sign = (temp[temp_sens_num]>0); 
  sign ? addAudio(30) : addAudio(29); //play sign
  uint8_t abs_temp = abs(temp[temp_sens_num]);
  if ((abs_temp > 0) && (abs_temp <= 20)) {
    addAudio(abs_temp); //play temp 1 - 20
  }
  else {
    for (uint8_t i = 2; i < 9; i++) //play temp 21 - 99
    {
      uint8_t first_digit = i*10;
      uint8_t last_digit = first_digit+10;
      if ( (abs_temp >= first_digit) && (abs_temp < last_digit)) {
        uint8_t digits = abs_temp - first_digit;
        addAudio(18+i);
        if (digits !=0 ) addAudio(digits);
      }
    }
  }
  addAudio(31);//play 'degree'
}

void initDS() //init temp sensors
{
  uint8_t i,resolution = 0;
  //byte resolution;
  while (ds.search(DSaddr[i]))
  {
    if (i > 1)
    {
      PRINTLNF("more than 2 devices, unable to load them all");
      break;
    }
    if (OneWire::crc8(DSaddr[i], 7) != DSaddr[i][7])
    {
      PRINTLNF("CRC of temp sensor is not valid!");
      break;
    }
    // PRINTLNF("read resolution");
    ds.reset(); //read resolution
    ds.select(DSaddr[i]);
    ds.write(0xBE); // Read Scratchpad
    for (uint8_t j = 0; j < 5; j++)
    {
      resolution = ds.read(); // we need fifth byte, (resolution) 7F=12bits 5F=11bits 3F=10bits 1F=9bits
    }
    PRINTLNHEX("DS18B20 resolution (0x7F max)=", resolution);
    if (resolution != 0x7f)
    {
      ds.reset();
      ds.select(DSaddr[i]);
      ds.write(0x4E);       // Write scratchpad command
      ds.write(0);          // TL data
      ds.write(0);          // TH data
      ds.write(0x7F);       // Configuration Register (resolution) 7F=12bits 5F=11bits 3F=10bits 1F=9bits
      ds.reset();           // This "reset" sequence is mandatory
      ds.select(DSaddr[i]); // it allows the DS18B20 to understand the copy scratchpad to EEPROM command
      ds.write(0x48);       // Copy Scratchpad command
    }
    i++;
  }
}

void backlightOFF()
{
  if (backlightFlag)
  {
    PRINTLNF("switch backlight off");
    backlightFlag = false;
    lcd.setBacklight(backlightFlag);
    updateMainScreen = true;
    clearMainSreen = true;
  }
}

void backlightON()
{
  timer.restartTimer(2); //restart backlight timer, ID 2
  if (!backlightFlag)
  {
    PRINTLNF("switch backlight on");
    backlightFlag = true;
    lcd.setBacklight(backlightFlag);
    updateMainScreen = true;
    clearMainSreen = true;
  }
}

void drawMainSreen()
{
  static char two_digits_buff[4];

  if (!updateMainScreen)
    return;
  if (clearMainSreen)
  {
    lcd.clear();
    clearMainSreen = false;
  }
  
  //lcd.createChar(3, memory);
  lcd.createChar(4, gsm);
  lcd.createChar(5, celsius);
  lcd.createChar(6, home);
  lcd.createChar(7, heater);
  lcd.createChar(3, ruble);
  
  //time show
  lcd.setCursor(0, 0);
  sprintf(two_digits_buff, strcpy_P(string_buff, PSTR("%02d")), hour());
  lcd.print(two_digits_buff);
  ((millis() / 1000) % 2) ? lcd.write(':') : lcd.write(' ');
  sprintf(two_digits_buff, string_buff, minute());
  lcd.print(two_digits_buff);
  
  //balance show
  lcd.setCursor(1, 1);
  if (balance==-32768) {
    lcd.print(strcpy_P(two_digits_buff, PSTR("--")));
  }
  else {
    sprintf(two_digits_buff, strcpy_P(string_buff, PSTR("%02d")), balance);
    lcd.print(two_digits_buff);
  }
  lcd.write(3); // russian currency sign

  lcd.setCursor(12, 1);
  lcd.write(4); //GSM sign
  sprintf(two_digits_buff, string_buff, gsm_signal);
  lcd.print(two_digits_buff);
  lcd.write('%');

  lcd.setCursor(12, 0);
  relayFlag ? lcd.print(F("BK\247")) : lcd.print(F("B\256K\247")); //ВКЛ ВЫКЛ

  for (uint8_t i = 0; i<=1; i++) {
    lcd.setCursor(6, i);
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

  current_menu = 0;
  updateMainScreen = false;
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
  if (isGSMBusy())
    return;
  //mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  //gsmSerial.println(F("AT+CHUP")); //hang up all calls// TODO

  uint16_t read_byte_pos = addrToRead(); //find EEPROM address to read from
  Log log_data;
  // byte sentinel = 0; //sentinel bit
  // if (read_byte_pos >= EEPROM.length()-sizeof(Log)) { //reach end of EEPROM go to begining (to 0), to the start of EEPROM
  //   read_byte_pos = 0;
  //   EEPROM.get(read_byte_pos, log_data); //read EEPROM data from start byte (0)
  //   sentinel = bitRead(log_data.home_temp,7); //read sentinel bit
  //   sentinel ^= 1; // invert sentintel bit
  // }
  // else {
  EEPROM.get(read_byte_pos, log_data); //read EEPROM data
  byte sentinel = bitRead(log_data.home_temp,7); //read sentinel bit
  // }
  PRINTLN("sentinel ",sentinel);
  time_t previous_datetime = log_data.unix_time;
  if (previous_datetime == 0xFFFFFFFF) {//check data, if empty date = empty data, nothing to read
    previous_datetime = 0; //no data - send SMS and write to EEPROM
    sentinel ^= 1 ; // invert sentintel bit
  }
  PRINTLN("read_byte_pos ",read_byte_pos);
  PRINTLN("sentinel ",sentinel);
//  return;
  if (elapsedDays(now()) - elapsedDays(previous_datetime) >= 80) { //if more than 80 days have passed from last sending, send new SMS
    //write datetitme (data struct) to EEPROM
    uint16_t write_byte_pos = read_byte_pos + sizeof(Log);
    if (write_byte_pos >= EEPROM.length()) { //reaching end of EEPROM go to begining (to 0), to the start of EEPROM
      write_byte_pos = 0;
      if (previous_datetime != 0xFFFFFFFF)
        sentinel ^= 1 ; //invert sentintel bit
    }
    log_data.home_temp = temp[0]; //home temp must be <= 0b00111111, i.e. abs(home_temp) <= 63
    bitWrite(log_data.home_temp,7,sentinel); //set last bit, its sentinel (0b10000000)
    log_data.heater_temp = temp[1];
    log_data.balance = balance;
    log_data.unix_time = now();   
    //EEPROM.put(write_byte_pos, log_data );
    PRINTLNF("Writing to EEPROM done!");
    
    //send SMS
//    gsmSerial.println(F("AT+CMGF=1")); // Configuring TEXT mode
//    loadingAnimation(500);
    gsmSerial.print(F("AT+CMGS=+")); //sms to phone number
    //char phone_date_buff[16]; //number 7XXXXXXXXXX
    GSMstring[0] = '\0'; //TODO number 7XXXXXXXXXX
    strcpy_P(GSMstring, (char *)pgm_read_word(&(phone_table[0]))); // first phone number from table
    gsmSerial.println(GSMstring);
    PRINTLN("phone=+",GSMstring);
    gsmSerial.print(F("Balance: "));
    gsmSerial.print(balance);
    gsmSerial.print(F("; Home temp: "));
    sprintf(GSMstring, strcpy_P(string_buff, PSTR("%+03d")), temp[0]);
    gsmSerial.print(GSMstring);
    gsmSerial.print(F("; Heat temp: "));
    sprintf(GSMstring, strcpy_P(string_buff, PSTR("%+03d")), temp[1]);
    gsmSerial.print(GSMstring);
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
    PRINTLNF("Sending SMS done!");
    requestBalance();
  }
}

void waitAndReadUntilOK(const bool &syncOK) { //wait and read data response
    for (uint8_t i = 0; i<20; i++) { 
      loadingAnimation(500);
      readStringGSM();
      if (syncOK)
        return;
  }
}

void waitAndReadUntilOK2(const bool &syncOK) { //wait and read data response
    for (uint8_t i = 0; i<21; i++) { 
      loadingAnimation(500);
      timer.run(); //running readUntilOK2
      if (syncOK)
        return;
  }
}

void setup()
{
  #ifdef DEBUGGING
  Serial.begin(9600);
  PRINTLNF("Debug on");
  #endif
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayFlag); //relay OFF

  lcd.init(); //init display
  lcd.setBacklight(backlightFlag);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(F("GSM-pe\273e v.")); //GSM-реле v.
  lcd.setCursor(12, 0);
  lcd.print(F("0."));
  lcd.print(PROG_VERSION);
  lcd.setCursor(3, 1);
  initDS();  //init DS temp modules
  requestTemp();  //request temp
  initMP3(); //mp3 serial port by default is not listening
  initGSM(); //init GSM module, the last intialized port is listening
  requestTime2(); //request time from GSM
  waitAndReadUntilOK2(timeSyncOK); //read data response
  requestSignalAndRAM();  //request signal quality from GSM
  waitAndReadUntilOK(signalSyncOK); //read data response
  requestBalance();  //request balance from GSM  
  waitAndReadUntilOK(balanceSyncOK); //read data response
  readDSresponse(); //read temp data response
  sendSMSBalance(); //check if its needed to send SMS and then send

  menu_system.set_focusPosition(Position::LEFT); //init menu system
  main_line1.attach_function(1, go_switch_relay);
  //main_line2.attach_function(1, func);
  //main_line3.attach_function(1, func);
  main_line4.attach_function(1, go_info_menu);
  main_line5.attach_function(1, go_main_screen);

//  temp_line1.attach_function(1, func);
//  temp_line2.attach_function(1, func);
//  temp_line3.attach_function(1, func);
//  temp_line4.attach_function(1, func);
//  temp_line5.attach_function(1, go_main_screen);
  
  info_line1.attach_function(1, func);
  info_line2.attach_function(1, func);
  info_line3.attach_function(1, func);
  info_line4.attach_function(1, go_main_menu);
  // info_line5.attach_function(1, go_main_menu);

  main_line1.set_asProgmem(1); //set PROGMEM menu lines
  //main_line2.set_asProgmem(1);
  //main_line3.set_asProgmem(1);
  main_line4.set_asProgmem(1);
  main_line5.set_asProgmem(1);

//  temp_line1.set_asProgmem(1);
//  temp_line2.set_asProgmem(1);
//  temp_line3.set_asProgmem(1);
//  temp_line4.set_asProgmem(1);
//  temp_line5.set_asProgmem(1);

  info_line1.set_asProgmem(1);
  info_line2.set_asProgmem(1);
  info_line3.set_asProgmem(1);
  info_line4.set_asProgmem(1);
  // info_line5.set_asProgmem(1);

  timer.setInterval(1000, requestTemp); //request temp once a second
  timer.setInterval(SECS_PER_MIN * 10000L, backlightOFF); //auto backlight off 10 mins
  timer.setInterval(10000L, requestSignalAndRAM);        //request signal quality every 10 secs
  timer.setInterval(SECS_PER_HOUR * 6000L, requestTime); //sync time every 6 hours
  timer.setInterval(SECS_PER_DAY * 80L, sendSMSBalance); //send sms with balance every 80 days
  timer.setInterval(SECS_PER_HOUR * 6000L, requestBalance);//request balance every 6 hours
  timer.setInterval(100L, playAudio); //100 ms delay

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
  //playAudio();
}
