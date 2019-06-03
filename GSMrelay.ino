//TODO remove all strings objects, 
//use DFPlayerMini_Fast, 
//add shedule and temp automation, 
//add preferences menu section, 
//add voice temp information, add read BUSY pin +
//add PROGMEM to all const strings
#include <MemoryFree.h> //https://github.com/maniacbug/MemoryFree
// #define DEBUGGING
#ifdef DEBUGGING
#include <MemoryFree.h> //https://github.com/maniacbug/MemoryFree
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
const uint8_t REL_PIN = 11;                                                                             //pin for Relay
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

#include <CircularBuffer.h> // https://github.com/rlogiacco/CircularBuffer
#include <OneWire.h>             //for DS18B20
#include <DFRobotDFPlayerMini.h> //DF MP3 Player mini
// #include <DFPlayerMini_Fast.h> //DFPlayer MP3 mini https://github.com/scottpav/DFPlayerMini_Fast
#include <ClickEncoder.h>   //encoder with button https://github.com/0xPIT/encoder
#include <TimeLib.h>        //timekeeping
#include <SoftwareSerial.h> //for GSM modem A6
#include <SimpleTimer.h>    // Handy timers
#include <LiquidMenu.h>     //The menu wrapper library https://github.com/microdimmer/LiquidMenu and https://github.com/ssilver2007/LCD_1602_RUS and https://github.com/johnrickman/LiquidCrystal_I2C

OneWire ds(TEMP_SENSOR_PIN);
byte DSaddr[2][8]; // first and second DS18B20 addresses, home 28FFE44750170473 heater 28FF2FDAC11704DE

ClickEncoder encoder(ENCODER_PIN1, ENCODER_PIN2, ENCODER_PIN3, 2); //changed to 2 at one turn
ClickEncoder::Button button;
int16_t last_val, enc_val;

SoftwareSerial gsmSerial(GSM_SERIAL_RX, GSM_SERIAL_TX);
SoftwareSerial mp3Serial(MP3_SERIAL_RX, MP3_SERIAL_TX);
// DFPlayerMini_Fast mp3Player;
DFRobotDFPlayerMini mp3Player;

SimpleTimer timer;

LCD_1602_RUS lcd(0x3F, 16, 2);

const char heater[] PROGMEM = {0xF2, 0xE9, 0xF2, 0xE9, 0xE0, 0xF5, 0xFF, 0xFF};
const char home[] PROGMEM = {0xE4, 0xEE, 0xFF, 0xFF, 0xF1, 0xF5, 0xF1, 0xFF};
const char celsius[] PROGMEM = {0x18, 0x18, 0x00, 0x07, 0x05, 0x04, 0x05, 0x07};
const char gsm[] PROGMEM = {0x01, 0x03, 0x1F, 0x11, 0x1F, 0x15, 0x1B, 0x1F};
const char memory[] PROGMEM = {0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00};

char string_buff[8]; //string buffer

int8_t temp[2] = {-99, -99}; // temp home, temp heater
// int8_t t_heater_set = 0;
// int8_t t_home_set = 0;
int8_t gsm_signal = 0;
bool gsm_busy = false;
uint16_t memory_free = 0;
CircularBuffer<uint8_t,5> audio_queue;     // audio sequence size, can play five files continuously
bool relayFlag = false;
bool backlightFlag = true;
bool updateMainScreen = true;
bool clearMainSreen = false;

const char MENU_ON_OFF[] PROGMEM = {"Вкл/Выкл"};
const char MENU_SCHEDULE[] PROGMEM = {"Расписание"}; //TODO
const char MENU_TEMP[] PROGMEM = {"Уст.темп"};       //TODO
const char MENU_PREFS[] PROGMEM = {"Настройки"};     //TODO
const char MENU_INFO[] PROGMEM = {"Инфо"};
const char MENU_EXIT[] PROGMEM = {"Выход"};

const char MENU_INFO_HOME[] PROGMEM = {"Дом      %+03d°C"};
const char MENU_INFO_HEATER[] PROGMEM = {"Радиатор %+03d°C"};
const char MENU_INFO_GSM[] PROGMEM = {"GSM сигнал  %02d%%"};
// const char MENU_INFO_RAM[] PROGMEM = {"Память %03d%b"};

