# How to flash the board

To flash the Jade M1 board you need to use an AVR programmer. As such, we will use AVRDUDE to flash out binaries onto the board.

## Getting the binaries

To get the compiled binaries, go to the [Release](https://github.com/LordR31/Voltmeter-with-Waveform-Viewer/releases) tab on the Github Repository and download the .hex file.

## Using AVRDUDE
To falsh the binaries onto the board we will use AVRDUDE, in Windows Terminal. AVRDUDE needs to be set up before usage. The set up process can be seen [here](avrdude.md). After setting up, we will use the following command:

avrdude -C "C:\avrdude\avrdude.conf" -c wiring -p atmega2560 -P COM4 -b 115200 -D -U flash:w:"firmware.hex":i

The COM port needs to be changed to match the actual COM Port used by the board. For ease of usage, this command can be stores inside a bat file, as such:

@echo off
avrdude -C "C:\avrdude\avrdude.conf" -c wiring -p atmega2560 -P COM4 -b 115200 -D -U flash:w:"firmware.hex":i

If the hex file name is changed, or AVRDUDE will be used with a different binary, firmware.hex can be replaces with %1.hex. This will allow the user to call the .bat file with the binary name, so that you do not have to write and change the command every flash.

After saving the .bat file, it can be simply called from terminal with

.\flash.bat or, if using the "%1.hex" syntax, .\flash.bat name.hex.


## AVRDUDE Flags Explained

|Flag|Meaning|
|---|--------|
|-C:|Specifies the path to the configuration file (avrdude.conf).|
|-c|Defines the programmer type. wiring is used for boards like Arduino Mega. Jade M1 uses ATMega2560|
|-p|Specifies the target microcontroller. In this case, it's the ATmega2560.|
|-P|Sets the communication port. COM4 is the serial port connected to the board. This needs to be changed based on actual port used.|
|-b|Sets the baud rate for communication. 115200 is typical for Arduino Mega uploads.|
|-D|Disables auto erase before writing flash. Useful for preserving EEPROM or speeding up uploads.|
|-U flash:w:"firmware.hex":i|Uploads (w) the file firmware.hex to the flash memory. The :i indicates Intel Hex format.|