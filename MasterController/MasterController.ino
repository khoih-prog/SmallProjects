/****************************************************************************************************************************
   SmallProjects/MasterController/MasterController.ino
   MasterController for ESP32/ESP8266, using Blynk Bridge to control slave controllers
   For ESP8266 / ESP32 boards
   Written by Khoi Hoang
   Copyright (c) 2019 Khoi Hoang

   This example demontrates the use of a Master Controller (ESP32/ESP8266) to control 8 different Slave Controllers (ESP8266)
   Each Slave Controller is doing diferent tasks inside an imaginary house.
   One of the Slave Controller is RC-433MHz controller.

   This example uses 2-dimensional array of struct(ure)s to enable timer to control objects in Slave Controller via Bridge

   Built by Khoi Hoang https://github.com/khoih-prog/SmallProjects/MasterController
   Licensed under MIT license
   Version: 1.0.0

   Version Modified By   Date      Comments
   ------- -----------  ---------- -----------
    1.0.0   K Hoang     24/05/2018 Initial coding for ESP32/ESP8266
    1.0.1   K Hoang     12/12/2019 Use Blynk_WM
 *****************************************************************************************************************************/
/****************************************************************************************************************************
   Currently default settings to
   1) Use BLYNK_WM (with SPIFFS)
   2) Use ESP32
   3) No SSL
   4) Use OTA
   You can change these default settings by modifying corresponding #define from true to false
 *****************************************************************************************************************************/

#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG

// Not use #define USE_SPIFFS  => using EEPROM for configuration data in WiFiManager
// #define USE_SPIFFS    false => using EEPROM for configuration data in WiFiManager
// #define USE_SPIFFS    true  => using SPIFFS for configuration data in WiFiManager
// Be sure to define USE_SPIFFS before #include <BlynkSimpleEsp8266_WM.h>

#define USE_SPIFFS    true
//#define USE_SPIFFS    false

#define USE_BLYNK_WM    true            // https://github.com/khoih-prog/Blynk_WM
//#define USE_BLYNK_WM    false

//Ported to ESP32
#define USE_ESP32         true
//#define USE_ESP32         false

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

#define USE_SSL     false
//#define USE_SSL     true

#if USE_SSL
//#define BLYNK_SSL_USE_LETSENCRYPT       true

#if USE_BLYNK_WM

#else
// SSL can't use server with local IP 192.168.x.x, but OK with yourname.duckdns.org because Certificate generated only for yourname.duckdns.org
//
String cloudBlynkServer = "yourname.duckdns.org";
#define BLYNK_SERVER_HARDWARE_PORT    9443
#endif
#else
#if USE_BLYNK_WM

#else
char blynk_token[]        = "Master-Token";
char localBlynkServerIP[] = "yourname.duckdns.org";
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

#define DEBUG_SETUP         0
#define DEBUG_LOOP          1
#define DEBUG_BLYNK_RTC     1

#define USE_ARDUINO_OTA     true

#if USE_ARDUINO_OTA
#include <ArduinoOTA.h>
#endif

BlynkTimer timer;

#include <TimeLib.h>
#include <WidgetRTC.h>
WidgetRTC rtc;

#define CURRENT_TIME_LENGTH     12
#define CURRENT_DAY_LENGTH      15

char currentTime[CURRENT_TIME_LENGTH];   //prepared for AM/PM time
char currentDay [CURRENT_DAY_LENGTH];

char dayOfWeek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
char activatedTime[15];

bool clockSync = false;

#define BLYNK_SCHEDULE_ACCURACY       30            // in seconds

WidgetLCD lcd(V0);

WidgetBridge bridge1(V1); //Initiating Bridge Widget on V1
WidgetBridge bridge2(V2); //Initiating Bridge Widget on V2
WidgetBridge bridge3(V3); //Initiating Bridge Widget on V3
WidgetBridge bridge4(V4); //Initiating Bridge Widget on V4
WidgetBridge bridge5(V5); //Initiating Bridge Widget on V5
WidgetBridge bridge6(V6); //Initiating Bridge Widget on V6
WidgetBridge bridge7(V7); //Initiating Bridge Widget on V7
WidgetBridge bridge8(V8); //Initiating Bridge Widget on V8

