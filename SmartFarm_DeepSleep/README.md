# SmartFarm_DeepSleep Project for ESP8266, ESP32

## SmartFarm_DeepSleep demonstrates how to use DeepSleep in ESP32 and ESP8266

In the sample project, you can use these features:

1. Deep Sleep in ESP8266 and ESP32, with configurables DeepSleep time. 
2. Configurable sensor type, without having to rewrite code such as DHT type (DHT11,DHT22, AM2302, MW33, etc.), Soil Moist Sensor type (Capacitive, Resistive). 
3. Many configurable parameters, such as alarm setpoint, pump running setpoint, etc...

## Menu-based Configurable Parameters
1. Degree: 
  a. Celcius
  b. Fahrenheit

2. Pump Mode
  a. Auto & Notice
  b. Auto
  c. Manual & Notice
  d. Manual

3. DHT Type
  a. Autu Detect
  b. DHT11
  c. MW33
  d. DHT22
  e. AM2302
  f. RHT03

4. Moist Alarm Interval
  a. No Alarm
  b. Every 1 hour
  c. Every 6 hrs
  d. Every 24 hrs

5. Soil Moist Sensor Type
  a. Resistive
  b. Capacitive
  
## Other configurable parameters

1.  Minimum Air Temperature Alarm
2.  Maximum Air Temperature Alarm
3.  DHT Adjustment %
4.  Moist Adjustment %
5.  Minimum Air Humidity Alarm
6.  Maximum Air Humidity Alarm
7.  Dry Soil Humidity % for Running Pump
8.  Time Pump Running in seconds
9.  Dry Soil Humidity % Alarm
10. Wet Soil Humidity % Alarm
11. Interval between DeepSleep in minutes
12. DeepSleep Time in minutes

## Control Buttons
1. Restart
2. Force DeepSleep

## To improve

Use ISR-based hardware timers instead of software timers so that the pump functions is ISR-based.

## Contributing
If you want to contribute to this project:
- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library

## Copyright
Copyright 2019- Khoi Hoang
