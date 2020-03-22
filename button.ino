void readEncoder() {
  encoder.service();
  enc_val += encoder.getValue(); //read encoder
  if (enc_val == last_val)
    return;
  backlightON();
  if (currentMenu == 0) { // if main screen return
    last_val = enc_val; //reset encoder values
    return;
  }
  
  if ((max(enc_val, last_val) - min(enc_val, last_val)) >= 2)  { //if difference more than 2 -> one turn
    if (last_val > enc_val) {
      PRINTLNF("enc ++");

      if (currentMenu ==4 ) { // set home temp
        t_home_set++;
      }
      else if (currentMenu ==5 ) { // set heater temp
        t_heater_set++;
      }
      else if (currentMenu ==6 ) { // set home temp hysteresis
        t_home_hysteresis_set++;
      }
      else if (currentMenu ==7 ) { // set set heater temp hysteresis
        t_heater_hysteresis_set++;
      }
      else if (!menu_system.switch_focus()) { // end of menu lines
        menu_system++;
        menu_system.switch_focus();
      }
    }
    else {
      PRINTLNF("enc --");
     if (currentMenu ==4 ) { // set home temp
        t_home_set--;
      }
      else if (currentMenu ==5 ) { // set heater temp
        t_heater_set--;
      }
      else if (currentMenu ==6 ) { // set home temp hysteresis
        t_home_hysteresis_set--;
      }
      else if (currentMenu ==7 ) { // set set heater temp hysteresis
        t_heater_hysteresis_set--;
      }
      else if (!menu_system.switch_focus(false)) { // end of menu lines
        menu_system--;
        menu_system.switch_focus(false);
      }
    }
    last_val = enc_val;
    PRINTLNF("_");
  }
}

void readButton()
{
  encoder.service();
  button = encoder.getButton();
  if (button != ClickEncoder::Open) {
    PRINTLNF("button clicked");
    backlightON();
    if (currentMenu == 0) { //homepagego to main menu
      menu_system.change_menu(main_menu);
      menu_system.change_screen(1);
      menu_system.switch_focus();
      updateMainScreenFlag = false;
      currentMenu = 1; //change to main menu
    }
    else { // go to attached to menu function
      menu_system.call_function(1);
    }
  }
}