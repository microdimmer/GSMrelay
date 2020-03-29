//use DFPlayerMini_Fast, 
//add shedule and temp automation, 
//add preferences menu section, 
//add voice temp information, add read BUSY pin +
//!!!!edit LiquidMenu_config.h first!!!!
/// Configures the number of available variables per line.
// const uint8_t MAX_VARIABLES = 2; ///< @note Default: 5
// /// Configures the number of available functions per line.
// const uint8_t MAX_FUNCTIONS = 1; ///< @note Default: 8
// /// Configures the number of available lines per screen.
// const uint8_t MAX_LINES = 2; ///< @note Default: 12
// /// Configures the number of available screens per menu.
// const uint8_t MAX_SCREENS = 3; ///< @note Default: 14
// /// Configures the number of available menus per menus system.
// const uint8_t MAX_MENUS = 3; ///< @note Default: 8

#define DEBUGGING

const char PROG_VERSION = '3';
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
// #include <DFRobotDFPlayerMini.h> //DF MP3 Player mini
#include <DFPlayerMini_Fast.h> //DFPlayer MP3 mini https://github.com/scottpav/DFPlayerMini_Fast
#include <ClickEncoder.h>   //encoder with button https://github.com/0xPIT/encoder
#include <TimeLib.h>        //timekeeping https://github.com/PaulStoffregen/Time
#include <SoftwareSerial.h> //for GSM modem A6 and MP3 module 
#include <SimpleTimer.h>    // Handy timers with callback parameters and polling https://github.com/marcelloromani/Arduino-SimpleTimer
#include <LiquidMenu.h>     //The menu wrapper library https://github.com/microdimmer/LiquidMenu and https://github.com/johnrickman/LiquidCrystal_I2C
#include <EEPROM.h>         //saving log data to EEPROM and send SMS every 2.5 month

struct Log {
  int8_t home_temp; // temp +- 63, 1 byte, and sentinel, see this https://sites.google.com/site/dannychouinard/Home/atmel-avr-stuff/eeprom-longevity
  int8_t heater_temp; // heater +- 128, 1 byte
  int16_t balance; // balance +-32967, 2 bytes
  time_t unix_time; // current POSIX time, 4 bytes
}; //total 8 bytes (64 bits)

struct Prefs {
  int8_t t_home_set; // temp +- 63, 1 byte, and sentinel, see this https://sites.google.com/site/dannychouinard/Home/atmel-avr-stuff/eeprom-longevity
  int8_t t_heater_set; // heater +- 63, 1 byte, and thermostat flag
  int8_t t_home_hysteresis_set; // home hysteresis +- 128, 1 byte
  int8_t t_heater_hysteresis_set; // heater hysteresis +- 128, 1 byte
}; //total 4 bytes (32 bits)

OneWire ds(TEMP_SENSOR_PIN);
byte DSaddr[2][8]; // first and second DS18B20 addresses, home 28FFE44750170473 heater 28FF2FDAC11704DE

ClickEncoder encoder(ENCODER_PIN1, ENCODER_PIN2, ENCODER_PIN3, 2); //changed to 2 at one turn
ClickEncoder::Button button;
int16_t last_val = 0;
int16_t enc_val = 0;

SoftwareSerial gsmSerial(GSM_SERIAL_RX, GSM_SERIAL_TX);
SoftwareSerial mp3Serial(MP3_SERIAL_RX, MP3_SERIAL_TX);
DFPlayerMini_Fast mp3Player;
// DFRobotDFPlayerMini mp3Player;
SimpleTimer timer;
LiquidCrystal_I2C lcd(0x3F, 16, 2);

char GSMstring[64] = {'\0'}; //GSM read string buffer
char string_buff[8] = {'\0'}; //string buffer, used for showing info on display
char phone_buff[12] = {'\0'} ; //phone number string
int8_t temp[2] = {-99, -99}; // temp home, temp heater
int8_t temp_set[2] = {25, 70}; // temp home to set, temp heater to set
int8_t temp_set_hysteresis[2] = {2, 10}; // temp home to set, temp heater to set
int8_t signalStrength = 0; //GSM signal strength
bool GSMonAirFlag = false; //answer call flag
bool GSMwaitReqFlag = false; // waiting request command flag
uint8_t readResponseCounter = 0; // count response commands to run, max 20 times x 500ms
const uint8_t readResponseNum = 20;  //limit 20 times to wait
const uint16_t readResponseTimeout = 500;  //responce timeout to wait
int16_t backlightTimerID = 0;

uint16_t memoryFree = 0;
int16_t balance = -32768; //min uint
CircularBuffer<uint8_t,16> audioQueue;     // audio sequence size, can play five files continuously CircularBuffer<uint8_t,6> audioQueue;
bool workFlag = false;
bool relayFlag = false;
bool thermostatFlag = false; 
bool backlightFlag = true;
bool updateMainScreenFlag = true;
bool clearMainSreenFlag = false;
bool SIMreadyOK = false;
bool GSMinitOK = false;
bool timeSyncOK = false;
bool signalSyncOK = false;
bool balanceSyncOK = false;
uint8_t currentMenu = 0; //0 - homepage, 1 - main menu, 2 - info menu, 3 - temp menu, 4 - set params

#include "menu.h"
#include "debug.h"

void jobThermostat() {
  if (workFlag && thermostatFlag) {
    if (temp_set[0] - temp[0] >= temp_set_hysteresis[0]) {
        if (!relayFlag) {
          relayFlag = true;
          digitalWrite(RELAY_PIN, relayFlag);
          PRINTLNF("relay on");
        }
    }
    else if (relayFlag)  {
          relayFlag = false;
          digitalWrite(RELAY_PIN, relayFlag);
          PRINTLNF("relay off");
    }
  }
}

void setup() {
  #ifdef DEBUGGING
  Serial.begin(9600);
  PRINTLNF("Debug on");
  #endif
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayFlag); //relay OFF

  initMenu();
  initMP3(); //mp3 serial port by default is not listening
  initGSM(); //init GSM module, the last intialized port is listening
  initDS();  //init DS temp modules
  requestTempUpdateScreen();  //request temp

  timer.setInterval(1000, requestTempUpdateScreen); //request temp once a second and update screen //TODO separate
  backlightTimerID = timer.setInterval(SECS_PER_MIN * 10000L, backlightOFF); //auto backlight off 10 mins
  
  if (!GSMinitOK) {
    lcd.clear();
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
  timer.setInterval(10000L, jobThermostat);        //do thermostat job
  timer.setInterval(SECS_PER_HOUR * 6000L, requestTime); //sync time every 6 hours
  timer.setInterval(SECS_PER_DAY * 80L, sendSMSBalance); //send sms with balance every 80 days
  timer.setInterval(SECS_PER_HOUR * 6000L, requestBalance);//request balance every 6 hours

  readStringGSM();
  lcd.clear();
  drawMainSreen();
}

void loop() {
  timer.run();
  readButton();
  readEncoder();
  drawMainSreen();
  readStringGSM();
}
