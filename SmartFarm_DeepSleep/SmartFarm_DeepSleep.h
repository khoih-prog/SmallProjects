/****************************************************************************************************************************
   SmartFarm_DeepSleep.ino
   SmartFarm for ESP32 and ESP8266, using configurable DeepSleep, and many other parameters
   For ESP8266 / ESP32 boards
   Written by Khoi Hoang
   Copyright (c) 2019 Khoi Hoang

   Built by Khoi Hoang https://github.com/khoih-prog/SmallProjects/SmartFarm_DeepSleep
   Licensed under MIT license
   Version: 1.0.2

   Now we can use these new 16 ISR-based timers, while consuming only 1 hardware Timer.
   Their independently-selected, maximum interval is practically unlimited (limited only by unsigned long miliseconds)
   The accuracy is nearly perfect compared to software timers. The most important feature is they're ISR-based timers
   Therefore, their executions are not blocked by bad-behaving functions / tasks.
   This important feature is absolutely necessary for mission-critical tasks.

   Notes:
   Special design is necessary to share data between interrupt code and the rest of your program.
   Variables usually need to be "volatile" types. Volatile tells the compiler to avoid optimizations that assume
   variable can not spontaneously change. Because your function may change variables while your program is using them,
   the compiler needs this hint. But volatile alone is often not enough.
   When accessing shared variables, usually interrupts must be disabled. Even with volatile,
   if the interrupt changes a multi-byte variable between a sequence of instructions, it can be read incorrectly.
   If your data is multiple variables, such as an array and a count, usually interrupts need to be disabled
   or the entire sequence of your code which accesses the data.

   Version Modified By   Date      Comments
   ------- -----------  ---------- -----------
    1.0.0   K Hoang     18/09/2019 Initial coding for ESP32
    1.0.1   K Hoang     25/09/2019 Add ESP8266 and SSL support
    1.0.2   K Hoang     20/10/2019 Use Blynk_WM for easy management and test
*****************************************************************************************************************************/

/* For ESP32:
   Compile using: ESP32 Dev Module, 921600, 160MHz, 4M (1M SPIFFS), QIO, Flash 80MHz
   For ESP32 SSL, use <certs/letsencrypt_pem.h> for BLYNK_SSL_USE_LETSENCRYPT and <certs/blynkcloud_pem.h> if NO BLYNK_SSL_USE_LETSENCRYPT
     #if defined(BLYNK_SSL_USE_LETSENCRYPT)
      static const char BLYNK_DEFAULT_ROOT_CA[] =
      #include <certs/letsencrypt_pem.h>
    #else
      static const char BLYNK_DEFAULT_ROOT_CA[] =
      #include <certs/blynkcloud_pem.h>
    #endif

   To create file blynkcloud_pem.h/letsencrypt_pem.h from localServer certificate file fullchain.crt (copy and add ",",\n,;
   similar way to original file. This file is OK to use (both blynkcloud_pem.h/letsencrypt_pem.h are the same)
   even if BLYNK_SSL_USE_LETSENCRYPT is defined or not.

   For ESP8266-based:

   ESP8266 DEEPSLEEP

   1) To put the ESP8266 in deep sleep mode, use ESP.deepSleep(uS) and pass as argument sleep time in microseconds.
   D0/GPIO16 must be connected to reset (RST) pin so the ESP8266 is able to wake up.
   To put the ESP8266 in deep sleep mode for an indefinite period of time use ESP.deepSleep(0).
   The ESP8266 will wake up when the RST pin receives a LOW signal.

   2) Uploading program (IMPORTANT !!!)
   To upload program in middle of deepsleep, or after power off/on while in deepsleep
   We have to press RST button to wake the ESP8266 up to communicate with programmer
   or remove the jumper wire between RST and WAKE D0/GPIO16
*/

#define BLYNK_PRINT Serial

// Not use #define USE_SPIFFS  => using EEPROM for configuration data in WiFiManager
// #define USE_SPIFFS    false => using EEPROM for configuration data in WiFiManager
// #define USE_SPIFFS    true  => using SPIFFS for configuration data in WiFiManager
// Be sure to define USE_SPIFFS before #include <BlynkSimpleEsp8266_WM.h>

#define USE_SPIFFS    true
//#define USE_SPIFFS    false

#define USE_BLYNK_WM    true            // https://github.com/khoih-prog/Blynk_WM
//#define USE_BLYNK_WM    false

//Ported to ESP32
//#define USE_ESP32         true
#define USE_ESP32         false

