void initMP3() {
  mp3Serial.begin(9600);
  mp3Player.begin(mp3Serial);
  PRINTLNF("trying init DFPlayer");
  mp3Player.volume(30); //set volume value. From 0 to 30
  mp3Player.pause();
}

void playGSM() {
  if (workFlag) { //play switch on/off
    addAudio(34);
    thermostatFlag ? addAudio(55) : addAudio(56); //play switch on/off //play thermostat on/off
  }
  else {
    addAudio(35);
  }
  playTemp(0); //play temp home
  playTemp(1); //play temp heater
  playBalance();
  //playSignalStrength(); //TODO
  playAudio();
}

bool isMP3Busy() {
  return digitalRead(MP3_BUSY) == LOW; //resistors! https://github.com/DFRobot/DFRobotDFPlayerMini/issues/20
}

bool addAudio(uint8_t num_track) {
  if (!audioQueue.isFull()) 
    return audioQueue.push(num_track);
  else { PRINTLNF("audioQueue isFull"); 
    return false;}
}

void playAudio() {
  if (isMP3Busy() ) {
    timer.setTimeout(40L, playAudio);
    return;
  }
  if (audioQueue.isEmpty()) {
    PRINTLNF("audioQueue isEmpty"); 
    if (GSMonAirFlag)
      hangUpGSM();
    return; 
  }
//  PRINTLN("audioQueue ",audioQueue.size());
  mp3Player.play(audioQueue.shift());
  timer.setTimeout(40L, playAudio);
}

void playTemp(uint8_t temp_sens_num) {
  if (temp[temp_sens_num]==-99) // has no data
    return;
  
  if (temp_sens_num==0) { //play temp source (temp1 or temp2)
    addAudio(37); // home temp
    }
  else {
    addAudio(36); // heater temp
  }
  
  //play zero temp
  uint8_t abs_temp = abs(temp[temp_sens_num]);
  if (abs_temp == 0) {
    addAudio(28); 
  }
  else { //if not zero
    playNumber(abs_temp);
  }

  //play 'degree'
  uint8_t first_digit = abs_temp % 10;
  uint8_t second_digit = (abs_temp / 10) % 10;
  if ((abs_temp != 11) && (first_digit == 1)) {
    addAudio(31);// градус
  }
  else if (first_digit >= 2 && first_digit <=4 && second_digit != 1) {
    addAudio(33);// градуса
  }
  else
    addAudio(32);// градусов
  
  //play sign
  if (temp[temp_sens_num] > 0) //if zero - not playing sign
    addAudio(30);
  else if (temp[temp_sens_num] < 0)  
    addAudio(29); 

}

void playBalance() {
  if (balance == -32768) //no data,  int16_t balance = -32768; 
    return;

  if (balance < 0) //play sign 
    addAudio(29);   
  
  addAudio(38); // play 'balance'

  uint8_t abs_balance = abs(balance);
  playNumber(abs_balance);
  //play 'ruble'
  uint8_t two_first_digit = abs_balance % 100;
  uint8_t first_digit = two_first_digit % 10;
  uint8_t second_digit = (two_first_digit / 10) % 10;
  if ((two_first_digit != 11) && (first_digit == 1)) {
    addAudio(39); //рубль
  }
  else if (first_digit >= 2 && first_digit <=4 && second_digit != 1) {
    addAudio(41);//рубля
  }
  else
    addAudio(40);//рублей
}

void playNumber(uint16_t num) {
  //play zero temp
  if (num==0) {
    addAudio(28);
    return; 
  }
  else { //if not zero
    
    //play hundreds
    if (num/100 > 0)
      addAudio(45+(num / 100));

    //play tens and units
    if ((num > 0) && (num <= 20)) {
      addAudio(num); //play temp 1 - 20
    }
    else {
      for (uint8_t i = 2; i < 9; i++) //play temp 21 - 99
      {
        uint8_t first_digit = i*10;
        uint8_t last_digit = first_digit+10;
        if ( (num >= first_digit) && (num < last_digit)) {
          uint8_t digits = num - first_digit;
          addAudio(18+i);
          if (digits !=0 ) addAudio(digits);
        }
      }
    }
  }  
}