#define RC_SWITCH_ENABLE    true

#if RC_SWITCH_ENABLE

#define ZAP_PWR_SOCKET_PULSE_LEN      182       //microsecs
#define ZAP_PWR_SOCKET_PROTOCOL       1

// Binary code for buttons OF zap rc, CODE 0326
#define ZAP_PWR_SOCKET_BUTTON_1_ON      "010001000001010100110011"  //0x441533
#define ZAP_PWR_SOCKET_BUTTON_1_OFF     "010001000001010100111100"  //0x44153C
#define ZAP_PWR_SOCKET_BUTTON_2_ON      "010001000001010111000011"  //0x4415C3
#define ZAP_PWR_SOCKET_BUTTON_2_OFF     "010001000001010111001100"  //0X4415CC
#define ZAP_PWR_SOCKET_BUTTON_3_ON      "010001000001011100000011"  //0X441703
#define ZAP_PWR_SOCKET_BUTTON_3_OFF     "010001000001011100001100"  //0X44170C
#define ZAP_PWR_SOCKET_BUTTON_4_ON      "010001000001110100000011"  //0X441D03
#define ZAP_PWR_SOCKET_BUTTON_4_OFF     "010001000001110100001100"  //0X441D0C
#define ZAP_PWR_SOCKET_BUTTON_5_ON      "010001000011010100000011"  //0X443503
#define ZAP_PWR_SOCKET_BUTTON_5_OFF     "010001000011010100001100"  //0X44350C

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

#define DEBUG_RC_SWITCH     1

#endif

BLYNK_CONNECTED()
{
  rtc.begin();
  bridge1.setAuthToken("Slave-Token-1");        // Token of the Stairway Light of New WiFi Relay
  bridge2.setAuthToken("Slave-Token-2");        // Token of the Buffet Light of Sonoff WiFi Relay 1
  bridge3.setAuthToken("Slave-Token-3");        // Token of the Desk Lamp of Sonoff WiFi Relay 2
  bridge4.setAuthToken("Slave-Token-4");        // Token of the TestNodeMCU  (Master Bed, etc.)
  bridge5.setAuthToken("Slave-Token-5");        // Token of the TestNodeMCU2 (Dining Room, etc.)
  bridge6.setAuthToken("Slave-Token-6");        // Token of the TestNodeMCU3 (Kitchen, etc.)
  bridge7.setAuthToken("Slave-Token-7");        // Token of the RX/TX Relay1 (Garage Door)
  bridge8.setAuthToken("Slave-Token-8");        // Token of the RX/TX Relay2 (Basement Light)
  //bridge9.setAuthToken("Slave-Token-9");      // Token of the 1st Garage
  //bridge10.setAuthToken("Slave-Token-10");    // Token of the 2nd Garage

  //synchronize the state of widgets with hardware states
  //Blynk.syncAll();
}

void activetoday()
{
  // check if schedule #1 or #2 should run today
  if (year() != 1970)
  {
    //Device 0, Stairway Light
    Blynk.syncVirtual(V71);  // sync scheduler #1
    Blynk.syncVirtual(V72);  // sync scheduler #2
    Blynk.syncVirtual(V73);  // sync scheduler #3
    Blynk.syncVirtual(V74);  // sync scheduler #4

    //Device 1, Garage Door
    Blynk.syncVirtual(V80);  // sync scheduler #1
    Blynk.syncVirtual(V81);  // sync scheduler #2
    Blynk.syncVirtual(V82);  // sync scheduler #3

    //Device 2, Basement Light
    Blynk.syncVirtual(V84);  // sync scheduler #1
    Blynk.syncVirtual(V85);  // sync scheduler #2
    Blynk.syncVirtual(V86);  // sync scheduler #3
    Blynk.syncVirtual(V87);  // sync scheduler #4
  }
}

