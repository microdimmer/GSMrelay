void initMenu() {
  lcd.init(); //init display
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
  main_line1.attach_function(1, go_switch_relay);
  main_line2.attach_function(1, go_temp_menu);
  //main_line3.attach_function(1, func);
  main_line4.attach_function(1, go_info_menu);
  main_line5.attach_function(1, go_main_screen);

 temp_line1.attach_function(1, func);
 temp_line2.attach_function(1, func);
 temp_line3.attach_function(1, func);
 temp_line4.attach_function(1, func);
 temp_line5.attach_function(1, go_main_menu);
  
  info_line1.attach_function(1, func);
  info_line2.attach_function(1, func);
  info_line3.attach_function(1, func);
  info_line4.attach_function(1, go_main_menu);
  // info_line5.attach_function(1, go_main_menu);

  main_line1.set_asProgmem(1); //set PROGMEM menu lines
  main_line2.set_asProgmem(1);
  //main_line3.set_asProgmem(1);
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
  if (clearMainSreenFlag)
  {
    lcd.clear();
    clearMainSreenFlag = false;
  }
  currentMenu = 0;
  updateMainScreenFlag = false;

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

void go_temp_menu() {
 PRINTLNF("changing to temp menu!");
 menu_system.change_menu(temp_menu);
 menu_system.change_screen(1);
 menu_system.switch_focus();
 currentMenu = 3;
}

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