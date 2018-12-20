//TODO remove all strings objects, use DFPlayerMini_Fast, add shedule and temp automation, add voice temp information,add PROGMEM to all const
#define DEBUGGING
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
#define PRINT(s, v)     \
  {                     \
    Serial.print(F(s)); \
    Serial.print(v);    \
  }
#else
#define PRINTLNF(s)
#define PRINTLN(s, v)
#define PRINT(s, v)
#endif

const char PROG_VERSION = '2';
const char PHOME_NUMBER1[] PROGMEM = {"79227754426"};
const char PHOME_NUMBER2[] PROGMEM = {"79044719617"};                                                   //pap
const char PHOME_NUMBER3[] PROGMEM = {"79822219033"};                                                   //pap
const char PHOME_NUMBER4[] PROGMEM = {"79226505965"};                                                   //tat
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
const char *const phone_table[] PROGMEM = {PHOME_NUMBER1, PHOME_NUMBER2, PHOME_NUMBER3, PHOME_NUMBER4}; // phones table

#include <OneWire.h>             //for DS18B20
#include <DFRobotDFPlayerMini.h> //DF MP3 Player mini
// #include <DFPlayerMini_Fast.h> //DFPlayer MP3 mini https://github.com/scottpav/DFPlayerMini_Fast
#include <ClickEncoder.h>   //encoder with button https://github.com/0xPIT/encoder
#include <TimeLib.h>        //timekeeping
#include <SoftwareSerial.h> //for GSM modem A6
#include <SimpleTimer.h>    // Handy timers
#include <LiquidMenu.h>     //The menu wrapper library

OneWire ds(TEMP_SENSOR_PIN);
const byte DSaddr1[] = {0x28, 0xFF, 0xE4, 0x47, 0x50, 0x17, 0x04, 0x73}; // first DS18B20 address  //TODO add PROGMEM
const byte DSaddr2[] = {0x28, 0xFF, 0x2F, 0xDA, 0xC1, 0x17, 0x04, 0xDE}; // second DS18B20 address
// const byte *const DSaddr_table[] PROGMEM = {DSaddr1, DSaddr2};

ClickEncoder encoder(ENCODER_PIN1, ENCODER_PIN2, ENCODER_PIN3, 2); //changed to 2 at one turn
ClickEncoder::Button button;
int16_t last_val, enc_val;

SoftwareSerial gsmSerial(GSM_SERIAL_RX, GSM_SERIAL_TX);
SoftwareSerial mp3Serial(MP3_SERIAL_RX, MP3_SERIAL_TX);
// DFPlayerMini_Fast mp3Player;
DFRobotDFPlayerMini mp3Player;

String timeString = ""; //TODO

SimpleTimer timer;

LCD_1602_RUS lcd(0x3F, 16, 2);

const char heater[] PROGMEM = {0xF2, 0xE9, 0xF2, 0xE9, 0xE0, 0xF5, 0xFF, 0xFF};
const char home[] PROGMEM = {0xE4, 0xEE, 0xFF, 0xFF, 0xF1, 0xF5, 0xF1, 0xFF};
const char celsius[] PROGMEM = {0x18, 0x18, 0x00, 0x07, 0x05, 0x04, 0x05, 0x07};

int8_t t_heater = -99;
// int8_t t_heater_set = 0;
int8_t t_home = -99;
// float t_home = -99;
// int8_t t_home_set = 0;
int8_t gsm_signal = 0;
bool relayFlag = false;
bool backlightFlag = true;

bool updateMainScreen = true;

const char MENU_ON_OFF[] PROGMEM = {"Вкл/Выкл"};
const char MENU_SCHEDULE[] PROGMEM = {"Расписание"}; //TODO
const char MENU_TEMP[] PROGMEM = {"Уст.темп"};       //TODO
const char MENU_INFO[] PROGMEM = {"Инфо"};
const char MENU_EXIT[] PROGMEM = {"Выход"};

const char MENU_INFO_HOME[] PROGMEM = {"Дом       %02d°C"};
const char MENU_INFO_HEATER[] PROGMEM = {"Радиатор  %02d°C"};
const char MENU_INFO_GSM[] PROGMEM = {"GSM сигнал  %02d%%"};

uint8_t current_menu = 0; //0 - homepage, 1 - main menu, 2 - info menu
LiquidLine main_line1(1, 0, MENU_ON_OFF);
LiquidLine main_line2(1, 1, MENU_SCHEDULE);
LiquidLine main_line3(1, 0, MENU_TEMP);
LiquidLine main_line4(1, 1, MENU_INFO);
LiquidLine main_line5(1, 0, MENU_EXIT);

LiquidScreen main_screen1(main_line1, main_line2);
LiquidScreen main_screen2(main_line3, main_line4);
LiquidScreen main_screen3(main_line5);