char* currentTimeAM_PM(char *inputTime)
{
  int hourAM_PM = hour();

  if (hourAM_PM >= 12)
  {
    if (hourAM_PM > 12)
      hourAM_PM -= 12;
    sprintf(inputTime, "%02d:%02d:%02d PM", hourAM_PM, minute(), second());
  }
  else
  {
    if (hourAM_PM == 0)
      hourAM_PM = 12;
    sprintf(inputTime, "%02d:%02d:%02d AM", hourAM_PM, minute(), second());
  }
  return inputTime;
}

void clockDisplay()
{
  if (year() != 1970)
  {
    sprintf(currentDay,  "%s %02d/%02d/%04d", dayOfWeek[weekday() - 1], day(), month(), year());
    lcd.print(0, 0, currentTimeAM_PM(currentTime));
    lcd.print(0, 1, currentDay);
    if (clockSync == false)
    {
#if (DEBUG_BLYNK_RTC > 0)
      //Only print the first time after sync
      sprintf(activatedTime, "%s %02d/%02d/%04d", dayOfWeek[weekday() - 1], day(), month(), year());
      Serial.printf("First sync at %s on %s\n", currentTimeAM_PM(currentTime), activatedTime);
#endif
      //Null terminated string
      currentTime[CURRENT_TIME_LENGTH - 1]  = 0;
      currentDay[CURRENT_DAY_LENGTH - 1]    = 0;

      clockSync = true;
    }
  }
}

enum action_t
{
  TURN_OFF    = 0,
  TURN_ON     = 1,
  DO_NOTHING  = 2,
  MAX_ACTION
};

#if (DEBUG_BLYNK_RTC > 1)
char* lastActionStr[MAX_ACTION] = { "TURN_OFF", "TURN_ON", "DO_NOTHING" };
#endif

int isTimeInputActivated(const unsigned int scheduleNo, int &lastAction, const TimeInputParam t)
{
  int startseconds = (t.getStartHour() * 3600) + (t.getStartMinute() * 60) + t.getStartSecond();
  int stopseconds  = (t.getStopHour() * 3600) + (t.getStopMinute() * 60) + t.getStopSecond();

  //In case of inactive timer
  if (startseconds == stopseconds)
    return DO_NOTHING;

  int retVal = DO_NOTHING;
  int nowseconds   = ((hour() * 3600) + (minute() * 60) + second());

  int blynkWeekDay = weekday();

  if (blynkWeekDay == 1)
    blynkWeekDay = 7; // needed for Sunday Time library is day 1 and Blynk is day 7
  else
    blynkWeekDay--;

  if (t.isWeekdaySelected(blynkWeekDay))
  {
    //Time library starts week on Sunday, Blynk on Monday
    //Schedule is ACTIVE today
    if ((lastAction != TURN_ON) && (nowseconds >= (startseconds - (BLYNK_SCHEDULE_ACCURACY + 1)) && nowseconds <= (startseconds + BLYNK_SCHEDULE_ACCURACY)))
    {
#if (DEBUG_BLYNK_RTC > 0)
      sprintf(activatedTime, "%s %02d/%02d/%04d", dayOfWeek[weekday() - 1], day(), month(), year());
      Serial.printf("Schedule %d started at %s on %s\n", scheduleNo, currentTimeAM_PM(currentTime), activatedTime);
#endif
      retVal = TURN_ON;

#if (DEBUG_BLYNK_RTC > 1)
      Serial.printf("Last Action = %s, Action = %s, Now = %d, startsec = %d, stopsec = %d\n",
                    lastActionStr[lastAction], lastActionStr[retVal], nowseconds, startseconds, stopseconds);
#endif
    }

    if ((lastAction != TURN_OFF) && (nowseconds >= (stopseconds - (BLYNK_SCHEDULE_ACCURACY + 1)) && nowseconds <= (stopseconds + BLYNK_SCHEDULE_ACCURACY)))
    {
#if (DEBUG_BLYNK_RTC > 0)
      sprintf(activatedTime, "%s %02d/%02d/%04d", dayOfWeek[weekday() - 1], day(), month(), year());
      Serial.printf("Schedule %d stopped at %s on %s\n", scheduleNo, currentTimeAM_PM(currentTime), activatedTime);
#endif
      //If both startSeconds and stopSeconds falls in the same 2 * (BLYNK_SCHEDULE_ACCURACY) range, DO_NOTHING
      if (retVal == TURN_ON)
        retVal = DO_NOTHING;
      else
        retVal = TURN_OFF;
#if (DEBUG_BLYNK_RTC > 1)
      Serial.printf("Last Action = %s, Action = %s, Now = %d, startsec = %d, stopsec = %d\n",
                    lastActionStr[lastAction], lastActionStr[retVal], nowseconds, startseconds, stopseconds);
#endif
    }

    if (retVal != DO_NOTHING)
      lastAction = retVal;
  }

  return retVal;
}

