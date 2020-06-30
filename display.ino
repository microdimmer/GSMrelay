void initMenu() {
  lcd.init(); //init display
  lcd.createChar(0, thermostat);
  lcd.createChar(3, ruble);
  lcd.createChar(4, gsm);
  lcd.createChar(5, celsius);
  lcd.createChar(6, home);
  lcd.createChar(7, heater);
  lcd.setBacklight(backlightFlag);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print(F("GSM-pe\273e v.")); //GSM-реле v.
  lcd.setCursor(12, 0);
  lcd.print(F("0."));
  lcd.print(PROG_VERSION);
  lcd.setCursor(3, 1); // display loading message
  lcd.print(F("\267a\264py\267\272a     ")); //загрузка
  
  menu_system.set_focusPosition(Position::LEFT); //init menu system
  main_line1.attach_function(1, setWorkFlag);
  main_line2.attach_function(1, setThermostatFlag);
  main_line3.attach_function(1, goTempMenu);
  main_line4.attach_function(1, goInfoMenu);
  main_line5.attach_function(1, goHomeScreen);

  temp_line1.attach_function(1, setHomeTemp);
  temp_line2.attach_function(1, setHeaterTemp);
  temp_line3.attach_function(1, setHomeHysteresis);
  temp_line4.attach_function(1, setHeaterHysteresis);
  temp_line5.attach_function(1, savePrefsGoHomeScreen); //exit
  
  info_line1.attach_function(1, func);
  info_line2.attach_function(1, func);
  info_line3.attach_function(1, func);
  info_line4.attach_function(1, goMainMenu);
  // info_line5.attach_function(1, goMainMenu);

  main_line1.set_asProgmem(1); //set PROGMEM menu lines
  main_line2.set_asProgmem(1);
  main_line3.set_asProgmem(1);
  main_line4.set_asProgmem(1);
  main_line5.set_asProgmem(1);

  temp_line1.set_asProgmem(1);
  temp_line2.set_asProgmem(1);
  temp_line3.set_asProgmem(1);
  temp_line4.set_asProgmem(1);
  temp_line5.set_asProgmem(1);

  info_line1.set_asProgmem(1);
  info_line2.set_asProgmem(1);
  info_line3.set_asProgmem(1);
  info_line4.set_asProgmem(1);
  // info_line5.set_asProgmem(1);
}