LiquidLine info_line1(1, 0, MENU_INFO_HOME, t_home); //degree (char)223
LiquidLine info_line2(1, 1, MENU_INFO_HEATER, t_heater);
LiquidLine info_line3(1, 0, MENU_INFO_GSM, gsm_signal);
LiquidLine info_line4(1, 1, MENU_EXIT);
LiquidScreen info_screen1(info_line1, info_line2);
LiquidScreen info_screen2(info_line3, info_line4);

LiquidMenu main_menu(lcd, main_screen1, main_screen2, main_screen3, 1);
LiquidMenu info_menu(lcd, info_screen1, info_screen2, 1);

LiquidSystem menu_system(main_menu, info_menu, 1);

void func()
{ // Blank function, it is attached to the lines so that they become focusable.
  PRINTLNF("hello!");
}

void go_switch_relay()
{
  relayFlag = !relayFlag;
  digitalWrite(REL_PIN, relayFlag);
  PRINTLNF("relay switch!");
  // lcd.clear();
  // go_main_screen();
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
  { //if changed to 2 -> one turn
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
      // lcd.clear();
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

const char *printDigits(uint16_t digits, bool leadingZero = true, bool blinking = false)
{ //prints preceding colon and leading 0, blinking
  timeString = String(digits);
  if (blinking && ((millis() / 500 % 2) == 0))
    return "";
  if (digits < 10 && leadingZero)
    timeString = "0" + String(digits);

  return timeString.c_str();
}

void drawMainSreen()
{
  if (!updateMainScreen)
    return;
  // lcd.clear();
  char two_digits_buff[4] = "  ";
  lcd.createChar(5, celsius);
  lcd.createChar(6, heater);
  lcd.createChar(7, home);
  lcd.setCursor(0, 0);
  lcd.print(F("GSM "));
  sprintf(two_digits_buff,"%02d",gsm_signal);
  lcd.print(two_digits_buff);
  lcd.write('%');
  lcd.setCursor(0, 1);
  relayFlag ? lcd.print(F("ВКЛ")) : lcd.print(F("ВЫКЛ"));
  lcd.setCursor(11, 0);
  sprintf(two_digits_buff,"%02d",hour());
  lcd.print(two_digits_buff);
  lcd.write(':');
  sprintf(two_digits_buff,"%02d",minute());
  lcd.print(two_digits_buff);
  lcd.setCursor(5, 1);
  lcd.write(6); //heater sign
  sprintf(two_digits_buff,"%+03d",t_heater);
  lcd.print(two_digits_buff);
  lcd.write(5); //celsius
  lcd.setCursor(11, 1);
  lcd.write(7); //home sign
  sprintf(two_digits_buff,"%+03d",t_home);
  lcd.print(two_digits_buff);
  lcd.write(5); //celsius
  current_menu = 0;
  updateMainScreen = false;
}

void readMeasurements() //send request to temp sensors
{
  ds.reset();
  ds.write(0xCC, 0); // skip address (broadcast to all devices)
  ds.write(0x44, 0); // start conversion

  timer.setTimeout(200L, readDSresponse);
}

void readDSresponse() //read response from sensor
{
  byte DSdata[2]; // first and last temp bytes and TODO add CRC    
  // uint8_t temp_list[2] = {t_home, t_heater}; //TODO add PROGMEM
  // char DSaddr[8];
  // for (byte i = 0; i < 2; i++)
  // {
  //   strcpy_P(DSaddr, (char *)pgm_read_word(&(DSaddr_table[i]))); // Necessary casts and dereferencing, just copy.

  //   ds.reset();
  //   ds.select(DSaddr);
  //   ds.write(0xBE, 0); // read data from first DS
  //   DSdata[0] = ds.read();
  //   DSdata[1] = ds.read();
  //   temp_list[i] = ((int)DSdata[0] | (((int)DSdata[1]) << 8)) * 0.0625 + 0.03125;
  // }

  ds.reset();
  ds.select(DSaddr1);
  ds.write(0xBE, 0); // read data from first DS

  DSdata[0] = ds.read();
  DSdata[1] = ds.read();
  t_home = ((int)DSdata[0] | (((int)DSdata[1]) << 8)) * 0.0625 + 0.03125;

  ds.reset();
  ds.select(DSaddr2);
  ds.write(0xBE, 0); // read data from second DS

  DSdata[0] = ds.read();
  DSdata[1] = ds.read();
  t_heater = ((int)DSdata[0] | (((int)DSdata[1]) << 8)) * 0.0625 + 0.03125;

  if (current_menu == 0) //if main screen is displayed,
    updateMainScreen = true;
}

void initGSM()
{
  gsmSerial.begin(115200);
  String loading = "загрузка";

  pinMode(GSM_PIN, OUTPUT);
  digitalWrite(GSM_PIN, LOW);
  lcd.setCursor(3, 1); // display loading message
  lcd.print(loading);
  delay(2000);
  digitalWrite(GSM_PIN, HIGH);

  PRINTLNF("trying to set 9600");
  for (size_t i = 0; i < 4; i++)
  {
    gsmSerial.println(F("AT+IPR=9600")); //trying to set 9600
    delay(50);
  }
  gsmSerial.begin(9600);

  while (true)
  {
    gsmSerial.println(F("AT+CPAS"));
    if (gsmSerial.find("+CPAS:0"))
      break;

    lcd.setCursor(3, 1); // display loading message
    loading.trim();
    loading += ".   ";
    lcd.print(loading);
    if (loading.length() > 22)
      loading = loading.substring(0, 16);
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

void requestSignalQuality() //request signal quality from GSM
{
  // cleanSerialGSM();
  gsmSerial.println(F("AT+CSQ"));
#ifdef DEBUGGING
  PRINTLN("freeMemory()=", freeMemory())
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
  else if ((strBuffer.indexOf("RING") >= 0)) //return ring signal
  {
    gsmSerial.println(F("AT+CLCC"));
    backlightON();
    PRINTLNF("ringin!");
  }
  else if ((strBuffer.indexOf("+CSQ") >= 0)) //return signal quality, command like +CSQ: 22,99
  {
    strBuffer = strBuffer.substring(strBuffer.indexOf("+CSQ") + 6, 8); //return 22
    gsm_signal = strBuffer.toInt() * 100 / 31;
    PRINTLN("signal quality=", gsm_signal);
  }
  else if (strBuffer.indexOf("+CCLK") >= 0) //return time, command like +CCLK: "18/11/29,07:34:36+05"
  {
    strBuffer = strBuffer.substring(strBuffer.indexOf("+CCLK:") + 8, 30); //return 18/11/29,07:34:36+05
    PRINTLN("time string=", strBuffer);
    byte parse_time_arr[7];                            // Year, Month, Day, Hour, Minute, Second, timeZone
    memset(parse_time_arr, 0, sizeof(parse_time_arr)); //set zeros
    for (byte i = 0, str_index; i < 7; i++, str_index += 3)
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
  else if (strBuffer.indexOf("+CLCC") >= 0)
    if (checkNumber(strBuffer)) //check phone number +CLCC:
    {
      timer.setTimeout(5000L, hangUpGSM);

      relayFlag = !relayFlag;
      digitalWrite(REL_PIN, relayFlag);
      PRINTLNF("relay switch!");

      gsmSerial.println(F("ATA"));                       //answer call
      mp3Player.volume(28);                              //set volume value. From 0 to 30
      relayFlag ? mp3Player.play(1) : mp3Player.play(2); //play mp3
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
  for (int i = 0; i < 6; i++)
  {
    strcpy_P(buffer, (char *)pgm_read_word(&(phone_table[i]))); // Necessary casts and dereferencing, just copy.
    if (phone_string.indexOf(buffer) >= 0)
    {
      PRINTLNF("Bingo!");
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

// void backlightSwitch()
// {
//   PRINTLNF("switch backlight");
//   backlightFlag != backlightFlag;
//   lcd.setBacklight(backlightFlag);
//   menu_system.switch_focus();
//   updateMainScreen = true;
//   current_menu = 0;
// }

void backlightOFF()
{
  if (backlightFlag)
  {
    PRINTLNF("switch backlight off");
    backlightFlag = false;
    lcd.setBacklight(backlightFlag);
  }
}

void backlightON()
{
  timer.restartTimer(2); //restart backlight timer
  if (!backlightFlag)
  {
    PRINTLNF("switch backlight on");
    backlightFlag = true;
    lcd.setBacklight(backlightFlag);
  }
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
  lcd.print(F("загрузка...")); //init display

#ifdef DEBUGGING
  Serial.begin(9600);
#endif

  // sendDScommand(DSaddr1); //request temp from DS sensor

  initMP3(); //mp3 serial port by default is not listening

  initGSM(); //init GSM module, the last intialized port is listening
  delay(1000);
  requestTime(); //request time from GSM and sync internal clock
  delay(200);
  requestSignalQuality();                        //request signal quality from GSM and sync internal clock
  readMeasurements();                            //request temp
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

  main_line1.set_asProgmem(1); //set PROGMEM menu lines
  main_line2.set_asProgmem(1);
  main_line3.set_asProgmem(1);
  main_line4.set_asProgmem(1);
  main_line5.set_asProgmem(1);

  info_line1.set_asProgmem(1);
  info_line2.set_asProgmem(1);
  info_line3.set_asProgmem(1);
  info_line4.set_asProgmem(1);

  timer.setInterval(5000, readMeasurements);
  timer.setInterval(SECS_PER_MIN * 5000L, backlightOFF); //auto backlight off 5 mins
  timer.setInterval(SECS_PER_HOUR * 6000L, requestTime); //sync time every 6 hours
  timer.setInterval(10000L, requestSignalQuality);       //request signal quality every 10 secs

  lcd.clear();
  drawMainSreen();

  gsmSerial.setTimeout(100); //set timeout for readString() func
}
void loop()
{
  timer.run();
  readButton();
  readEncoder();
  drawMainSreen();
  readStringGSM();
}