void processBLYNK_WRITE(WidgetBridge* bridgeP, int pinData, int LEDVPin, int controlVPin, bool isInverted)
{
  if (pinData == TURN_ON)
  {
    Blynk.virtualWrite(LEDVPin, 255);  // turn ON virtual LED
    if (isInverted)
      bridgeP->digitalWrite(controlVPin, LOW);
    else
      bridgeP->digitalWrite(controlVPin, HIGH);
  }
  else if (pinData == TURN_OFF)
  {
    Blynk.virtualWrite(LEDVPin, 0);   // turn OFF virtual LED
    if (isInverted)
      bridgeP->digitalWrite(controlVPin, HIGH);
    else
      bridgeP->digitalWrite(controlVPin, LOW);
  }
}

#define NUM_OF_TIMERS_PER_DEVICE      4
#define MAX_DEVICE_ID                 3

//The new relay uses opto-coupler and GPIO0 to drive the relay (GPIO0 = LOW => relay activates)
#define PIN_GPIO0     0     // Header Pin GPIO0, mapped to pin 15 ESP8266EX
#define PIN_GPIO2     2     // Header Pin GPIO2, mapped to pin 14 ESP8266EX

typedef struct
{
  const int vPin;
  WidgetBridge* bridgeP;
  const int LEDVPin;
  const int controlVPin;
  int lastAction;
  int currentAction;
} Timer_Property_t;

Timer_Property_t myTimer[MAX_DEVICE_ID][NUM_OF_TIMERS_PER_DEVICE] =
{
  {
    { V71, &bridge1, V50, PIN_GPIO0, DO_NOTHING, DO_NOTHING },
    { V72, &bridge1, V50, PIN_GPIO0, DO_NOTHING, DO_NOTHING },
    { V73, &bridge1, V50, PIN_GPIO0, DO_NOTHING, DO_NOTHING },
    { V74, &bridge1, V50, PIN_GPIO0, DO_NOTHING, DO_NOTHING }
  },
  {
    { V80, &bridge7, V53, PIN_GPIO2, DO_NOTHING, DO_NOTHING },
    { V81, &bridge7, V53, PIN_GPIO2, DO_NOTHING, DO_NOTHING },
    { V82, &bridge7, V53, PIN_GPIO2, DO_NOTHING, DO_NOTHING },
    { 0,   NULL,     V53, PIN_GPIO2, DO_NOTHING, DO_NOTHING }
  },
  {
    { V84, &bridge8, V54, PIN_GPIO2, DO_NOTHING, DO_NOTHING },
    { V85, &bridge8, V54, PIN_GPIO2, DO_NOTHING, DO_NOTHING },
    { V86, &bridge8, V54, PIN_GPIO2, DO_NOTHING, DO_NOTHING },
    { V87, &bridge8, V54, PIN_GPIO2, DO_NOTHING, DO_NOTHING }
  }
};

int finalAction[MAX_DEVICE_ID];
int lastFinalAction[MAX_DEVICE_ID] = { DO_NOTHING };

const int RELAY_PIN_[MAX_DEVICE_ID] = { PIN_GPIO0 };      // Header Pin GPIO0, mapped to pin 15 ESP8266EX

