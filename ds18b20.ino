void initDS() {//init temp sensors
  uint8_t i = 0;
  uint8_t resolution = 0;
  while (ds.search(DSaddr[i])) {
    if (i > 1) {
      PRINTLNF("more than 2 devices, unable to load them all");
      break;
    }
    if (OneWire::crc8(DSaddr[i], 7) != DSaddr[i][7]) {
      PRINTLNF("CRC of temp sensor is not valid!");
      break;
    }
    // PRINTLNF("read resolution");
    ds.reset(); //read resolution
    ds.select(DSaddr[i]);
    ds.write(0xBE); // Read Scratchpad
    for (uint8_t j = 0; j < 5; j++) {
      resolution = ds.read(); // we need fifth byte, (resolution) 7F=12bits 5F=11bits 3F=10bits 1F=9bits
    }
    PRINTLNHEX("DS18B20 resolution (0x7F max)=", resolution);
    if (resolution != 0x7f) {
      ds.reset();
      ds.select(DSaddr[i]);
      ds.write(0x4E);       // Write scratchpad command
      ds.write(0);          // TL data
      ds.write(0);          // TH data
      ds.write(0x7F);       // Configuration Register (resolution) 7F=12bits 5F=11bits 3F=10bits 1F=9bits
      ds.reset();           // This "reset" sequence is mandatory
      ds.select(DSaddr[i]); // it allows the DS18B20 to understand the copy scratchpad to EEPROM command
      ds.write(0x48);       // Copy Scratchpad command
    }
    i++;
  }
}

void requestTempUpdateScreen() {//send request to temp sensors
  ds.reset();
  ds.write(0xCC, 0); // skip address (broadcast to all devices)
  ds.write(0x44, 0); // start conversion (start temp measurement)

  timer.setTimeout(200L, readDSresponse);
}

void readDSresponse() {//read response from sensor
  static uint8_t buf[9];
  for (uint8_t i = 0; i < 2; i++) {
    ds.reset();
    ds.select(DSaddr[i]);
    ds.write(0xBE, 0); // read data from DS
    
    ds.read_bytes(buf, 9);
    if (OneWire::crc8(buf, 8) == buf[8] )  // check CRC
      temp[i] = ((int)buf[0] | (((int)buf[1]) << 8)) * 0.0625 + 0.03125; //first and second byte read, convert to int
    else 
      temp[i] = -99; // CRC is not valid
    //temp[i] = ((int)ds.read() | (((int)ds.read()) << 8)) * 0.0625 + 0.03125; //first and second byte read, convert to int, TODO add CRC check
  }

  if (currentMenu == 0 || currentMenu >= 4) //if main screen or set params is displayed,
    updateMainScreenFlag = true;
}
