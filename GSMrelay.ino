const char PROG_VERSION[] = "0.1";
#define DEBUG

#include <OneWire.h>      //for DS18B20
#include <DFMiniMp3.h>    //DF MP3 Player mini
#include <ClickEncoder.h> //encoder with button https://github.com/0xPIT/encoder
#include <TimeLib.h>      //timekeeping
// #include <iarduino_GSM.h> //for GSM modem A6
#include <SoftwareSerial.h> //for GSM modem A6

OneWire ds(12);
byte DSaddr1[8] = {0x28, 0xFF, 0xE4, 0x47, 0x50, 0x17, 0x04, 0x73}; // first DS18B20 adress
byte DSdata1[9];                                                    //first and last temp bytes and CRC

ClickEncoder encoder(A1, A0, A2, 2); //changed to 2 at one turn
ClickEncoder::Button button;
int16_t last_val, enc_val;

// iarduino_GSM gsm;
SoftwareSerial gsmSerial(2, 3);
char callnumber[13];

String timeString = ""; //TODO

#include <SimpleTimer.h> // Handy timers
SimpleTimer timer;
#include <LiquidMenu.h> //The menu wrapper library
LCD_1602_RUS lcd(0x3F, 16, 2);

int8_t REL_PIN = 11; //pin for relay1
int8_t GSM_PIN = 9;  //pin for GSM

#ifdef DEBUG
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

uint8_t heater[8] = {0xF2, 0xE9, 0xF2, 0xE9, 0xE0, 0xF5, 0xFF, 0xFF};
uint8_t home[8] = {0xE4, 0xEE, 0xFF, 0xFF, 0xF1, 0xF5, 0xF1, 0xFF};

int8_t t_heater = 73;
int8_t t_heater_set = 0;
float t_home = -99;
// int8_t t_home = -23;
int8_t t_home_set = 0;
int8_t gsm_signal = 55;
bool relayFlag = false;

bool updateMainScreen = true;

bool GSMTimeoutFlag = false;

uint8_t current_menu = 0; //0 - homepage, 1 - main menu, 2 - info menu
LiquidLine main_line1(1, 0, "Power");
LiquidLine main_line2(1, 1, "Shedule");
LiquidLine main_line3(1, 0, "Info");
LiquidLine main_line4(1, 1, "Time");
LiquidLine main_line5(1, 0, "Exit");
LiquidScreen main_screen1(main_line1, main_line2);
LiquidScreen main_screen2(main_line3, main_line4);
LiquidScreen main_screen3(main_line5);

LiquidLine info_line1(1, 0, "Home    t %+02d°C", t_home); //degree (char)223
LiquidLine info_line2(1, 1, "Heater  t %+02d°C", t_heater);
LiquidLine info_line3(1, 0, "GSM signal  %02d%%", gsm_signal);
LiquidLine info_line4(1, 1, "Exit");
LiquidScreen info_screen1(info_line1, info_line2);
LiquidScreen info_screen2(info_line3, info_line4);

LiquidMenu main_menu(lcd, main_screen1, main_screen2, main_screen3, 1);
LiquidMenu info_menu(lcd, info_screen1, info_screen2, 1);

LiquidSystem menu_system(main_menu, info_menu, 1);

void func()
{ // Blank function, it is attached to the lines so that they become focusable.
  PRINTLNF("hello!");
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
  // lcd.clear();
  // drawMainSreen();
}

void readEncoder()
{
  if (current_menu == 0)
    return;
  encoder.service();
  enc_val += encoder.getValue(); //read encoder
  if (enc_val == last_val)
    return;
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
  lcd.createChar(6, heater);
  lcd.createChar(7, home);
  lcd.setCursor(0, 0);
  relayFlag ? lcd.print("  ВКЛ") : lcd.print(" ВЫКЛ");
  lcd.setCursor(11, 0);
  lcd.print(printDigits(hour()));
  lcd.print(":");
  lcd.print(printDigits(minute()));
  lcd.setCursor(0, 1);
  lcd.write(6); //heater sign
  if (t_heater > 0)
    lcd.print("+");
  lcd.print(t_heater);
  lcd.print("°C");
  lcd.setCursor(8, 1);
  lcd.write(7); //home sign
  if (t_home > 0)
    lcd.print("+");
  lcd.print(t_home, 1);
  lcd.print("°C");
  // PRINTLNF("show main screen");
  current_menu = 0;
  updateMainScreen = false;
}

void updateScr()
{
  lcd.setCursor(9, 1);
  if (t_home > 0)
    lcd.print("+");
  lcd.print(t_home, 1);
  lcd.print("°C");
}

void readMeasurements() //send request to temp sensor
{
  ds.reset();
  ds.select(DSaddr1);
  ds.write(0x44, 0); // start conversion

  ds.reset();
  ds.select(DSaddr1);
  ds.write(0xBE, 0); // read Scratchpad

  timer.setTimeout(200L, readDSresponse);
}

