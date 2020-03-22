const char heater[] PROGMEM = "\xF2\xE9\xF2\xE9\xE0\xF5\xFF\xFF"; //{0xF2, 0xE9, 0xF2, 0xE9, 0xE0, 0xF5, 0xFF,  0xFF}; heater sign
const char home[] PROGMEM = "\xE4\xEE\xFF\xFF\xF1\xF5\xF1\xFF"; //{0xE4, 0xEE, 0xFF, 0xFF, 0xF1, 0xF5, 0xF1, 0xFF};
const char celsius[] PROGMEM = "\x18\x18\x00\x07\x05\x04\x05\x07"; //{0x18, 0x18, 0x00, 0x07, 0x05, 0x04, 0x05, 0x07};
const char gsm[] PROGMEM = "\x01\x03\x1F\x11\x1F\x15\x1B\x1F"; //{0x01, 0x03, 0x1F, 0x11, 0x1F, 0x15, 0x1B, 0x1F};
const char memory[] PROGMEM = "\x0A\x1F\x0A\x1F\x0A\x1F\x0A\x00"; //{0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00};
const char ruble[] PROGMEM = "\x0E\x09\x09\x1E\x08\x1C\x08\x08"; //{ 0x0E,  0x09,  0x09,  0x1E,  0x08,  0x1C,  0x08, 0x08};

const char MENU_ON_OFF[] PROGMEM = {"BK\247/B\256K\247"}; //ВКЛ/ВЫКЛ
const char MENU_TEMP[] PROGMEM = {"\251CTAHOBKA TEM\250"}; //УСТАНОВКА ТЕМП       //TODO
//const char MENU_SCHEDULE[] PROGMEM = {"Расписание"}; //TODO
const char MENU_INFO[] PROGMEM = {"\245H\252O"}; //ИНФО
const char MENU_EXIT[] PROGMEM = {"B\256XO\340"}; //ВЫХОД

const char MENU_TEMP_SET_HOME[] PROGMEM = {"TEM\250 BO\244\340  +%02d\x05"}; //ТЕМП ВОЗД  \x06
const char MENU_TEMP_SET_RADIATOR[] PROGMEM = {"TEM\250 PA\340   +%02d\x05"}; //ТЕМП РАД \x07
const char MENU_TEMP_HOME_HYSTERESIS[] PROGMEM = {"\241\245CT BO\244\340   %02d\x05"}; //ГИСТ ВОЗД \x06
const char MENU_TEMP_RADIATOR_HYSTERESIS[] PROGMEM = {"\241\245CT PA\340    %02d\x05"}; //ГИСТ РАД \x07

const char MENU_INFO_HOME[] PROGMEM = {"\340o\274      %+03d°C"}; //Дом
const char MENU_INFO_HEATER[] PROGMEM = {"Pa\343\270a\277op %+03d°C"}; //Радиатор
const char MENU_INFO_GSM[] PROGMEM = {"GSM c\270\264\275a\273  %02d%%"}; //GSM сигнал
//const char MENU_INFO_RAM[] PROGMEM = {"Память %03d%b"};

const char CLEAR_LINE[] PROGMEM = {"  "};
const char FORMAT_DIGITS_2[] PROGMEM = {"%02d"};
const char FORMAT_DIGITS_3_SP[] PROGMEM = {"%+03d"};
const char EMPTY_NUM[] PROGMEM = {"--"};
const char CUSD[] PROGMEM = {"+CUSD:"};
const char CCLK[] PROGMEM = {"+CCLK:"};
const char CSQ[] PROGMEM = {"+CSQ:"};
const char DELIMETERS[] PROGMEM = {"/,:+"};


LiquidLine main_line1(1, 0, MENU_ON_OFF);
LiquidLine main_line2(1, 1, MENU_TEMP);
//LiquidLine main_line3(1, 0, MENU_SCHEDULE);
LiquidLine main_line4(1, 0, MENU_INFO);
LiquidLine main_line5(1, 1, MENU_EXIT);
//LiquidScreen main_screen1(main_line1, main_line2);
//LiquidScreen main_screen2(main_line3, main_line4);
LiquidScreen main_screen1(main_line1, main_line2);
LiquidScreen main_screen3(main_line4, main_line5);
LiquidLine temp_line1(1, 0, MENU_TEMP_SET_HOME, t_home_set);
LiquidLine temp_line2(1, 1, MENU_TEMP_SET_RADIATOR, t_heater_set);
LiquidLine temp_line3(1, 0, MENU_TEMP_HOME_HYSTERESIS,t_home_hysteresis_set);
LiquidLine temp_line4(1, 1, MENU_TEMP_RADIATOR_HYSTERESIS,t_heater_hysteresis_set);
LiquidLine temp_line5(1, 0, MENU_EXIT);
//
LiquidScreen temp_screen1(temp_line1, temp_line2);
LiquidScreen temp_screen2(temp_line3, temp_line4);
LiquidScreen temp_screen3(temp_line5);
LiquidLine info_line1(1, 0, MENU_INFO_HOME, temp[0]);
LiquidLine info_line2(1, 1, MENU_INFO_HEATER, temp[1]);
LiquidLine info_line3(1, 0, MENU_INFO_GSM, signalStrength);
//LiquidLine info_line4(1, 1, MENU_INFO_RAM, memory_free);
LiquidLine info_line4(1, 1, MENU_EXIT);
LiquidScreen info_screen1(info_line1, info_line2);
LiquidScreen info_screen2(info_line3, info_line4);
// LiquidScreen info_screen3(info_line5);
LiquidMenu main_menu(lcd, main_screen1, main_screen3, 1);
LiquidMenu temp_menu(lcd, temp_screen1, temp_screen2, temp_screen3, 1);
LiquidMenu info_menu(lcd, info_screen1, info_screen2, 1);
// LiquidSystem menu_system(main_menu, info_menu, 1);
LiquidSystem menu_system(main_menu, info_menu, temp_menu, 1);