#if USE_ESP32
#ifdef ESP8266
#undef ESP8266
#define ESP32
#endif
#endif

#if USE_ESP32
//#include <SPIFFS.h>
#include <esp_wifi.h>
#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

#else
#define ESP_getChipId()   (ESP.getChipId())
#endif

//#define USE_SSL     false
#define USE_SSL     true

#if USE_SSL
//#define BLYNK_SSL_USE_LETSENCRYPT       true

#if USE_BLYNK_WM

#else
// SSL can't use server 192.168.x.x, but OK with khoih.duckdns.org because Certificate generated only for khoih.duckdns.org
//
String cloudBlynkServer = "yourname.duckdns.org";
#define BLYNK_SERVER_HARDWARE_PORT    9443
#endif
#else
#if USE_BLYNK_WM

#else
String cloudBlynkServer = "yourname.duckdns.org";
//String cloudBlynkServer = "192.168.x.x";
#define BLYNK_SERVER_HARDWARE_PORT    8080
#endif
#endif

#if USE_ESP32

#include <WiFi.h>

#if USE_SSL
#include <WiFiClientSecure.h>
#if USE_BLYNK_WM
#include <BlynkSimpleEsp32_SSL_WM.h>                // https://github.com/khoih-prog/Blynk_WM
#else
#include <BlynkSimpleEsp32_SSL.h>
#endif
#else
#include <WiFiClient.h>
#if USE_BLYNK_WM
#include <BlynkSimpleEsp32_WM.h>                    // https://github.com/khoih-prog/Blynk_WM
#else
#include <BlynkSimpleEsp32.h>
#endif
#endif

#else   //USE_ESP32

#if USE_SSL
#include <WiFiClientSecure.h>

#if USE_BLYNK_WM
#include <BlynkSimpleEsp8266_SSL_WM.h>          // https://github.com/khoih-prog/Blynk_WM
#else
#include <BlynkSimpleEsp8266_SSL.h>
#endif
#else
#include <ESP8266WiFi.h>

#if USE_BLYNK_WM
#include <BlynkSimpleEsp8266_WM.h>                // https://github.com/khoih-prog/Blynk_WM
#else
#include <BlynkSimpleEsp8266.h>
#endif

#endif


#endif    //USE_ESP32

#include <WidgetRTC.h>

// Use DHT for ESPx library by beegee_tokyo   (https://github.com/beegee-tokyo/DHTesp)
#include <DHTesp.h>
#include <Ticker.h>

#define FWV    102    // Firmware version: 101 means 1.0.1

// Firmware sub version:  a means x.x.x.a, add p for D1 Mini Pro (160MHz, 16M(14M SPIFFS)), d for D1 Mini (160MHz), t for testing
//                        w for ESP32, s for SSL enabled

#define FWSV   "a"

#if USE_ESP32

//See file .../hardware/espressif/esp32/variants/(esp32|doitESP32devkitV1)/pins_arduino.h
#define LED_BUILTIN       2         // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED
#define PIN_LED           2         // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED

#define PIN_D0            0         // Pin D0 mapped to pin GPIO0/BOOT/ADC11/TOUCH1 of ESP32
#define PIN_D1            1         // Pin D1 mapped to pin GPIO1/TX0 of ESP32
#define PIN_D2            2         // Pin D2 mapped to pin GPIO2/ADC12/TOUCH2 of ESP32
#define PIN_D3            3         // Pin D3 mapped to pin GPIO3/RX0 of ESP32
#define PIN_D4            4         // Pin D4 mapped to pin GPIO4/ADC10/TOUCH0 of ESP32
#define PIN_D5            5         // Pin D5 mapped to pin GPIO5/SPISS/VSPI_SS of ESP32
#define PIN_D6            6         // Pin D6 mapped to pin GPIO6/FLASH_SCK of ESP32
#define PIN_D7            7         // Pin D7 mapped to pin GPIO7/FLASH_D0 of ESP32
#define PIN_D8            8         // Pin D8 mapped to pin GPIO8/FLASH_D1 of ESP32
#define PIN_D9            9         // Pin D9 mapped to pin GPIO9/FLASH_D2 of ESP32