void makeDecision(int deviceNo, int timerNo, int returnAction, bool isInverted)
{
  //Complicated to take care of overlapping timers and minimize traffic when DO_NOTHING
  if (returnAction == DO_NOTHING)
    return;
  else if (returnAction == TURN_ON)
    myTimer[deviceNo][timerNo].currentAction = finalAction[deviceNo] = TURN_ON;
  else if (returnAction == TURN_OFF)
  {
    myTimer[deviceNo][timerNo].currentAction = finalAction[deviceNo] = TURN_OFF;
    //If only another timer turns ON relay, still keep relay ON even this timer wants to turn OFF (OR action for ON)
    for (int i = 0; i < NUM_OF_TIMERS_PER_DEVICE; i++)
    {
#if (DEBUG_BLYNK_RTC > 1)
      Serial.printf("Device %d, Timer %d, current Action = %s\n",
                    deviceNo, i + 1, lastActionStr[myTimer[deviceNo][i].currentAction]);
#endif
      if (myTimer[deviceNo][i].currentAction == TURN_ON)
      {
#if (DEBUG_BLYNK_RTC > 2)
        Serial.printf("Device %d, Timer %d, current Action = %s, previous Final Action = %s, Final Action = TURN_ON\n",
                      deviceNo, i + 1, lastActionStr[myTimer[deviceNo][i].currentAction], lastActionStr[finalAction[deviceNo]]);
#endif
        finalAction[deviceNo] = TURN_ON;
        break;
      }
    }
  }

  if (lastFinalAction[deviceNo] != finalAction[deviceNo])
  {
#if (DEBUG_BLYNK_RTC > 1)
    Serial.printf("Device %d, Timer %d, Last Final Action = %s, ****> Final Action = %s\n",
                  deviceNo, timerNo + 1, lastActionStr[lastFinalAction[deviceNo]], lastActionStr[finalAction[deviceNo]]);
#endif
    lastFinalAction[deviceNo] = finalAction[deviceNo];
    processBLYNK_WRITE(myTimer[deviceNo][timerNo].bridgeP, finalAction[deviceNo], myTimer[deviceNo][timerNo].LEDVPin,
                       myTimer[deviceNo][timerNo].controlVPin, isInverted);
  }
}

//Stairway Light, Device 0
BLYNK_WRITE(V71)
{
  //Device 0, Scheduler #1 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(1, myTimer[0][0].lastAction, t);

  //Device 0, Timer 0, action, isInverted
  makeDecision(0, 0, returnAction, true);
}

BLYNK_WRITE(V72)
{
  //Device 0, Scheduler #2 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(2, myTimer[0][1].lastAction, t);

  //Device 0, Timer 1, action, isInverted
  makeDecision(0, 1, returnAction, true);
}

BLYNK_WRITE(V73)
{
  //Device 0, Scheduler #3 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(3, myTimer[0][2].lastAction, t);

  //Device 0, Timer 2, action, isInverted
  makeDecision(0, 2, returnAction, true);
}

BLYNK_WRITE(V74)
{
  //Device 0, Scheduler #4 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(4, myTimer[0][3].lastAction, t);

  //Device 0, Timer 3, action, isInverted
  makeDecision(0, 3, returnAction, true);
}

//Garage Door, Device 1
BLYNK_WRITE(V80)
{
  //Device 1, Scheduler #1 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(11, myTimer[1][0].lastAction, t);

  //Device 1, Timer 0, action, isInverted
  makeDecision(1, 0, returnAction, false);
}

BLYNK_WRITE(V81)
{
  //Device 1, Scheduler #2 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(12, myTimer[1][1].lastAction, t);

  //Device 1, Timer 1, action, isInverted
  makeDecision(1, 1, returnAction, false);
}

BLYNK_WRITE(V82)
{
  //Device 1, Scheduler #3 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(13, myTimer[1][2].lastAction, t);

  //Device 1, Timer 2, action, isInverted
  makeDecision(1, 2, returnAction, false);
}

//Basement Light, Device 2
BLYNK_WRITE(V84)
{
  //Device 2, Scheduler #1 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(21, myTimer[2][0].lastAction, t);

  //Device 2, Timer 0, action, isInverted
  makeDecision(2, 0, returnAction, false);
}

