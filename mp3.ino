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
  return digitalRead(MP3_BUSY) == LOW;
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

void playTemp(uint8_t temp_sens_num) { // TODO
  if (temp[temp_sens_num]==-99) { // has no data
    return;
  }
  if (temp_sens_num==0) //play temp source (temp1 or temp2)
    addAudio(35); // home temp
  else
    addAudio(34); // heater temp
    
  if (temp[temp_sens_num]==0) {
    addAudio(28); // //play zero temp
    return;
  }
  bool sign = (temp[temp_sens_num]>0); 
  sign ? addAudio(30) : addAudio(29); //play sign
  uint8_t abs_temp = abs(temp[temp_sens_num]);
  if ((abs_temp > 0) && (abs_temp <= 20)) {
    addAudio(abs_temp); //play temp 1 - 20
  }
  else {
    for (uint8_t i = 2; i < 9; i++) //play temp 21 - 99
    {
      uint8_t first_digit = i*10;
      uint8_t last_digit = first_digit+10;
      if ( (abs_temp >= first_digit) && (abs_temp < last_digit)) {
        uint8_t digits = abs_temp - first_digit;
        addAudio(18+i);
        if (digits !=0 ) addAudio(digits);
      }
    }
  }
  addAudio(31);//play 'degree'
}
