# Arduino GSM relay with ring controlling and temp sensors

GSM relay with two temperature sensors, GSM time-synchronization, ring controlling and audio notification. 

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

## Libraries

* [Arduino](https://github.com/arduino/Arduino)
* [Forked menu library for 1602](https://github.com/microdimmer/LiquidMenu)
* [I2C support for 1602](https://github.com/johnrickman/LiquidCrystal_I2C)
* [Memory free library for debuging](https://github.com/maniacbug/MemoryFree)
* [Simple timer library](https://github.com/marcelloromani/Arduino-SimpleTimer)
* [Timekeeping library](https://github.com/PaulStoffregen/Time)
* [Library to handle rotary encoder with button](https://github.com/0xPIT/encoder)
* [DFPlayer mini module library](https://github.com/DFRobot/DFRobotDFPlayerMini)
* [OneWire library](https://github.com/PaulStoffregen/OneWire)

## Scheme:
![СХЕМА](https://github.com/microdimmer/GSMrelay/blob/master/readme/scheme.png)

## Known issues:

* Relay creates electromagnetic interference which affects the I2C display driver, had to install ferrite bead on I2C data wires.


# Russian:
GSM реле c термостатом, двумя датчиками температуры, автоматизацией по температуре, активацией по телефону и аудио оповещениями.

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

* [Arduino](https://github.com/arduino/Arduino)
* [Forked menu library for 1602](https://github.com/microdimmer/LiquidMenu)
* [I2C support for 1602](https://github.com/johnrickman/LiquidCrystal_I2C)
* [Memory free library for debuging](https://github.com/maniacbug/MemoryFree)
* [Simple timer library](https://github.com/marcelloromani/Arduino-SimpleTimer)
* [Timekeeping library](https://github.com/PaulStoffregen/Time)
* [Library to handle rotary encoder with button](https://github.com/0xPIT/encoder)
* [DFPlayer mini module library](https://github.com/DFRobot/DFRobotDFPlayerMini)
* [OneWire library](https://github.com/PaulStoffregen/OneWire)


## Известные проблемы:

* При переключении GSM-реле на дисплее наблюдались крякозябры из-за электромагнитных помех, пришлось поставить ферритовый фильтры на шину I2C.
