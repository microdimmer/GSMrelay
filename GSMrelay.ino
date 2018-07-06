const char PROG_VERSION[] = "0.1";
 #define DEBUG
#include <SoftwareSerial.h>    //modem A6
#include <DallasTemperature.h> //DS18B20
#include <DFMiniMp3.h>         //DF MP3 Player mini
#include <ClickEncoder.h>      //encoder with button https://github.com/0xPIT/encoder
// #define ENCODER_OPTIMIZE_INTERRUPTS
// #include <Encoder.h>
// Encoder myEnc(2, 3);
ClickEncoder encoder(2, 3, 4);
int16_t last_val, enc_val;

#include <SimpleTimer.h> // Handy timers
SimpleTimer timer;

#include <LiquidMenu.h> //The menu wrapper library
#include <LiquidCrystal_I2C.h>
#include <LCD_1602_RUS.h> //1602 display rus
LCD_1602_RUS lcd(0x3F, 16, 2);

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
int8_t t_home = -23;
int8_t t_home_set = 0;

bool heating = false;
bool updateFlag = true;

LiquidLine main_line1(1, 0, "Power");
LiquidLine main_line2(1, 1, "Shedule");
LiquidLine main_line3(1, 0, "Info");
LiquidLine main_line4(1, 1, "Time");
LiquidLine main_line5(1, 0, "Exit");
LiquidScreen main_screen1(main_line1, main_line2);
LiquidScreen main_screen2(main_line3, main_line4);
LiquidScreen main_screen3(main_line5);

LiquidLine info_line1(1, 0, "Home temp ", t_home);
LiquidLine info_line2(1, 1, "Heater temp ", t_heater);
LiquidLine info_line3(1, 0, "GSM signal");
LiquidLine info_line4(1, 1, "Exit");
LiquidScreen info_screen1(info_line1, info_line2);
LiquidScreen info_screen2(info_line3, info_line4);

LiquidMenu main_menu(lcd, main_screen1, main_screen2, main_screen3, 1);
LiquidMenu info_menu(lcd, info_screen1, info_screen2, 1);

LiquidSystem menu_system(main_menu, info_menu, 1);

void timerIsr()
{
  encoder.service();
}

void func() { // Blank function, it is attached to the lines so that they become focusable.
  PRINTLNF("hello!");
}

void go_info_menu() {
  PRINTLNF("changing to info menu!");
  menu_system.change_menu(info_menu);
  menu_system.change_screen(1);
  menu_system.switch_focus();
}

void go_main_menu() {
  PRINTLNF("changing to main menu!");
  menu_system.switch_focus();
  menu_system.change_menu(main_menu);
  
}

void readMeasurements()
{
  enc_val += encoder.getValue(); //read encoder
  if (enc_val != last_val) {
    if (last_val > enc_val){ 
      if (!menu_system.switch_focus()) { // end of menu lines
         menu_system++; 
         menu_system.switch_focus();
      } 
    }
    else 
      if (!menu_system.switch_focus(false)) { // end of menu lines
        menu_system--; 
        menu_system.switch_focus(false);
      } 
    last_val = enc_val;
  }
  ClickEncoder::Button b = encoder.getButton();
  if (b != ClickEncoder::Open)
  {
    menu_system.call_function(1);
  }
}

void drawMenu()
{
  lcd.clear();
}

void drawMainSreen()
{
  lcd.clear();
  drawMenu();

  // lcd.createChar(6, heater);
  // lcd.createChar(7, home);
  // lcd.setCursor(0, 0);
  // heating?lcd.print("ВКЛ"):lcd.print("ВЫКЛ");
  // lcd.setCursor(11, 0);
  // lcd.print("12:59");
  // lcd.setCursor(0, 1);
  // lcd.write(6);
  // if (t_heater>0) lcd.print("+");
  // lcd.print(t_heater);
  // lcd.print("°C");
  // lcd.setCursor(8, 1);
  // lcd.write(7);
  // if (t_home>0) lcd.print("+");
  // lcd.print(t_home);
  // lcd.print("°C");

  updateFlag = false;
}

void setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(1, 0);
  lcd.print("GSM-реле v.");
  lcd.setCursor(12, 0);
  lcd.print(PROG_VERSION);
  lcd.setCursor(3, 1);
  lcd.print("загрузка...");
  delay(150);
  main_menu.set_focusPosition(Position::LEFT);
  info_menu.set_focusPosition(Position::LEFT);
  // main_menu.init();
  // info_menu.init();
  
  menu_system.set_focusPosition(Position::LEFT);

  main_line1.attach_function(1, go_info_menu);
  main_line2.attach_function(1, func);
  main_line3.attach_function(1, go_info_menu);
  main_line4.attach_function(1, func);
  main_line5.attach_function(1, func);
  info_line1.attach_function(1, func);
  info_line2.attach_function(1, func);
  info_line3.attach_function(1, func);
  info_line4.attach_function(1, go_main_menu);
  menu_system.switch_focus();
  menu_system.update();
  // delay(2500);
  // main_menu.next_screen();
  // delay(1500);
  // main_menu.next_screen();

  //  last_val = -1;

  timer.setInterval(1000L, readMeasurements);
  timer.setInterval(1L, timerIsr);
}
void loop()
{
  timer.run();
  // if (updateFlag)
  // drawMainSreen();
}