BLYNK_WRITE(V85)
{
  //Device 2, Scheduler #2 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(22, myTimer[2][1].lastAction, t);

  //Device 2, Timer 1, action, isInverted
  makeDecision(2, 1, returnAction, false);
}

BLYNK_WRITE(V86)
{
  //Device 2, Scheduler #3 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(23, myTimer[2][2].lastAction, t);

  //Device 2, Timer 2, action, isInverted
  makeDecision(2, 2, returnAction, false);
}

BLYNK_WRITE(V87)
{
  //Device 2, Scheduler #4 Time Input widget
  TimeInputParam t(param);

  int returnAction = isTimeInputActivated(24, myTimer[2][3].lastAction, t);

  //Device 2, Timer 3, action, isInverted
  makeDecision(2, 3, returnAction, false);
}

//Button simulated at V60 to control LED and Stairway Light / New Relay via Bridge1, GPIO0 LOW active (inverted)
BLYNK_WRITE(V60)
{
  // Button Input widget
  int pinData = param.asInt();
  //GPIO0 LOW active (inverted)
  processBLYNK_WRITE(&bridge1, pinData, V50, PIN_GPIO0, true);
}

//Button simulated at V63 to control LED and Garage Door / RX/TX Relay via Bridge7
BLYNK_WRITE(V63)
{
  // Button Input widget
  int pinData = param.asInt();
  //GPIO2 HIGH active (not inverted)
  processBLYNK_WRITE(&bridge7, pinData, V53, PIN_GPIO2, false);
}
//Button simulated at V64 to control LED and Basement Light / RX/TX Relay via Bridge8
BLYNK_WRITE(V64)
{
  // Button Input widget
  int pinData = param.asInt();
  //GPIO2 HIGH active (not inverted)
  processBLYNK_WRITE(&bridge8, pinData, V54, PIN_GPIO2, false);
}

//Control 2 outputs
void processBLYNK_WRITE2(WidgetBridge* bridgeP, int pinData, int LEDVPin, int controlVPin1, bool isInverted1, int controlVPin2, bool isInverted2)
{
  if (pinData == TURN_ON)
  {
    Blynk.virtualWrite(LEDVPin, 255);  // turn ON virtual LED
    if (isInverted1)
      bridgeP->digitalWrite(controlVPin1, LOW);
    else
      bridgeP->digitalWrite(controlVPin1, HIGH);
    if (isInverted2)
      bridgeP->digitalWrite(controlVPin2, LOW);
    else
      bridgeP->digitalWrite(controlVPin2, HIGH);
  }
  else if (pinData == TURN_OFF)
  {
    Blynk.virtualWrite(LEDVPin, 0);   // turn OFF virtual LED
    if (isInverted1)
      bridgeP->digitalWrite(controlVPin1, HIGH);
    else
      bridgeP->digitalWrite(controlVPin1, LOW);
    if (isInverted2)
      bridgeP->digitalWrite(controlVPin2, HIGH);
    else
      bridgeP->digitalWrite(controlVPin2, LOW);
  }
}

#define SONOFF_LED_PIN       13    // Pin mapped to pin GPIO13 of ESP8266.
#define SONOFF_RELAY_PIN     12    // Pin mapped to pin GPIO12 of ESP8266.

//Button simulated at V61 to control LED and Buffet Light via Bridge2
BLYNK_WRITE(V61)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE2(&bridge2, pinData, V51, SONOFF_LED_PIN, true, SONOFF_RELAY_PIN, false);
}

//Button simulated at V62 to control LED and Desk Lamp via Bridge3
BLYNK_WRITE(V62)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE2(&bridge3, pinData, V52, SONOFF_LED_PIN, true, SONOFF_RELAY_PIN, false);
}

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

//NodeMCU, bridge4, V10-V15 and V30-V35
//Button simulated at V10 to control LED and Master Bed Light (D1/GPIO5) of TestNodeMCU via Bridge4
BLYNK_WRITE(V10)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge4, pinData, V30, PIN_D1, false);
}