uint8_t current_menu = 0; //0 - homepage, 1 - main menu, 2 - info menu
LiquidLine main_line1(1, 0, MENU_ON_OFF);
LiquidLine main_line2(1, 1, MENU_SCHEDULE);
LiquidLine main_line3(1, 0, MENU_TEMP);
LiquidLine main_line4(1, 1, MENU_INFO);
LiquidLine main_line5(1, 0, MENU_EXIT);

LiquidScreen main_screen1(main_line1, main_line2);
LiquidScreen main_screen2(main_line3, main_line4);
LiquidScreen main_screen3(main_line5);

LiquidLine info_line1(1, 0, MENU_INFO_HOME, temp[0]);
LiquidLine info_line2(1, 1, MENU_INFO_HEATER, temp[1]);
LiquidLine info_line3(1, 0, MENU_INFO_GSM, gsm_signal);
// LiquidLine info_line4(1, 1, MENU_INFO_RAM, memory_free);
LiquidLine info_line4(1, 1, MENU_EXIT);
LiquidScreen info_screen1(info_line1, info_line2);
LiquidScreen info_screen2(info_line3, info_line4);
// LiquidScreen info_screen3(info_line5);

LiquidMenu main_menu(lcd, main_screen1, main_screen2, main_screen3, 1);
LiquidMenu info_menu(lcd, info_screen1, info_screen2, 1);

LiquidSystem menu_system(main_menu, info_menu, 1);

void func() // Blank function, it is attached to the lines so that they become focusable.
{
  PRINTLNF("hello!");
}

