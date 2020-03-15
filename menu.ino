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
  main_line2.attach_function(1, func);
  //main_line3.attach_function(1, func);
  main_line4.attach_function(1, go_info_menu);
  main_line5.attach_function(1, go_main_screen);

 temp_line1.attach_function(1, func);
 temp_line2.attach_function(1, func);
 temp_line3.attach_function(1, func);
 temp_line4.attach_function(1, func);
 temp_line5.attach_function(1, go_main_screen);
  
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