//Button simulated at V11 to control LED and Master Bath Light (D2/GPIO4) of TestNodeMCU via Bridge4
BLYNK_WRITE(V11)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge4, pinData, V31, PIN_D2, false);
}

//Button simulated at V12 to control LED and Main Bath Light (D3/GPIO0) of TestNodeMCU via Bridge4
BLYNK_WRITE(V12)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge4, pinData, V32, PIN_D3, false);
}

//Button simulated at V13 to control LED and Family Room Light (D5/GPIO14) of TestNodeMCU via Bridge4
BLYNK_WRITE(V13)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge4, pinData, V33, PIN_D5, false);
}

//Button simulated at V14 to control LED and Guest Room Light (D6/GPIO12) of TestNodeMCU via Bridge4
BLYNK_WRITE(V14)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge4, pinData, V34, PIN_D6, false);
}

//Button simulated at V15 to control LED and Office Light (D7/GPIO13) of TestNodeMCU via Bridge4
BLYNK_WRITE(V15)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge4, pinData, V35, PIN_D7, false);
}

//NodeMCU2, bridge5, V16-V21 and V36-V41
//Button simulated at V16 to control LED and Dining Room Light (D1/GPIO5) of TestNodeMCU2 via Bridge5
BLYNK_WRITE(V16)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge5, pinData, V36, PIN_D1, false);
}

//Button simulated at V17 to control LED and Chandelier Light (D2/GPIO4) of TestNodeMCU2 via Bridge5
BLYNK_WRITE(V17)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge5, pinData, V37, PIN_D2, false);
}

//Button simulated at V18 to control LED and Living Room (D3/GPIO0) of TestNodeMCU2 via Bridge5
BLYNK_WRITE(V18)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge5, pinData, V38, PIN_D3, false);
}

//Button simulated at V19 to control LED and Powder Room Light (D5/GPIO14) of TestNodeMCU2 via Bridge5
BLYNK_WRITE(V19)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge5, pinData, V39, PIN_D5, false);
}

//Button simulated at V20 to control LED and Office SW (D6/GPIO12) of TestNodeMCU2 via Bridge5
BLYNK_WRITE(V20)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge5, pinData, V40, PIN_D6, false);
}

//Button simulated at V21 to control LED and Not-assigned (D7/GPIO13) of TestNodeMCU2 via Bridge5
BLYNK_WRITE(V21)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge5, pinData, V41, PIN_D7, false);
}

//NodeMCU3, bridge6, V22-V27 and V42-V47
//Button simulated at V22 to control LED and Kitchen Light (D1/GPIO5) of TestNodeMCU3 via Bridge6
BLYNK_WRITE(V22)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge6, pinData, V42, PIN_D1, false);
}

//Button simulated at V23 to control LED and Range Hood (D2/GPIO4) of TestNodeMCU3 via Bridge6
BLYNK_WRITE(V23)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge6, pinData, V43, PIN_D2, false);
}

//Button simulated at V24 to control LED and Fan Low (D3/GPIO0) of TestNodeMCU3 via Bridge6
BLYNK_WRITE(V24)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge6, pinData, V44, PIN_D3, false);
}

//Button simulated at V25 to control LED and Fan Medium (D5/GPIO14) of TestNodeMCU3 via Bridge6
BLYNK_WRITE(V25)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge6, pinData, V45, PIN_D5, false);
}

//Button simulated at V26 to control LED and Fan High (D6/GPIO12) of TestNodeMCU3 via Bridge6
BLYNK_WRITE(V26)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge6, pinData, V46, PIN_D6, false);
}

//Button simulated at V27 to control LED and Not-assigned (D7/GPIO13) of TestNodeMCU3 via Bridge6
BLYNK_WRITE(V27)
{
  // Button Input widget
  int pinData = param.asInt();
  processBLYNK_WRITE(&bridge6, pinData, V47, PIN_D7, false);
}

#if RC_SWITCH_ENABLE

//Button simulated at V65 to control RC Transmitter #1
BLYNK_WRITE(V65)
{
  // Button Input widget
  int pinData = param.asInt();

  if (pinData == TURN_ON)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 1 ON");
#endif

    Blynk.virtualWrite(V75, 255);  // turn ON virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_1_ON);
  }
  else if (pinData == TURN_OFF)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 1 OFF");
