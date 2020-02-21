void initMP3()
{
  mp3Serial.begin(9600);
  if (mp3Player.begin(mp3Serial))
  { //Use softwareSerial to communicate with mp3.
    PRINTLNF("DFPlayer Mini init OK");
  }
  mp3Player.pause();
}

bool isMP3Busy() {
  return mp3Player.readState() != 512; //resistors! https://github.com/DFRobot/DFRobotDFPlayerMini/issues/20
  //return digitalRead(MP3_BUSY) == LOW;
}

bool addAudio(uint8_t num_track) {
  if (!audioQueue.isFull()) 
    return audioQueue.push(num_track);
  else { PRINTLNF("isFull"); 
    return false;}
}

void playAudio() {
  if (isMP3Busy() || audioQueue.isEmpty())
    return;

    mp3Player.play(audioQueue.shift());
}

void playTemp(uint8_t temp_sens_num) {
  if (temp[temp_sens_num]==-99) // has no data
    return;
  
  if (temp_sens_num==0) { //play temp source (temp1 or temp2)
    addAudio(37); // home temp
    PRINTLNF("PLAY home temp");
    }
  else {
    addAudio(36); // heater temp
    PRINTLNF("PLAY heater temp");
  }
  
  //play zero temp
  uint8_t abs_temp = abs(temp[temp_sens_num]);
  if (abs_temp == 0) {
    addAudio(28); 
  }
  else { //if not zero
    playNumber(abs_temp);
    // uint8_t abs_temp = abs(temp[temp_sens_num]);
    // if ((abs_temp > 0) && (abs_temp <= 20)) {
    //   addAudio(abs_temp); //play temp 1 - 20
    // }
    // else {
    //   for (uint8_t i = 2; i < 9; i++) //play temp 21 - 99
    //   {
    //     uint8_t first_digit = i*10;
    //     uint8_t last_digit = first_digit+10;
    //     if ( (abs_temp >= first_digit) && (abs_temp < last_digit)) {
    //       uint8_t digits = abs_temp - first_digit;
    //       addAudio(18+i);
    //       if (digits !=0 ) addAudio(digits);
    //     }
    //   }
    // }
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
    addAudio(31); //рубль
  }
  else if (first_digit >= 2 && first_digit <=4 && second_digit != 1) {
    addAudio(33);//рубля
  }
  else
    addAudio(32);//рублей
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
