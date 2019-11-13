# Arduino GSM relay with temp sensors

GSM relay with temp automation with two sensors, time-synchronization and audio notification. 

## Components

* GSM/GPRS AiThinker A6 
* Arduino Pro Mini, Atmega 328p, 5V/16MHz
* I2C 1602 16X2 Character LCD Module
* Two ds18b20 temperature sensors
* DFPlayer mini MP3 module
* Relay module
* Rotary encoder switch EC11
* Resistors, wires, enclosure

## Photos:
![PHOTO1](https://github.com/microdimmer/GSMrelay/blob/master/readme/front.jpg)
![PHOTO2](https://github.com/microdimmer/GSMrelay/blob/master/readme/disassembled.jpg)

## Libraries

* [Arduino-esp8266](https://github.com/esp8266/Arduino)
* [Forked menu library for 1602](https://github.com/microdimmer/LiquidMenu)
* [Russian support for 1602](https://github.com/ssilver2007/LCD_1602_RUS)
* [I2C support for 1602](https://github.com/johnrickman/LiquidCrystal_I2C)
* [Memory free library for debuging](https://github.com/maniacbug/MemoryFree)
* [Simple timer library](https://github.com/jfturcot/SimpleTimer)
* [Timekeeping library](https://github.com/PaulStoffregen/Time)
* [Library to handle rotary encoder with button](https://github.com/0xPIT/encoder)
* [DFPlayer mini module library](https://github.com/DFRobot/DFRobotDFPlayerMini)
* [OneWire library](https://github.com/PaulStoffregen/OneWire)

## Scheme:
![СХЕМА](https://github.com/microdimmer/GSMrelay/blob/master/readme/scheme.png)

## Known issues:

* issue 1


# Russian:
GSM relay with temp automation with two sensors, time-synchronization and audio notification. 
GSM реле c термостатом, двумя датчиками температуры, автоматизацией по времени и температуре и аудио оповещениями.

## Компоненты

* Плата GSM/GPRS AiThinker A6 
* Arduino Pro Mini, Atmega 328p, 5V/16MHz
* Монохромный дисплей I2C 1602 16X2 Character LCD
* Датчик температуры ds18b20 x2
* Модуль воспроизведения MP3 DFPlayer mini 
* Реле 5v
* Энкодер с кнопкой EC11
* кнопки, резисторы, провода, корпус

## Библиотеки

* [Arduino-esp8266](https://github.com/esp8266/Arduino)
* [Forked menu library for 1602](https://github.com/microdimmer/LiquidMenu)
* [Russian support for 1602](https://github.com/ssilver2007/LCD_1602_RUS)
* [I2C support for 1602](https://github.com/johnrickman/LiquidCrystal_I2C)
* [Memory free library for debuging](https://github.com/maniacbug/MemoryFree)
* [Simple timer library](https://github.com/jfturcot/SimpleTimer)
* [Timekeeping library](https://github.com/PaulStoffregen/Time)
* [Library to handle rotary encoder with button](https://github.com/0xPIT/encoder)
* [DFPlayer mini module library](https://github.com/DFRobot/DFRobotDFPlayerMini)
* [OneWire library](https://github.com/PaulStoffregen/OneWire)


## Известные проблемы:

* Проблема 1