#endif

    Blynk.virtualWrite(V75, 0);  // turn OFF virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_1_OFF);
  }
}

//Button simulated at V66 to control RC Transmitter #2
BLYNK_WRITE(V66)
{
  // Button Input widget
  int pinData = param.asInt();

  if (pinData == TURN_ON)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 2 ON");
#endif

    Blynk.virtualWrite(V76, 255);  // turn ON virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_2_ON);
  }
  else if (pinData == TURN_OFF)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 2 OFF");
#endif

    Blynk.virtualWrite(V76, 0);  // turn OFF virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_2_OFF);
  }
}

//Button simulated at V67 to control RC Transmitter #3
BLYNK_WRITE(V67)
{
  // Button Input widget
  int pinData = param.asInt();

  if (pinData == TURN_ON)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 3 ON");
#endif

    Blynk.virtualWrite(V77, 255);  // turn ON virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_3_ON);
  }
  else if (pinData == TURN_OFF)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 3 OFF");
#endif

    Blynk.virtualWrite(V77, 0);  // turn OFF virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_3_OFF);
  }
}

//Button simulated at V68 to control RC Transmitter #4
BLYNK_WRITE(V68)
{
  // Button Input widget
  int pinData = param.asInt();

  if (pinData == TURN_ON)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 4 ON");
#endif

    Blynk.virtualWrite(V78, 255);  // turn ON virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_4_ON);
  }
  else if (pinData == TURN_OFF)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 4 OFF");
#endif

    Blynk.virtualWrite(V78, 0);  // turn OFF virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_4_OFF);
  }
}

//Button simulated at V69 to control RC Transmitter #5
BLYNK_WRITE(V69)
{
  // Button Input widget
  int pinData = param.asInt();

  if (pinData == TURN_ON)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 5 ON");
#endif

    Blynk.virtualWrite(V79, 255);  // turn ON virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_5_ON);
  }
  else if (pinData == TURN_OFF)
  {
#if (DEBUG_RC_SWITCH > 0)
    Serial.println("Send 5 OFF");
#endif

    Blynk.virtualWrite(V79, 0);  // turn OFF virtual LED
    mySwitch.send(ZAP_PWR_SOCKET_BUTTON_5_OFF);
  }
}

#endif

void setup()
{
  Serial.begin(115200);

#if RC_SWITCH_ENABLE

  // Transmitter is connected to Arduino Pin PIN_D1, GPIO5 of ESP8266
  mySwitch.enableTransmit(PIN_D1);

#if 0
  // Optional set protocol (default is 1, will work for most outlets)
  mySwitch.setProtocol(ZAP_PWR_SOCKET_PROTOCOL);
  // Optional set pulse length.
  mySwitch.setPulseLength(ZAP_PWR_SOCKET_PULSE_LEN);
#endif

  mySwitch.setProtocol(ZAP_PWR_SOCKET_PROTOCOL, ZAP_PWR_SOCKET_PULSE_LEN);

  // Optional set number of transmission repetitions.
  // mySwitch.setRepeatTransmit(15);

#endif

#if USE_BLYNK_WM
  Blynk.begin();
#else
  Blynk.begin(blynk_token, ssid, pass, localBlynkServerIP, BLYNK_SERVER_HARDWARE_PORT);
#endif

#if USE_ARDUINO_OTA
  ArduinoOTA.setHostname("MasterController");
  ArduinoOTA.begin();
#endif

  timer.setInterval(BLYNK_SCHEDULE_ACCURACY * 2 * 1000, activetoday);  // check every 2 * BLYNK_SCHEDULE_ACCURACY secs if ON / OFF trigger time has been reached
  timer.setInterval(5000L, clockDisplay);  // check every 5 seconds if time has been obtained from the server
}

void loop()
{
  Blynk.run();
  timer.run();

#if USE_ARDUINO_OTA
  ArduinoOTA.handle();
#endif
}