#define PIN_D10           10        // Pin D10 mapped to pin GPIO10/FLASH_D3 of ESP32
#define PIN_D11           11        // Pin D11 mapped to pin GPIO11/FLASH_CMD of ESP32
#define PIN_D12           12        // Pin D12 mapped to pin GPIO12/HSPI_MISO/ADC15/TOUCH5/TDI of ESP32
#define PIN_D13           13        // Pin D13 mapped to pin GPIO13/HSPI_MOSI/ADC14/TOUCH4/TCK of ESP32
#define PIN_D14           14        // Pin D14 mapped to pin GPIO14/HSPI_SCK/ADC16/TOUCH6/TMS of ESP32
#define PIN_D15           15        // Pin D15 mapped to pin GPIO15/HSPI_SS/ADC13/TOUCH3/TDO of ESP32
#define PIN_D16           16        // Pin D16 mapped to pin GPIO16/TX2 of ESP32
#define PIN_D17           17        // Pin D17 mapped to pin GPIO17/RX2 of ESP32     
#define PIN_D18           18        // Pin D18 mapped to pin GPIO18/VSPI_SCK of ESP32
#define PIN_D19           19        // Pin D19 mapped to pin GPIO19/VSPI_MISO of ESP32

#define PIN_D21           21        // Pin D21 mapped to pin GPIO21/SDA of ESP32
#define PIN_D22           22        // Pin D22 mapped to pin GPIO22/SCL of ESP32
#define PIN_D23           23        // Pin D23 mapped to pin GPIO23/VSPI_MOSI of ESP32
#define PIN_D24           24        // Pin D24 mapped to pin GPIO24 of ESP32
#define PIN_D25           25        // Pin D25 mapped to pin GPIO25/ADC18/DAC1 of ESP32
#define PIN_D26           26        // Pin D26 mapped to pin GPIO26/ADC19/DAC2 of ESP32
#define PIN_D27           27        // Pin D27 mapped to pin GPIO27/ADC17/TOUCH7 of ESP32     

#define PIN_D32           32        // Pin D32 mapped to pin GPIO32/ADC4/TOUCH9 of ESP32
#define PIN_D33           33        // Pin D33 mapped to pin GPIO33/ADC5/TOUCH8 of ESP32
#define PIN_D34           34        // Pin D34 mapped to pin GPIO34/ADC6 of ESP32

//Only GPIO pin < 34 can be used as output. Pins >= 34 can be only inputs
//See .../cores/esp32/esp32-hal-gpio.h/c
//#define digitalPinIsValid(pin)          ((pin) < 40 && esp32_gpioMux[(pin)].reg)
//#define digitalPinCanOutput(pin)        ((pin) < 34 && esp32_gpioMux[(pin)].reg)
//#define digitalPinToRtcPin(pin)         (((pin) < 40)?esp32_gpioMux[(pin)].rtc:-1)
//#define digitalPinToAnalogChannel(pin)  (((pin) < 40)?esp32_gpioMux[(pin)].adc:-1)
//#define digitalPinToTouchChannel(pin)   (((pin) < 40)?esp32_gpioMux[(pin)].touch:-1)
//#define digitalPinToDacChannel(pin)     (((pin) == 25)?0:((pin) == 26)?1:-1)

#define PIN_D35           35        // Pin D35 mapped to pin GPIO35/ADC7 of ESP32
#define PIN_D36           36        // Pin D36 mapped to pin GPIO36/ADC0/SVP of ESP32
#define PIN_D39           39        // Pin D39 mapped to pin GPIO39/ADC3/SVN of ESP32

#define PIN_RX0            3        // Pin RX0 mapped to pin GPIO3/RX0 of ESP32
#define PIN_TX0            1        // Pin TX0 mapped to pin GPIO1/TX0 of ESP32

#define PIN_SCL           22        // Pin SCL mapped to pin GPIO22/SCL of ESP32
#define PIN_SDA           21        // Pin SDA mapped to pin GPIO21/SDA of ESP32  

#else //USE_ESP32

//PIN_D0 can't be used for PWM/I2C
#define PIN_D0            16        // Pin D0 mapped to pin GPIO16/USER/WAKE of ESP8266. This pin is also used for Onboard-Blue LED. PIN_D0 = 0 => LED ON
#define PIN_D1            5         // Pin D1 mapped to pin GPIO5 of ESP8266
#define PIN_D2            4         // Pin D2 mapped to pin GPIO4 of ESP8266
#define PIN_D3            0         // Pin D3 mapped to pin GPIO0/FLASH of ESP8266
#define PIN_D4            2         // Pin D4 mapped to pin GPIO2/TXD1 of ESP8266
#define PIN_LED           2         // Pin D4 mapped to pin GPIO2/TXD1 of ESP8266, NodeMCU and WeMoS, control on-board LED
#define PIN_D5            14        // Pin D5 mapped to pin GPIO14/HSCLK of ESP8266
#define PIN_D6            12        // Pin D6 mapped to pin GPIO12/HMISO of ESP8266
#define PIN_D7            13        // Pin D7 mapped to pin GPIO13/RXD2/HMOSI of ESP8266
#define PIN_D8            15        // Pin D8 mapped to pin GPIO15/TXD2/HCS of ESP8266