void go_switch_relay()
{
  relayFlag = !relayFlag;
  digitalWrite(REL_PIN, relayFlag);
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

void initGSM()
{
  gsmSerial.begin(115200);

  pinMode(GSM_PIN, OUTPUT);
  digitalWrite(GSM_PIN, LOW);
  lcd.setCursor(3, 1); // display loading message
  lcd.print(F("загрузка     ")); // clear display 
  delay(2000);
  digitalWrite(GSM_PIN, HIGH);

  PRINTLNF("trying to set 9600");
  for (size_t i = 0; i < 4; i++)
  {
    gsmSerial.println(F("AT+IPR=9600")); //trying to set 9600
    delay(50);
  }
  gsmSerial.begin(9600);
  
  char loading_string[6] = {0};
  uint8_t i = 0;
  while (true)
  {
    gsmSerial.println(F("AT+CPAS"));
    if (gsmSerial.find(strcpy_P(string_buff, PSTR("+CPAS:0")))) //copy PROGMEM to buff and find answer in GSM serial 
      break;

    lcd.setCursor(11, 1); // display loading animation
    lcd.print(loading_string);
    loading_string[i]='.';
    i++;
    if (i == 6) {
      strcpy_P(loading_string, PSTR("     "));
      i = 0;
    }
    delay(500);
    PRINTLNF("initialize GSM...");
  }
  gsmSerial.println(F("ATE0")); //echo off
  cleanSerialGSM();
  PRINTLNF("init GSM OK");
}

void requestTime() //request time from GSM
{
  // cleanSerialGSM();
  gsmSerial.println(F("AT+CCLK?"));
  PRINTLNF("request time from GSM");
}

void requestSignalAndRAM() //request signal quality from GSM
{
  // cleanSerialGSM();
  gsmSerial.println(F("AT+CSQ"));
  memory_free = freeMemory();
#ifdef DEBUGGING
  PRINTLN("freeMemory()=", memory_free)
#endif
  // PRINTLNF("request signal quality from GSM");
}

void readStringGSM()
{
  String strBuffer;
  // while (gsmSerial.available())
  //   Serial.write(gsmSerial.read()); //Forward what Software Serial received to Serial Port

  while (gsmSerial.available())
  {
    //   // PRINTLN("GSM string=",gsmSerial.readStringUntil("\n"));
    //   // PRINTLN("GSM string=",gsmSerial.readStringUntil("\0"));
    //   // PRINTLN("GSM string=",gsmSerial.readStringUntil("\r"));
    // strBuffer = gsmSerial.readStringUntil("\r");
    strBuffer = gsmSerial.readStringUntil(0x0a); // \n	line feed - new line
    strBuffer.trim();
    // PRINTLN("strBuffer=", strBuffer);
    // PRINTLN("length()=", strBuffer.length());
    break;
    // while (gsmSerial.available())
    // {
    //   i = (gsmSerial.read());
    //   strBuffer += i;
    // }
  }
  if (strBuffer.length() < 4)
    return;
  else if ((strBuffer.indexOf(strcpy_P(string_buff, PSTR("RING"))) >= 0)) //return ring signal
  {
    gsmSerial.println(F("AT+CLCC")); //returns list of current call numbers
    backlightON();
    PRINTLNF("ringin!");
  }
  else if (strBuffer.indexOf(strcpy_P(string_buff, PSTR("+CSQ"))) >= 0) //return signal quality, command like +CSQ: 22,99
  {
    strBuffer = strBuffer.substring(strBuffer.indexOf(string_buff) + 6, 8); //return 22
    gsm_signal = strBuffer.toInt() * 100 / 31;
    PRINTLN("signal quality=", gsm_signal);
  }
  else if (strBuffer.indexOf(strcpy_P(string_buff, PSTR("+CCLK:"))) >= 0) //return time, command like +CCLK: "18/11/29,07:34:36+05"
  {
    strBuffer = strBuffer.substring(strBuffer.indexOf(string_buff) + 8, 28); //return 18/11/29,07:34:36+05
    PRINTLN("time string=", strBuffer);
    uint8_t parse_time_arr[7] = {0};                            // Year, Month, Day, Hour, Minute, Second, timeZone, set zeros
    // memset(parse_time_arr, 0, sizeof(parse_time_arr)); //set zeros
    for (uint8_t i = 0, str_index; i < 7; i++, str_index += 3)
    {
      parse_time_arr[i] = strBuffer.substring(str_index, str_index + 2).toInt(); // Year, Month, Day, Hour, Minute, Second, timeZone
      // if (parse_time_arr[i] == 0)
      // {
      //   PRINTLN("parse_time_arr[i]=", parse_time_arr[i]);
      //   PRINTLNF("sync clock fault!");
      //   return;
      // }
    }
    setTime(parse_time_arr[3], parse_time_arr[4], parse_time_arr[5], parse_time_arr[0], parse_time_arr[1], parse_time_arr[2]); //set time and date
    adjustTime(parse_time_arr[6] * SECS_PER_HOUR);                                                                             //set timezone
    PRINTLNF("sync clock OK");
  }
  else if ((strBuffer.indexOf(strcpy_P(string_buff, PSTR("+CLCC"))) >= 0) && !gsm_busy)
    if (checkNumber(strBuffer)) //check phone number +CLCC:
    {
      gsm_busy = true;
      timer.setTimeout(15000L, hangUpGSM);

      relayFlag = !relayFlag;
      digitalWrite(REL_PIN, relayFlag);
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

bool checkNumber(String &phone_string) //check phone number +CLCC:
{
  char buffer[12];
  byte list_size = sizeof(phone_table) / sizeof(phone_table[0]); //size of phones list
  for (int i = 0; i < list_size; i++)                            //4 phone numbers
  {
    strcpy_P(buffer, (char *)pgm_read_word(&(phone_table[i]))); // Necessary casts and dereferencing, just copy.
    if (phone_string.indexOf(buffer) >= 0)
    {
      PRINTLN("calling number=", buffer);
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
  mp3Player.begin(mp3Serial);

  // if (mp3Player.begin(mp3Serial))
  // { //Use softwareSerial to communicate with mp3.
  //   PRINTLNF("DFPlayer Mini init OK");
  // }
  // delay(5000);
  mp3Player.pause();
}

bool isMP3Busy() {
	return digitalRead(MP3_BUSY) == LOW;
}

bool addAudio(uint8_t num_track) {
  if (!audio_queue.isFull()) 
    return audio_queue.push(num_track);
  else
    return false;
}

void playAudio() {
  if ((audio_queue.isEmpty() || isMP3Busy())) {
    return;
  }
  else 
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

void initDS()
{
  uint8_t i = 0;
  byte resolution;
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
    PRINTLNHEX("DS18B20 resolution=", resolution);
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
  lcd.createChar(3, memory);
  lcd.createChar(4, gsm);
  lcd.createChar(5, celsius);
  lcd.createChar(6, home);
  lcd.createChar(7, heater);
  
  lcd.setCursor(0, 0);
  sprintf(two_digits_buff, strcpy_P(string_buff, PSTR("%02d")), hour());
  lcd.print(two_digits_buff);
  ((millis() / 1000) % 2) ? lcd.write(':') : lcd.write(' ');
  sprintf(two_digits_buff, string_buff, minute());
  lcd.print(two_digits_buff);

  lcd.setCursor(0, 1);
  lcd.write(3); //RAM sign
  sprintf(two_digits_buff, strcpy_P(string_buff, PSTR("%02d")), memory_free);
  lcd.print(two_digits_buff);

  lcd.setCursor(12, 1);
  lcd.write(4); //GSM sign
  sprintf(two_digits_buff, string_buff, gsm_signal);
  lcd.print(two_digits_buff);
  lcd.write('%');

  lcd.setCursor(12, 0);
  relayFlag ? lcd.print(F("ВКЛ")) : lcd.print(F("ВЫКЛ"));

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

void setup()
{
  pinMode(REL_PIN, OUTPUT);
  digitalWrite(REL_PIN, relayFlag); //relay OFF

  lcd.init(); //init display
  lcd.setBacklight(backlightFlag);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(F("GSM-реле v."));
  lcd.setCursor(12, 0);
  lcd.print(F("0."));
  lcd.print(PROG_VERSION);
  lcd.setCursor(3, 1);
  // lcd.print(F("загрузка..."));
#ifdef DEBUGGING
  Serial.begin(9600);
#endif

  initDS();  //init DS temp modules
  initMP3(); //mp3 serial port by default is not listening
  initGSM(); //init GSM module, the last intialized port is listening
  delay(1000); //delay to initialize SIM card
  requestTime(); //request time from GSM
  requestTemp();                                 //request temp
   delay(200);
  requestSignalAndRAM();                         //request signal quality from GSM
  
  menu_system.set_focusPosition(Position::LEFT); //init menu system
  main_line1.attach_function(1, go_switch_relay);
  main_line2.attach_function(1, func);
  main_line3.attach_function(1, func);
  main_line4.attach_function(1, go_info_menu);
  main_line5.attach_function(1, go_main_screen);

  info_line1.attach_function(1, func);
  info_line2.attach_function(1, func);
  info_line3.attach_function(1, func);
  info_line4.attach_function(1, go_main_menu);
  // info_line5.attach_function(1, go_main_menu);

  main_line1.set_asProgmem(1); //set PROGMEM menu lines
  main_line2.set_asProgmem(1);
  main_line3.set_asProgmem(1);
  main_line4.set_asProgmem(1);
  main_line5.set_asProgmem(1);

  info_line1.set_asProgmem(1);
  info_line2.set_asProgmem(1);
  info_line3.set_asProgmem(1);
  info_line4.set_asProgmem(1);
  // info_line5.set_asProgmem(1);

  timer.setInterval(1000, requestTemp);
  timer.setInterval(SECS_PER_MIN * 10000L, backlightOFF); //auto backlight off 10 mins
  timer.setInterval(SECS_PER_HOUR * 6000L, requestTime); //sync time every 6 hours
  timer.setInterval(10000L, requestSignalAndRAM);        //request signal quality every 10 secs

  readStringGSM();
  lcd.clear();
  drawMainSreen();

  gsmSerial.setTimeout(100); //set timeout for readString() func ??
}
void loop()
{
  timer.run();
  readButton();
  readEncoder();
  drawMainSreen();
  readStringGSM();
  playAudio();
}
