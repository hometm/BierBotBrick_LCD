# LCD Display for BierBot Bricks

BierBot Bricks https://bricks.bierbot.com/

## LCD Display

Display for visualizatzion of current brew process.

Features:
- Short overview of current/next step
- Time information
- Current Temperature and setpoint

## Hardware

### Hardware Components

Bill of Material (Conrad Electronic Feb/2022):

- GEHÄUSE KG 200 
Bestell-Nr.: 540986 
 
- SBH341-1A Enclosed Battery Box 4 x AA 
Bestell-Nr.: 1672572 
 
- DRUCKSCHALTER R13-23B-05 SW TASTE TC 
Bestell-Nr.: 1587699 
 
- PIEZO-SIGNALGEBER EFH1205 
Bestell-Nr.: 2300515 
 
- LED 10MM KLAR ROT 1000MCD 
Bestell-Nr.: 1577504

- Joy-IT LCD Display, 20x4, Blau 
Bestell-Nr.: 1503752 

- NodeMCU V2 LUA ESP8266
Bestell-Nr.: 1613301



Spare part for future extensions:
 
- DRUCKTASTER SCHWARZ QUAD 
Bestell-Nr.: 706175 


...Heat shrink tubing, stranded wire, series resistor, solder, soldering iron 
 
 

## Hardware Assembly
...draft will be added



## Software Programming/Loading
Arduino V2.0.0-rc7 (incl. esp32 hardware catalogue and necessary libs)

Three parameters must be updated inside source:
- Brick API Key
- Wifi SSID
- Wifi password

## Possible improvements

- LED and Buzzer included. Can be used for Alarming
- Implementation of Wifi Manager to avoid hard coded Wifi credentials
- OTA Programming (if bugs inside Arduino IED are closed)
- Configuration of Brick API key outside of source