#endif    //USE_ESP32

#if USE_ESP32

//#define PIN_RESET         PIN_D0    //16
#define DHTPIN            PIN_D22            // pin DATA @ D22 / GPIO22
#define PUMP_RELAY_PIN    PIN_D23            // PUMP RELAY @ D21
#define SOIL_MOIST_PIN    PIN_D32            // pin GPIO32 for analog input

#else   //USE_ESP32

//#define PIN_RESET         PIN_D0    //16
#define DHTPIN            PIN_D2            // pin DATA @ D2
#define PUMP_RELAY_PIN    PIN_D1            // PUMP RELAY @ D1
#define SOIL_MOIST_PIN    A0                // pin A0 with A0

#endif    //USE_ESP32

#define RELAY_ACTIVE_HIGH     true

#define RSSI_WEAK             -81
#define RSSI_OK               -71

#define RSSI_0_PERCENT        -105
#define RSSI_100_PERCENT      -30

#define RUN                   true
#define STOP                  false

#define BLYNK_PIN_PUMP_ON       V0
#define BLYNK_PIN_PUMP_OFF      V1
#define BLYNK_PIN_PUMP_BUTTON   V2
#define BLYNK_PIN_TEMP          V3
#define BLYNK_PIN_HUMID         V4
#define BLYNK_PIN_MOIST         V5
#define BLYNK_PIN_LCD           V6
#define BLYNK_PIN_TEMPCHART     V7

#define BLYNK_PIN_IP            V8
#define BLYNK_PIN_RSSI          V9
#define BLYNK_PIN_FWV           V10
#define BLYNK_PIN_SSID          V11
#define BLYNK_PIN_HEAT_INDEX    V12

#define BLYNK_PIN_MIN_AIR_TEMP          V20
#define BLYNK_PIN_MAX_AIR_TEMP          V21
#define BLYNK_PIN_MIN_AIR_HUMIDITY      V22
#define BLYNK_PIN_MAX_AIR_HUMIDITY      V23

#define BLYNK_PIN_DRY_SOIL              V24
#define BLYNK_PIN_WET_SOIL              V25

// Time pump ON when DRY, in seconds, selected from 10-50secs
#define BLYNK_PIN_TIME_PUMP_ON          V26
#define BLYNK_PIN_USE_CELCIUS           V27
#define BLYNK_PIN_DHT_TYPE              V28
#define BLYNK_PIN_DHT_ADJ_FACTOR        V29
#define BLYNK_PIN_PUMP_MODE             V30
#define BLYNK_PIN_MOIST_ALARM_INTERVAL  V31
#define BLYNK_PIN_MOIST_ALARM_LEVEL     V32
#define BLYNK_PIN_MOIST_ADJ_FACTOR      V33
#define BLYNK_PIN_MOIST_SENSOR_TYPE     V34

#define BLYNK_PIN_DEEPSLEEP_INTERVAL_FACTOR     V35
#define BLYNK_PIN_TIME_TO_DEEPSLEEP             V36
#define BLYNK_PIN_FORCE_DEEPSLEEP               V37
#define BLYNK_PIN_DEEPSLEEP_BOOTCOUNT           V38

#define BLYNK_PIN_RESET                 V40

// For deepSleep
#define uS_TO_S_FACTOR        1000000L     /* Conversion factor for micro seconds to seconds */
#define uS_TO_MIN_FACTOR      60000000L    /* Conversion factor for micro seconds to minutes */

//Blynk Color in format #RRGGBB
#define BLYNK_GREEN     "#23C48E"
#define BLYNK_BLUE      "#04C0F8"
#define BLYNK_YELLOW    "#ED9D00"
//#define BLYNK_RED       "#D3435C"
#define BLYNK_RED       "#FF0000"
#define BLYNK_DARK_BLUE "#5F7CD8"

#define SERIAL_DEBUG
#if defined(SERIAL_DEBUG)

#define DEBUG_BEGIN(x)   { Serial.begin(x); }
#define DEBUG_PRINT(x)   Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)

#else

#define DEBUG_BEGIN(x)   { Serial.begin(x); }
#define DEBUG_PRINT(x)   {}
#define DEBUG_PRINTLN(x) {}

#endif
