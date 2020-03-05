/****************************************************************************************************************************
   SmallProjects/PCF8574/PCF8574.ino
   PCF8574 8-channel I2C Digital Input Output Expander, for ESP32/ESP8266
   For ESP8266 / ESP32 boards
   Written by Khoi Hoang
   Copyright (c) 2019 Khoi Hoang

   This example demontrates the use of a Master Controller (ESP32/ESP8266) to control 8 different Slave Controllers (ESP8266)
   Each Slave Controller is doing diferent tasks inside an imaginary house.
   One of the Slave Controller is ESP8266-based RC-433MHz controller running Blynk/Alexa.
   Another one is SONOFF WiFi Smart SW, running Blynk/Alexa software.

   This example uses 2-dimensional array of struct(ure)s to enable timer to control objects in Slave Controller via Bridge

   Built by Khoi Hoang https://github.com/khoih-prog/SmallProjects/MasterController
   Licensed under MIT license
   Version: 1.0.0

   Version Modified By   Date      Comments
   ------- -----------  ---------- -----------
    1.0.0   K Hoang     12/12/2019 Use Blynk_WM
 *****************************************************************************************************************************/
#if !( defined(ESP8266) || defined(ESP32) )
#error This code is designed to run ESP8266 or ESP32! Please check your Tools->Board setting.
#endif

#define BLYNK_PRINT Serial

#define USE_SPIFFS    true
//#define USE_SPIFFS    false

#define USE_BLYNK_WM    true            // https://github.com/khoih-prog/Blynk_WM
//#define USE_BLYNK_WM    false

#if defined(ESP8266)
#define USE_ESP32         false
#warning Use ESP8266
#else
//Ported to ESP32
#define USE_ESP32         true
#warning Use ESP32
#endif

#if USE_ESP32
#ifdef ESP8266
#undef ESP8266
#define ESP32
#endif
#endif

#if USE_ESP32
#include <esp_wifi.h>
#define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())
#else
#define ESP_getChipId()   (ESP.getChipId())
#endif

#define USE_SSL     false
//#define USE_SSL     true

#if USE_SSL
//#define BLYNK_SSL_USE_LETSENCRYPT       true

#if !USE_BLYNK_WM
// SSL can't use server with local IP 192.168.x.x, but OK with yourname.duckdns.org because Certificate generated only for yourname.duckdns.org
//
char blynk_token[]        = "PFC8574-SSL-Token";
String cloudBlynkServer = "yourname.duckdns.org";
char ssid[] = "****";
char pass[] = "****";
#endif

#define BLYNK_SERVER_HARDWARE_PORT    9443

#else

#if !USE_BLYNK_WM
char blynk_token[]        = "PFC8574-Token";
char cloudBlynkServer[] = "yourname.duckdns.org";
#define BLYNK_SERVER_HARDWARE_PORT    8080
char ssid[] = "****";
char pass[] = "****";
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

#define PIN_D14           5         // Pin D14 mapped to pin GPIO4/SDA of ESP8266
#define PIN_D15           4         // Pin D15 mapped to pin GPIO5/SCL of ESP8266

#define PIN_SCL           4         // Pin SCL mapped to pin GPIO22/SCL of ESP32
#define PIN_SDA           5         // Pin SDA mapped to pin GPIO21/SDA of ESP32    

#endif    //USE_ESP32

#include <Wire.h>

#include <PCF8574.h>

#define PCF8574_I2C_ADDRESS_1     0x20
#define PCF8574_I2C_ADDRESS_2     0x21
#define PCF8574_I2C_ADDRESS_3     0x22
#define PCF8574_I2C_ADDRESS_4     0x23
#define PCF8574_I2C_ADDRESS_5     0x24
#define PCF8574_I2C_ADDRESS_6     0x25
#define PCF8574_I2C_ADDRESS_7     0x26
#define PCF8574_I2C_ADDRESS_8     0x27

#define USE_DYNAMIC_ALLOCATION    true        //false

