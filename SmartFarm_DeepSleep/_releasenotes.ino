/*
 * 
 ************************
   1.0.4 2020-07-07
 **********************
   - Use new Blynk_WM v1.0.16+ with new USE_DYNAMIC_PARAMETERS and LOAD_DEFAULT_CONFIG_DATA
   - TO DO:
   - 1) Wake up when moist is low by using GPIO
 ***********************
   1.0.3 2020-01-07
 **********************
   - Use Blynk_WM v1.0.4 with personalized DHCP hostname
   - TO DO:
   - 1) Wake up when moist is low by using GPIO
 ***********************
   1.0.2a 2019-10-20
 **********************
   - Use Homemade WiFiManager included in Blynk Classes : BlynkSimpleEsp8266_WM.h, BlynkSimpleEsp8266_SSL_WM.h, BlynkSimpleEsp32_WM.h and BlynkSimpleEsp32_SSL_WM.h
   - Correct polarity for LED_BUILDIN in ESP32. Move to activate LED_BUILDIN flashing only while connected to Blynk.
   - TO DO:
   - 1) Wake up when moist is low by using GPIO  
 ***********************
   1.0.1d 2019-10-16
 **********************
   - Change default DEEPSLEEP_INTERVAL_FACTOR = 0L => no deepsleep
   - Delete Serial.printf in BLYNK_WRITE to avoid Blynk flooding and reset
   - Fix deepsleep logic in check_status() to update deepsleep_timeout when first reset/wakeup or changing DEEPSLEEP_INTERVAL_FACTOR or 
      Time has been synchronized / Blynk just connected
   - Store DEEPSLEEP_INTERVAL_FACTOR, DEEPSLEEP_INTERVAL and important params in RTC RAM or SPIFFS and retrieve when first boot/reset/wakeup   
   - TO DO:
   - 1) Don't use hardcode auth, etc. Use WifiManager (similar to SmartGarage)
   - 2) Wake up when moist is low by using GPIO 
 ***********************
   1.0.1c 2019-10-07
 **********************
   - Use Arduino 1.8.10 and new libraries DHTesp
   - TO DO:
   - 1) Don't use hardcode auth, etc. Use WifiManager (similar to SmartGarage)
   - 2) Wake up when moist is low by using GPIO
 ***********************
   1.0.1b 2019-09-28
 **********************
   - Bug fix. Use uint64_t for uS_TO_SLEEP ( ulong = uint32_t will be overflowed)
   - Add bootCount for ESP8266
        ESP.rtcUserMemoryWrite(offset, &data, sizeof(data)) and ESP.rtcUserMemoryRead(offset, &data, sizeof(data)) allow data to be stored in and 
        retrieved from the RTC user memory of the chip respectively. offset is measured in blocks of 4 bytes and can range from 0 to 127 blocks 
        (total size of RTC memory is 512 bytes). data should be 4-byte aligned. The stored data can be retained between deep sleep cycles, but might 
        be lost after power cycling the chip. Data stored in the first 32 blocks will be lost after performing an OTA update, because they are used by 
        the Core internals.   
   - TO DO:
   - 1) Don't use hardcode auth, etc. Use WifiManager (similar to SmartGarage)
   - 2) Wake up when moist is low by using GPIO
 ***********************
   1.0.1a 2019-09-25
 **********************
   - Add DeepSleep support for ESP8266
   - TO DO:
   - 1) Don't use hardcode auth, etc. Use WifiManager (similar to SmartGarage)
   - 2) Wake up when moist is low by using GPIO       
 ***********************
   1.0.0c 2019-09-24
 **********************
   - Add SSL support for ESP32/ESP8266 for OpenSSL. Using same code, only change in Blynk Library certs files
   - TO DO:
   - 1) Don't use hardcode auth, etc. Use WifiManager (similar to SmartGarage)
   - 2) Wake up when moist is low by using GPIO
 **********************
   1.0.0b 2019-09-18
 **********************
   - Add SSL support for ESP32/ESP8266 for LetEncrypt
   - TO DO:
   - 1) Don't use hardcode auth, etc. Use WifiManager (similar to SmartGarage)
   - 2) Wake up when moist is low by using GPIO
 **********************
   1.0.0a 2019-09-18
 **********************
   - First version for deepsleep
   - TO DO:
   - 1) Don't use hardcode auth, etc. Use WifiManager (similar to SmartGarage)
   - 2) Wake up when moist is low by using GPIO
*/