void drawMainSreen() {
  static char two_digits_buff[4];

  if (!updateMainScreenFlag)
    return;
  if (clearMainSreenFlag) {
    lcd.clear();
    clearMainSreenFlag = false;
  }
  ///temp set menu
  // static bool blinking = true;
  // blinking = millis() % 1000;
  if (currentMenu >= 4) { //if temp set mode
    // static bool blinking = true; //see #111 ((millis() / 1000) % 2) ? lcd.write(':') : lcd.write(' ');
    // blinking ? strcpy_P(string_buff,CLEAR_LINE) : strcpy_P(string_buff, FORMAT_DIGITS_2);
    blinking ? strcpy_P(string_buff,CLEAR_LINE) : strcpy_P(string_buff, FORMAT_DIGITS_2);
    blinking = !blinking;
    switch (currentMenu) {
    case 4:
      sprintf(two_digits_buff, string_buff, temp_set[0]); // home
      lcd.setCursor(13, 0);
      break;
    case 5:
      sprintf(two_digits_buff, string_buff, temp_set[1]); // heater
      lcd.setCursor(13, 1);
      break;
    case 6:
      sprintf(two_digits_buff, string_buff, temp_set_hysteresis[0]); // home hysteresis 
      lcd.setCursor(13, 0);
      break;
    case 7:
      sprintf(two_digits_buff, string_buff, temp_set_hysteresis[1]); // heater hysteresis 
      lcd.setCursor(13, 1);
      break;
    default:
      break;
    }
    lcd.print(two_digits_buff);
    updateMainScreenFlag = false;
    return;
  }

  ///--temp set menu
  currentMenu = 0;
  updateMainScreenFlag = false;

  //thermostat state show
  if (thermostatFlag) {
    lcd.write(0); 
  }
  //relay state show
  lcd.setCursor(1, 0);
  workFlag ? lcd.print(F("BK\247")) : lcd.print(F("B\256K")); //ВКЛ ВЫК
  
  //time show
  lcd.setCursor(5, 0);
  sprintf(two_digits_buff, strcpy_P(string_buff, FORMAT_DIGITS_2), hour());
  lcd.print(two_digits_buff);
  // ((millis() / 1000) % 2) ? lcd.write(':') : lcd.write(' ');
  blinking ? lcd.write(':') : lcd.write(' ');
  blinking = !blinking;
  sprintf(two_digits_buff, string_buff, minute());
  lcd.print(two_digits_buff);
  
  //temp show
  for (uint8_t i = 0; i<=1; i++) {
    lcd.setCursor(11, i);
    lcd.write(6+i); //6 home sign, 7 heater sign
    if (temp[i]==-99) {
      lcd.print(strcpy_P(two_digits_buff, EMPTY_NUM)); //temp is invalid show --
    }
    else { 
      sprintf(two_digits_buff, strcpy_P(string_buff, FORMAT_DIGITS_3_SP), temp[i]);
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
  lcd.setCursor(1, 1);
  if (balance==-32768) {
    lcd.print(strcpy_P(two_digits_buff, EMPTY_NUM));
  }
  else {
    sprintf(two_digits_buff, strcpy_P(string_buff, FORMAT_DIGITS_2), balance);
    lcd.print(two_digits_buff);
  }
  lcd.write(3); // russian currency sign
  
  //signal strength show
  lcd.setCursor(6, 1);
  lcd.write(4); //GSM sign
  if (signalStrength==0) {
    lcd.print(strcpy_P(two_digits_buff, EMPTY_NUM));
  }
  else {  
    sprintf(two_digits_buff, strcpy_P(string_buff, FORMAT_DIGITS_2), signalStrength);
    lcd.print(two_digits_buff);
  }
  lcd.write('%');
  lcd.write(' ');
}

void backlightOFF() {
  if (backlightFlag) {
    PRINTLNF("backlight off");
    backlightFlag = false;
    lcd.setBacklight(backlightFlag);
    updateMainScreenFlag = true;
    clearMainSreenFlag = true;
  }
}

void backlightON() {
  timer.restartTimer(backlightTimerID); //restart backlight timer, ID 2
  if (!backlightFlag) {
    PRINTLNF("backlight on");
    backlightFlag = true;
    lcd.setBacklight(backlightFlag);
    updateMainScreenFlag = true;
    clearMainSreenFlag = true;
  }
}

void func() {// Blank function, it is attached to the lines so that they become focusable.
  PRINTLNF("hi");
}

void setHomeTemp() {
  currentMenu == 4 ? currentMenu = 3 :  currentMenu = 4;
}

void setHeaterTemp() {
  currentMenu == 5 ? currentMenu = 3 : currentMenu = 5;
}

void setHomeHysteresis() {
  currentMenu == 6 ? currentMenu = 3 :  currentMenu = 6;
}

void setHeaterHysteresis() {
  currentMenu == 7 ? currentMenu = 3 :  currentMenu = 7;
}

void setWorkFlag() {
  workFlag ^= 1;
  if (!thermostatFlag) {
    relayFlag = workFlag;
    digitalWrite(RELAY_PIN, relayFlag);
    PRINTLNF("relay switch");
  }
  else {
    relayFlag = false;
    digitalWrite(RELAY_PIN, relayFlag);
  }
  menu_system.switch_focus();
  menu_system.switch_focus();
  updateMainScreenFlag = true;
  clearMainSreenFlag = true;
  currentMenu = 0;
}

void setThermostatFlag() {
  thermostatFlag ^= 1; //invert flag
  PRINTLN("thermostat=",thermostatFlag);
  if (!thermostatFlag) {
    workFlag = false; //off work flag
    relayFlag = false; //switch off
    digitalWrite(RELAY_PIN, relayFlag);
  }
  menu_system.switch_focus();
  menu_system.switch_focus();
  updateMainScreenFlag = true;
  clearMainSreenFlag = true;
  currentMenu = 0;
}

void goInfoMenu() {
  PRINTLNF("info menu");
  menu_system.change_menu(info_menu);
  menu_system.change_screen(1);
  menu_system.switch_focus();
  currentMenu = 2;
}

void goTempMenu() {
 PRINTLNF("temp menu");
 readPrefs(); //reading all temp prefs from EEPROM
 menu_system.change_menu(temp_menu);
 menu_system.change_screen(1);
 menu_system.switch_focus();
 currentMenu = 3;
}

void goMainMenu() {
  PRINTLNF("main menu");
  menu_system.switch_focus();
  menu_system.change_menu(main_menu);
  currentMenu = 1;
}

void savePrefsGoHomeScreen() {
  PRINTLNF("homescreen");
  savePrefs();
  menu_system.switch_focus();
  menu_system.change_menu(main_menu);
  updateMainScreenFlag = true;
  clearMainSreenFlag = true;
  currentMenu = 0;
}

void goHomeScreen() {
  PRINTLNF("homescreen");
  menu_system.switch_focus();
  updateMainScreenFlag = true;
  clearMainSreenFlag = true;
  currentMenu = 0;
}

void loadingAnimation(uint8_t count = 1) {
  for (uint8_t i = 0; i < count; i++) {
    PRINTINFO(".");
    static char loading_string[6] = {0};
    lcd.setCursor(11, 1);
    lcd.print(loading_string);
    delay(500);
    char *p = strrchr(loading_string,'.');
    if (p == NULL)
      loading_string[0] = '.';
    else
      *++p = '.';

    if (loading_string[5]=='.')
      strcpy_P(loading_string, PSTR("     ")); //this command "memset(loading_string,0,sizeof(loading_string));" - uses more flash
      
  }
}