#if USE_DYNAMIC_ALLOCATION
PCF8574* p_pfc8574[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
#else
PCF8574 pfc8574[8];
#endif

#define RELAY_LOW_ACTIVE     true

#if RELAY_LOW_ACTIVE
#define ON      0
#define OFF     1
#else
#define ON      1
#define OFF     0
#endif

void moduleWrite(byte module, byte chan, boolean value)
{
#if USE_DYNAMIC_ALLOCATION
  if (p_pfc8574[module])
    p_pfc8574[module]->digitalWrite(chan, value);
#else
  pfc8574[module].digitalWrite(chan, value);
#endif

}

BLYNK_WRITE(V1)
{
  moduleWrite(0, 0, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V2)
{
  moduleWrite(0, 1, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V3)
{
  moduleWrite(0, 2, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V4)
{
  moduleWrite(0, 3, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V5)
{
  moduleWrite(0, 4, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V6)
{
  moduleWrite(0, 5, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V7)
{
  moduleWrite(0, 6, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V8)
{
  moduleWrite(0, 7, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V9)
{
  moduleWrite(1, 0, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V10)
{
  moduleWrite(1, 1, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V11)
{
  moduleWrite(1, 2, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V12)
{
  moduleWrite(1, 3, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V13)
{
  moduleWrite(1, 4, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V14)
{
  moduleWrite(1, 5, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V15)
{
  moduleWrite(1, 6, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V16)
{
  moduleWrite(1, 7, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V17)
{
  moduleWrite(2, 0, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V18)
{
  moduleWrite(2, 1, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V19)
{
  moduleWrite(2, 2, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V20)
{
  moduleWrite(2, 3, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V21)
{
  moduleWrite(2, 4, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V22)
{
  moduleWrite(2, 5, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V23)
{
  moduleWrite(2, 6, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V24)
{
  moduleWrite(2, 7, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V25)
{
  moduleWrite(3, 0, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V26)
{
  moduleWrite(3, 1, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V27)
{
  moduleWrite(3, 2, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V28)
{
  moduleWrite(3, 3, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V29)
{
  moduleWrite(3, 4, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V30)
{
  moduleWrite(3, 5, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V31)
{
  moduleWrite(3, 6, param.asInt() ? ON : OFF);
}

BLYNK_WRITE(V32)
{
  moduleWrite(3, 7, param.asInt() ? ON : OFF);
}

BLYNK_CONNECTED()
{
#if (DEBUG_SETUP > 0)
  Serial.println("Blynk Connected. SyncAll");
#endif

  // synchronize relays' (hardware) states with current stored buttons' (widget) states
  // after power-on, reset or blynk reconnected after disconnected.
  Blynk.syncAll();
}

#define DEBUG_SETUP   1

// Debug console
void setup()
{
  Serial.begin(115200);

#if (DEBUG_SETUP > 0)
  Serial.println("Starting");
#endif

  Wire.begin (PIN_SDA, PIN_SCL);

  for (byte i = PCF8574_I2C_ADDRESS_1, module = 0; i < PCF8574_I2C_ADDRESS_7; i++, module++)
  {
    Wire.beginTransmission (i);        // Begin I2C transmission Address (i)

    if (Wire.endTransmission () == 0)  // Receive 0 = success (ACK response)
    {
#if (DEBUG_SETUP > 0)
      Serial.println ("Init PCF8574 @ address 0x" + String(i, HEX));
#endif

#if USE_DYNAMIC_ALLOCATION

      p_pfc8574[module] = new PCF8574();

      if (p_pfc8574[module])
      {
        p_pfc8574[module]->begin(i);

        for (byte j = 0; j < 8; j++)
        {
#if (DEBUG_SETUP > 1)
          Serial.printf("Setup pin %d\n", j + 1);
#endif
          p_pfc8574[module]->pinMode (j, OUTPUT);
          p_pfc8574[module]->digitalWrite(j, OFF);
        }
#if (DEBUG_SETUP > 0)
        Serial.printf("Done for module %d\n", module);
#endif
      }

#else
      pfc8574[module].begin(i);

      for (byte j = 0; j < 8; j++)
      {
#if (DEBUG_SETUP > 1)
        Serial.printf("Setup pin %d\n", j + 1);
#endif
        pfc8574[module].pinMode (j, OUTPUT);
        pfc8574[module].digitalWrite(j, OFF);
      }
#if (DEBUG_SETUP > 0)
      Serial.printf("Done for module %d\n", module);
#endif

#endif

    }
  }

#if USE_BLYNK_WM
  Blynk.begin("PFC-8574");
#else
  Blynk.begin(blynk_token, ssid, pass, cloudBlynkServer.c_str(), BLYNK_SERVER_HARDWARE_PORT);
#endif
}

static ulong curr_time        = 0;

void check_status()
{
  static ulong checkstatus_timeout              = 0;
  static ulong checkstatus_report_timeout       = 0;

#define SEC_TO_MS                 1000

#define STATUS_CHECK_INTERVAL     (10 * SEC_TO_MS)

  curr_time = millis();

  // Send status report every STATUS_REPORT_INTERVAL (10) seconds: we don't need to send updates frequently if there is no status change.
  if ((curr_time > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    // report status to Blynk
    if (Blynk.connected())
    {
#if 1 //(DEBUG_LOOP > 0)
      Serial.println("B");
#endif
    }

    checkstatus_timeout = curr_time + STATUS_CHECK_INTERVAL;
  }
}


void loop()
{
  Blynk.run();
  check_status();
}