void readDSresponse() //read response from sensor
{
  DSdata1[0] = ds.read();
  DSdata1[1] = ds.read();
  t_home = (float)((int)DSdata1[0] | (((int)DSdata1[1]) << 8)) * 0.0625 + 0.03125;

  if (current_menu == 0) //if main screen is displayed,
    updateMainScreen = true;
}

void setTimenow() //send request to GSM
{
  while (gsmSerial.available() > 0) //clean UART buffer
    gsmSerial.read();

  gsmSerial.println(F("AT+CCLK?"));
  PRINTLNF("send request command to GSM");
  timer.setTimeout(500L, readGSMresponse);
}

void readGSMresponse() //read response from GSM
{
  PRINTLNF("trying to sync clock");

  char i;
  String strBuffer;
  if (gsmSerial.find("+CCLK:")) //return command like +CCLK: "18/11/29,07:34:36+05"
  {
    while (gsmSerial.available() > 0)
    {
      i = (gsmSerial.read());
      strBuffer += i;
    }
  }

  byte Year, Month, Day, Hour, Minute, Second, timeZone; //return command like +CCLK: "18/11/29,07:34:36+05"
  Year = strBuffer.substring(2, 4).toInt();
  Month = strBuffer.substring(5, 7).toInt();
  Day = strBuffer.substring(8, 10).toInt();
  Hour = strBuffer.substring(11, 13).toInt();
  Minute = strBuffer.substring(14, 16).toInt();
  Second = strBuffer.substring(17, 19).toInt();
  timeZone = strBuffer.substring(20, 22).toInt();
  setTime(Hour, Minute, Second, Day, Month, Year);
  adjustTime(timeZone * SECS_PER_HOUR); //set timezone
}

void switchRelay()
{
  // if (gsm.CALLavailable(callnumber))
  // {
  //   PRINTLN("calling number ", callnumber);
  //   gsm.CALLend();
  //   relayFlag = !relayFlag;
  //   digitalWrite(REL_PIN, relayFlag);
  //   if (current_menu == 0)
  //     updateMainScreen = true;
  // }
}

void initGSM()
{
  gsmSerial.begin(115200);

  pinMode(GSM_PIN, OUTPUT);
  digitalWrite(GSM_PIN, LOW);
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
    // while (gsmSerial.available())
    // Serial.write(gsmSerial.read()); //Forward what Software Serial received to Serial Port
    (millis() / 500 % 2) ? lcd.print(F("загрузка..")) : lcd.print(F("загрузка. "));
    delay(500);
    PRINTLNF("initialize GSM...");
  }
  // gsmSerial.println(F("ATE0"));
  PRINTLNF("init GSM OK");
}

void setup()
{
  pinMode(REL_PIN, OUTPUT);
  digitalWrite(REL_PIN, relayFlag); //relay OFF

  lcd.init(); //init display
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print(F("GSM-реле v."));
  lcd.setCursor(12, 0);
  lcd.print(PROG_VERSION);
  lcd.setCursor(3, 1);
  lcd.print(F("загрузка...")); //init display

  Serial.begin(9600);

  // sendDScommand(DSaddr1); //request temp from DS sensor

  initGSM();
  // gsm.begin(gsmSerial); //init GSM module
  // while (gsm.status() != GSM_OK)
  // {
  //   delay(1000);
  //   PRINTLN("status ", gsm.status());
  //   lcd.setCursor(3, 1);
  //   (millis() / 500 % 2) ? lcd.print(F("загрузка..")) : lcd.print(F("загрузка. "));
  // }

  // gsm.runAT(F("ATE0\r\n"));
  // PRINTLN("time is", gsm.runAT("AT+CCLK?\r\n"));

  setTimenow(); //request time from GSM and sync internal clock

  menu_system.set_focusPosition(Position::LEFT); //init menu system
  main_line1.attach_function(1, func);
  main_line2.attach_function(1, func);
  main_line3.attach_function(1, go_info_menu);
  main_line4.attach_function(1, func);
  main_line5.attach_function(1, go_main_screen);
  info_line1.attach_function(1, func);
  info_line2.attach_function(1, func);
  info_line3.attach_function(1, func);
  info_line4.attach_function(1, go_main_menu);
  // menu_system.switch_focus();

  // setTime(12, 59, 40, 01, 01, 19);

  readMeasurements();

  timer.setInterval(5000, readMeasurements);
  timer.setInterval(SECS_PER_HOUR * 6 * 1000L, setTimenow); //sync time every 6 hours
  // timer.setInterval(5000L, setTimenow); //sync time every 6 hours
  // timer.setInterval(5000L, updateMainScreen);
  // timer.setInterval(1L, timerIsr);

  lcd.clear();
  drawMainSreen();
}
void loop()
{
  timer.run();
  readButton();
  readEncoder();

  drawMainSreen();
  // switchRelay();
}
