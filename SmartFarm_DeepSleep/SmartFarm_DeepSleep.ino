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

/****************************************************************************************************************************
   To use ESP32 Dev Module, CPU 240MHz (WiFi/BT), QIO, Flash 4MB/80MHz, Upload 921600
   KH Mods in .../hardware/espressif/esp32/libraries/WiFi/src/WiFiScan.cpp
   KH Mods in ~/Arduino/libraries/WiFiManager/WiFiManager.cpp
   Later check why using latest esp32 (arduino-esp32-master) -> crash. Already fixed in WiFiManager.h/cpp

   Issues:
   1) SPIFFS OK now. Follow intruction at  https://github.com/me-no-dev/arduino-esp32fs-plugin
      To upload firmware, press BOOT button just after uploading started.
      Press EN button if booting can't connect to WiFi
*****************************************************************************************************************************/

#include "SmartFarm_DeepSleep.h"

#define USE_DEEPSLEEP      true
//#define USE_DEEPSLEEP      false

#define USE_BLYNK_RTC       true

#if USE_BLYNK_WM

String ssid = "";

#else
#define SMART_FARM_BOARD_NO      6

#if (SMART_FARM_BOARD_NO == 1)
String blynk_token      = "board1-token";
#elif (SMART_FARM_BOARD_NO == 2)
String blynk_token      = "board2-token";
#elif (SMART_FARM_BOARD_NO == 3)
String blynk_token      = "board3-token";
#elif (SMART_FARM_BOARD_NO == 4)
String blynk_token      = "board4-token";
#elif (SMART_FARM_BOARD_NO == 5)
String blynk_token      = "board5-token";
#elif (SMART_FARM_BOARD_NO == 6)
String blynk_token      = "board6-token";
#endif

String ssid = "****";
String pass = "****";
#endif

int16_t curr_RSSI;
int16_t percent_RSSI;

float   humDHT            = 0;
float   tempDHT           = 0;
float   tempF_DHT         = 32;
float   HeatIndexDHT      = 0;
float   HeatIndexF_DHT    = 32;
float   soilMoist         = 0;

boolean pumpStatus        = STOP;
boolean pumpModeAuto      = true;
boolean pumpModeNotice    = true;

boolean tempNormal        = true;
boolean humidNormal       = true;
boolean moistNormal       = true;
boolean RSSINormal        = true;

uint    moist_alarm_level = 50;
boolean moist_alarm       = false;

BlynkTimer    timer;
Ticker        relay_ticker;
Ticker        aux_ticker;

WidgetLED blynk_led_pump_on(BLYNK_PIN_PUMP_ON);
WidgetLED blynk_led_pump_off(BLYNK_PIN_PUMP_OFF);
WidgetRTC rtc;

WidgetLCD blynk_lcd(BLYNK_PIN_LCD);

DHTesp* dht;

String firmware_version = String(FWV / 100 % 10) +  String(FWV / 10 % 10) + String(FWV % 10) + FWSV;

String get_last_ip()
{
  String ip = "x.";
  IPAddress _ip = WiFi.localIP();
  ip += _ip[3];
  return ip;
}

#define DEBUG_LOOP          1
#define DHT_DEBUG           0

#define DEBUG_BLYNK_RTC     1

#define CURRENT_TIME_LENGTH     12
#define CURRENT_DAY_LENGTH      15

char currentTime[CURRENT_TIME_LENGTH];   //prepared for AM/PM time
char currentDay [CURRENT_DAY_LENGTH];

char dayOfWeek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

#if (DEBUG_BLYNK_RTC > 0)
char activatedTime[15];
#endif

bool clockSync = false;

static void restart(void)
{
  ESP.restart();
  //digitalWrite(PIN_RESET, LOW);
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
    blynk_lcd.print(0, 0, currentTimeAM_PM(currentTime));
    blynk_lcd.print(0, 1, currentDay);
    if (clockSync == false)
    {
#if (DEBUG_BLYNK_RTC > 0)
      //Only print the first time after sync
      sprintf(activatedTime, "%s %02d/%02d/%04d", dayOfWeek[weekday() - 1], day(), month(), year());
      Serial.printf("First sync at %s on %s\n", currentTimeAM_PM(currentTime), activatedTime);
#endif
      //blynk_lcd.clear();
      //Null terminated string
      currentTime[CURRENT_TIME_LENGTH - 1]  = 0;
      currentDay[CURRENT_DAY_LENGTH - 1]    = 0;
      clockSync = true;
    }
  }
}

#if (USE_DEEPSLEEP)
// 2 days
#define DAY_IN_MINS        1440
uint64_t deepSleepMax_mins =  (2 * DAY_IN_MINS);

#define USE_BITFIELD    true
#if USE_BITFIELD
/* define a structure with bit fields */
struct RTC_BoolData
{
  uint32_t reserved         : 29;
  uint32_t MW33             : 1;
  uint32_t sensorCapacitive : 1;
  uint32_t USE_CELCIUS      : 1;
};
#endif

#if (USE_ESP32)
RTC_DATA_ATTR uint32_t              bootCount = 0;
RTC_DATA_ATTR uint32_t              RTC_DEEPSLEEP_INTERVAL_FACTOR;
RTC_DATA_ATTR uint32_t              RTC_TIME_TO_DEEPSLEEP;
RTC_DATA_ATTR uint32_t              RTC_DRY_SOIL;
RTC_DATA_ATTR DHTesp::DHT_MODEL_t   RTC_DHTTYPE;
#if USE_BITFIELD
RTC_DATA_ATTR RTC_BoolData          RTC_Data;
#else
// Better to use bit field for boolean data
RTC_DATA_ATTR boolean               RTC_MW33;
RTC_DATA_ATTR boolean               RTC_sensorCapacitive;
RTC_DATA_ATTR boolean               RTC_USE_CELCIUS;
#endif

#else //(USE_ESP32)
/*
  ESP.rtcUserMemoryWrite(offset, &data, sizeof(data)) and ESP.rtcUserMemoryRead(offset, &data, sizeof(data)) allow data to be stored in and
  retrieved from the RTC user memory of the chip respectively. offset is measured in blocks of 4 bytes and can range from 0 to 127 blocks
  (total size of RTC memory is 512 bytes). data should be 4-byte aligned. The stored data can be retained between deep sleep cycles, but might
  be lost after power cycling the chip. Data stored in the first 32 blocks will be lost after performing an OTA update, because they are used by
  the Core internals.
*/
#define bootCountOffset     32
uint32_t bootCount = 0;

uint32_t  RTC_DEEPSLEEP_INTERVAL_FACTOR;
uint32_t  RTC_TIME_TO_DEEPSLEEP;
uint32_t  RTC_DRY_SOIL;
uint32_t  RTC_DHTTYPE;

#if USE_BITFIELD
/* define a structure with bit fields */
RTC_BoolData RTC_Data;
#else
// Better to use bit field for boolean data
uint32_t  RTC_MW33;
uint32_t  RTC_sensorCapacitive;
uint32_t  RTC_USE_CELCIUS;
#endif

#endif  //(USE_ESP32)

#endif  //(USE_DEEPSLEEP)


BLYNK_CONNECTED()
{
  rtc.begin();
  //synchronize the state of widgets with hardware states
  blynk_lcd.clear();
  Blynk.virtualWrite(BLYNK_PIN_FWV, firmware_version);
  ssid = WiFi.SSID();
  Blynk.virtualWrite(BLYNK_PIN_SSID, ssid);
  Blynk.virtualWrite(BLYNK_PIN_IP, get_last_ip());

#if (USE_DEEPSLEEP)
  Blynk.setProperty(BLYNK_PIN_TIME_TO_DEEPSLEEP, "max", deepSleepMax_mins);
  Blynk.virtualWrite(BLYNK_PIN_DEEPSLEEP_BOOTCOUNT, String(bootCount));

#endif

  //change color to GREEN
  tempNormal        = true;
  humidNormal       = true;
  moistNormal       = true;
  RSSINormal        = true;

  Blynk.setProperty(BLYNK_PIN_TEMP, "color", BLYNK_GREEN);
  Blynk.setProperty(BLYNK_PIN_HUMID, "color", BLYNK_GREEN);
  Blynk.setProperty(BLYNK_PIN_MOIST, "color", BLYNK_GREEN);
  Blynk.setProperty(BLYNK_PIN_RSSI, "color", BLYNK_GREEN);

  moist_alarm   = false;

  updateBlynkStatus();
  Blynk.syncAll();
}

BLYNK_WRITE(BLYNK_PIN_RESET)
{
  if (param.asInt())
  {
    restart();
  }
}

DHTesp::DHT_MODEL_t DHTTYPE       = DHTesp::AUTO_DETECT;    // default Auto Detect
DHTesp::DHT_MODEL_t lastDHTTYPE;

//MW33 is one special type of DHT11. Needs conversion. Init is the same.
boolean MW33    = false;

BLYNK_WRITE(BLYNK_PIN_DHT_TYPE)
{
  switch (param.asInt())
  {
    case 1:
      { // Item 1 "Auto Detect"
        DHTTYPE = DHTesp::AUTO_DETECT;
        break;
      }
    case 2:
      { // Item 2
        DHTTYPE = DHTesp::DHT11;
        MW33 = false;
        break;
      }
    case 3:
      { // Item 3
        DHTTYPE = DHTesp::DHT11;
        MW33 = true;
        break;
      }
    case 4:
      { // Item 4
        DHTTYPE = DHTesp::DHT22;
        break;
      }
    case 5:
      { // Item 5
        DHTTYPE = DHTesp::AM2302;
        break;
      }
    case 6:
      { // Item 6
        DHTTYPE = DHTesp::RHT03;
        break;
      }
  }
}

boolean sensorCapacitive = true;

BLYNK_WRITE(BLYNK_PIN_MOIST_SENSOR_TYPE)
{
  switch (param.asInt())
  {
    case 1:
      { // Item 1 "Resistive"
        if (sensorCapacitive)
        {
          sensorCapacitive = false;
        }
        break;
      }
    case 2:
      { // Item 2
        if (!sensorCapacitive)
        {
          sensorCapacitive = true;
        }
        break;
      }
  }
}

BLYNK_WRITE(BLYNK_PIN_PUMP_MODE)
{
  switch (param.asInt())
  {
    case 1:
      { // Item 1 "Auto & Notice"
        pumpModeAuto = true;
        pumpModeNotice = true;
        break;
      }
    case 2:
      { // Item 2  "Auto & No Notice"
        pumpModeAuto = true;
        pumpModeNotice = false;
        break;
      }
    case 3:
      { // Item 3 "Manual & Notice"
        pumpModeAuto = false;
        pumpModeNotice = true;
        break;
      }
    case 4:
      { // Item 4  "Manual & No Notice"
        pumpModeAuto = false;
        pumpModeNotice = false;
        break;
      }
  }
}

// Default Send alarm msg of low moist every hour, in seconds. If 0 => No alarm
uint32_t alarm_moist_interval = 3600;

BLYNK_WRITE(BLYNK_PIN_MOIST_ALARM_INTERVAL)
{
  switch (param.asInt())
  {
    case 1:
      { // Item 1 "No Low Moisture Alarm"
        alarm_moist_interval = 0;
        break;
      }
    case 2:
      { // Item 2  "Every 1 hour"
        alarm_moist_interval = 3600;
        break;
      }
    case 3:
      { // Item 3 "Every 6 hrs"
        alarm_moist_interval = 3600 * 6;
        break;
      }
    case 4:
      { // Item 4  "Every 24 hrs"
        alarm_moist_interval = 3600 * 24;
        break;
      }
  }
}

BLYNK_WRITE(BLYNK_PIN_MOIST_ALARM_LEVEL)
{
  if (moist_alarm_level != param.asInt())
  {
    moist_alarm_level = param.asInt();
  }
}


int MIN_AIR_TEMPC_ALM = 10;
int MAX_AIR_TEMPC_ALM = 50;
int MIN_AIR_TEMPF_ALM = 50;
int MAX_AIR_TEMPF_ALM = 122;

#define MIN_TEMPC_ALM_LORANGE     0
#define MIN_TEMPC_ALM_HIRANGE     30
#define MAX_TEMPC_ALM_LORANGE     30
#define MAX_TEMPC_ALM_HIRANGE     50

#define MIN_TEMPF_ALM_LORANGE     32
#define MIN_TEMPF_ALM_HIRANGE     86
#define MAX_TEMPF_ALM_LORANGE     86
#define MAX_TEMPF_ALM_HIRANGE     122

boolean USE_CELCIUS = true;
BLYNK_WRITE(BLYNK_PIN_USE_CELCIUS)
{
  switch (param.asInt())
  {
    case 1:
      { // Item 1 "Celcius"
        if (!USE_CELCIUS)
        {
          USE_CELCIUS = true;
          Blynk.setProperty(BLYNK_PIN_MIN_AIR_TEMP, "min", MIN_TEMPC_ALM_LORANGE);
          Blynk.setProperty(BLYNK_PIN_MIN_AIR_TEMP, "max", MIN_TEMPC_ALM_HIRANGE);
          MIN_AIR_TEMPC_ALM = (uint) ((MIN_AIR_TEMPF_ALM - 32) * 5 ) / 9;

          Blynk.setProperty(BLYNK_PIN_MAX_AIR_TEMP, "min", MAX_TEMPC_ALM_LORANGE);
          Blynk.setProperty(BLYNK_PIN_MAX_AIR_TEMP, "max", MAX_TEMPC_ALM_HIRANGE);
          MAX_AIR_TEMPC_ALM = (uint) ((MAX_AIR_TEMPF_ALM - 32) * 5 ) / 9;
        }
        break;
      }
    case 2:
      { // Item 2
        if (USE_CELCIUS)
        {
          USE_CELCIUS = false;
          Blynk.setProperty(BLYNK_PIN_MIN_AIR_TEMP, "min", MIN_TEMPF_ALM_LORANGE);
          Blynk.setProperty(BLYNK_PIN_MIN_AIR_TEMP, "max", MIN_TEMPF_ALM_HIRANGE);
          MIN_AIR_TEMPF_ALM = (uint) ((MIN_AIR_TEMPC_ALM * 9) / 5 ) + 32;

          Blynk.setProperty(BLYNK_PIN_MAX_AIR_TEMP, "min", MAX_TEMPF_ALM_LORANGE);
          Blynk.setProperty(BLYNK_PIN_MAX_AIR_TEMP, "max", MAX_TEMPF_ALM_HIRANGE);
          MAX_AIR_TEMPF_ALM = (uint) ((MAX_AIR_TEMPC_ALM * 9) / 5 ) + 32;
        }
        break;
      }
  }
}

uint moist_adj_factor = 100;

BLYNK_WRITE(BLYNK_PIN_MOIST_ADJ_FACTOR)
{
  if (moist_adj_factor != param.asInt())
  {
    moist_adj_factor = param.asInt();
  }
}

uint DHT_ADJ_FACTOR = 100;

BLYNK_WRITE(BLYNK_PIN_DHT_ADJ_FACTOR)
{
  DHT_ADJ_FACTOR = param.asInt(); // Get value as integer
}

BLYNK_WRITE(BLYNK_PIN_MIN_AIR_TEMP)
{
  // Get value as integer
  if (USE_CELCIUS)
    MIN_AIR_TEMPC_ALM = param.asInt();
  else
    MIN_AIR_TEMPF_ALM = param.asInt();
}

BLYNK_WRITE(BLYNK_PIN_MAX_AIR_TEMP)
{
  // Get value as integer
  if (USE_CELCIUS)
    MAX_AIR_TEMPC_ALM = param.asInt();
  else
    MAX_AIR_TEMPF_ALM = param.asInt();
}

int MIN_AIR_HUMIDITY = 30;
BLYNK_WRITE(BLYNK_PIN_MIN_AIR_HUMIDITY)
{
  MIN_AIR_HUMIDITY = param.asInt(); // Get value as integer
}

int MAX_AIR_HUMIDITY = 95;
BLYNK_WRITE(BLYNK_PIN_MAX_AIR_HUMIDITY)
{
  MAX_AIR_HUMIDITY = param.asInt(); // Get value as integer
}

uint32_t DRY_SOIL = 68;
BLYNK_WRITE(BLYNK_PIN_DRY_SOIL)
{
  DRY_SOIL = param.asInt(); // Get value as integer
}

int WET_SOIL = 90;
BLYNK_WRITE(BLYNK_PIN_WET_SOIL)
{
  WET_SOIL = param.asInt(); // Get value as integer
}

unsigned long TIME_PUMP_ON = 15000L;
BLYNK_WRITE(BLYNK_PIN_TIME_PUMP_ON)
{
  TIME_PUMP_ON = param.asInt() * 1000; // Get value as seconds, converting to ms
}

// Use button V2 (BLYNK_PIN_PUMP_BUTTON) to control pump manually
BLYNK_WRITE(BLYNK_PIN_PUMP_BUTTON)
{
  if (param.asInt())
  {
    pumpStatus = !pumpStatus;
    control_pump(pumpStatus);
  }
}

#if USE_DEEPSLEEP
//Default 0 mins => disable deepsleep
static uint32_t DEEPSLEEP_INTERVAL_FACTOR = 0L;
BLYNK_WRITE(BLYNK_PIN_DEEPSLEEP_INTERVAL_FACTOR)
{
  if (DEEPSLEEP_INTERVAL_FACTOR != param.asInt())
  {
    DEEPSLEEP_INTERVAL_FACTOR = param.asInt();

    //Force interval between 1-60 (1-60 minutes)
    if (DEEPSLEEP_INTERVAL_FACTOR < 0)
      DEEPSLEEP_INTERVAL_FACTOR = 0;
    else if (DEEPSLEEP_INTERVAL_FACTOR > 60)
      DEEPSLEEP_INTERVAL_FACTOR = 60;
  }
}

// In minutes, default no deepsleep = 0 mins
static uint32_t TIME_TO_DEEPSLEEP = 0L;
BLYNK_WRITE(BLYNK_PIN_TIME_TO_DEEPSLEEP)
{
  if (TIME_TO_DEEPSLEEP != param.asInt())
  {
    TIME_TO_DEEPSLEEP = param.asInt();

    // Force deepsleep time between 1-2 days (1 min -> 2 days/2880 minutes).
    // TIME_TO_DEEPSLEEP = 0 => No DeepSleep
    if (TIME_TO_DEEPSLEEP > deepSleepMax_mins)
    {
      TIME_TO_DEEPSLEEP = deepSleepMax_mins;
      Blynk.virtualWrite(BLYNK_PIN_TIME_TO_DEEPSLEEP, deepSleepMax_mins);
    }
  }
}

BLYNK_WRITE(BLYNK_PIN_FORCE_DEEPSLEEP)
{
  if (param.asInt())
  {
    //Enter forced deepsleep only when pump is STOP
    safe_deepsleep();
  }
}

#endif

void updateBlynkStatus(void)
{
  // Update only when data is valid
  if (tempDHT != 0)
  {
    if ( (((tempDHT > MAX_AIR_TEMPC_ALM) || (tempDHT < MIN_AIR_TEMPC_ALM)) && USE_CELCIUS) ||
         (((tempF_DHT > MAX_AIR_TEMPF_ALM) || (tempF_DHT < MIN_AIR_TEMPF_ALM)) && !USE_CELCIUS) )
    {
      if (tempNormal)
      {
        tempNormal = false;
#if (DEBUG_LOOP > 1)
        Serial.println("Temp. to RED");
#endif
        //change color to RED
        Blynk.setProperty(BLYNK_PIN_TEMP, "color", BLYNK_RED);
      }
    }
    else
    {
      if (!tempNormal)
      {
        tempNormal = true;
#if (DEBUG_LOOP > 1)
        Serial.println("Temp. to GREEN");
#endif
        //change color to GREEN
        Blynk.setProperty(BLYNK_PIN_TEMP, "color", BLYNK_GREEN);
      }
    }
  }

  // Update only when data is valid
  if (humDHT != 0)
  {
    if ((humDHT > MAX_AIR_HUMIDITY) || (humDHT < MIN_AIR_HUMIDITY))
    {
      if (humidNormal)
      {
        humidNormal = false;
#if (DEBUG_LOOP > 1)
        Serial.println("Humid. to RED");
#endif
        //change color to RED
        Blynk.setProperty(BLYNK_PIN_HUMID, "color", BLYNK_RED);
      }
    }
    else
    {
      if (!humidNormal)
      {
        humidNormal = true;
#if (DEBUG_LOOP > 1)
        Serial.println("Humid. to GREEN");
#endif
        //change color to GREEN
        Blynk.setProperty(BLYNK_PIN_HUMID, "color", BLYNK_GREEN);
      }
    }
  }

  if (((soilMoist < moist_alarm_level) || (soilMoist > WET_SOIL)) && soilMoist > 0)
  {
    if (!moist_alarm)
    {
      moist_alarm = true;
#if (DEBUG_LOOP > 1)
      Serial.println("MoistAlarm ON");
#endif
    }
  }
  else
  {
    if (moist_alarm)
    {
      moist_alarm = false;
#if (DEBUG_LOOP > 1)
      Serial.println("MoistAlarm OFF");
#endif
    }
  }

  if (((soilMoist > WET_SOIL) || (soilMoist < DRY_SOIL)) && soilMoist > 0)
  {
    if (moistNormal)
    {
      moistNormal = false;
#if (DEBUG_LOOP > 1)
      Serial.println("Moist to RED");
#endif
      //change color to RED
      Blynk.setProperty(BLYNK_PIN_MOIST, "color", BLYNK_RED);
    }
  }
  else
  {
    if (!moistNormal)
    {
      moistNormal = true;
#if (DEBUG_LOOP > 1)
      Serial.println("Moist to GREEN");
#endif
      //change color to GREEN
      Blynk.setProperty(BLYNK_PIN_MOIST, "color", BLYNK_GREEN);
    }
  }


#define TESTING_RSSI_BOUNDARY    false
#if TESTING_RSSI_BOUNDARY
  static int testCount = 0;
  curr_RSSI = -(testCount);
  Serial.printf("curr_RSSI = %d\n", curr_RSSI);
  testCount = (testCount + 5) % 100;
#endif

  if (curr_RSSI < RSSI_WEAK)
  {
    if (RSSINormal)
    {
      RSSINormal = false;
#if (DEBUG_LOOP > 1)
      Serial.println("RSSI to RED");
#endif
      //change color to RED
      Blynk.setProperty(BLYNK_PIN_RSSI, "color", BLYNK_RED);
    }
  }
  else
  {
    if (!RSSINormal)
    {
      RSSINormal = true;
#if (DEBUG_LOOP > 1)
      Serial.println("RSSI to GREEN");
#endif
      //change color to GREEN
      Blynk.setProperty(BLYNK_PIN_RSSI, "color", BLYNK_GREEN);
    }
  }
}

float fmap(float x, long in_min, long in_max, long out_min, long out_max)
{
  return (float) ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

void getSoilMoist(void)
{
#define KAVG      10

  static int i;
  static int temp_soilMoist;

  temp_soilMoist = 0;

  for (i = 0; i < KAVG; i++)  //
  {
    temp_soilMoist += analogRead(SOIL_MOIST_PIN);
    delay(20);
  }
  temp_soilMoist = temp_soilMoist / KAVG;

#if 0
  soilMoist = (fmap(temp_soilMoist, 1024, 0, 0, 100) * moist_adj_factor) / 100.0;
#else
  if (sensorCapacitive)
  {
#if USE_ESP32
    // Capacitive Sensor testing: Air => 3250/3390, Water => 1600/1520. Adjust the range correspondingly
    soilMoist = (fmap(temp_soilMoist, 3250, 1600, 0, 100) * moist_adj_factor) / 100.0;
    //soilMoist = (fmap(temp_soilMoist, 4095, 0, 0, 100) * moist_adj_factor) / 100.0;
#else
    // Capacitive Sensor testing: Air => 830, Water => 360. Adjust the range correspondingly
    soilMoist = (fmap(temp_soilMoist, 830, 360, 0, 100) * moist_adj_factor) / 100.0;
    //soilMoist = (fmap(temp_soilMoist, 1024, 360, 0, 100) * moist_adj_factor) / 100.0;
#endif
  }
  else
  {
    // Resistive Sensor testing: Air => 1024, Water => 350. Adjust the range correspondingly
    soilMoist = (fmap(temp_soilMoist, 1024, 350, 0, 100) * moist_adj_factor) / 100.0;
  }
#endif

  if (soilMoist < 0)
    soilMoist = 0.0;
  if (soilMoist > 100.0)
    soilMoist = 100.0;

#if (DEBUG_LOOP > 1)
  Serial.printf("getSoilMoist: Analog Read temp_soilMoist = %d, soilMoist = %4.1f\n", temp_soilMoist, soilMoist);
#endif

#define TESTING_MOIST_BOUNDARY    false
#if TESTING_MOIST_BOUNDARY
  static int testCount = 0;
  soilMoist = testCount;
  Serial.printf("soilMoist = %5.2f\n", soilMoist);
  testCount = (testCount + 3) % 100;
#endif
}

void getDhtData(void)
{
  static int8_t thisTemp;
  static float v;

  tempDHT = 0;
  humDHT  = 0;


  if (dht)
  {
    TempAndHumidity th = dht->getTempAndHumidity();

#if DHT_DEBUG
    if (isnan(th.temperature) || isnan(th.humidity))
      Serial.println("read_TH_sensor: Temp. or Humid. is NAN");
    else
      Serial.printf("read_TH_sensor: Temp. = %5.2f, Humidity = %5.2f\n", th.temperature, th.humidity);
#endif

    v = th.temperature;
    if (!isnan(v))
    {
      if (!MW33)
      {
        tempDHT = v;
      }
      else
      {
        // MW33 temp. from -20 -> 60 degC, have to take care of negative
        // Lower then 0x7F => positive. Over 0x80 => Negative
        thisTemp = (int8_t) v;
        if (thisTemp & 0x80)
        {
          thisTemp = - (thisTemp & 0x7F);
          if (thisTemp < -20)
            thisTemp = -20;
        }
        else if (thisTemp > 60)
          thisTemp = 60;

        tempDHT = (float) thisTemp;
      }

      tempF_DHT = tempDHT * 1.8 + 32.0;
    }

    v = th.humidity;
    if (!isnan(v))
    {
      if (!MW33)
        humDHT = v;
      else
      {
#define MW33_HUMID_OFFSET     15
#define MW33_MIN_HUMID        (float) 5.0
#define MW33_MAX_HUMID        (float) 95.0
#define DHT11_MIN_HUMID       (float) 20.0
#define DHT11_MAX_HUMID       (float) 90.0
        //(DHT11_MAX_HUMID - DHT11_MIN_HUMID) / (MW33_MAX_HUMID - MW33_MIN_HUMID)
#define DHT11_MW33_COEF       (float) 0.7777778

        // Scaling
        humDHT = (v - MW33_MIN_HUMID) * ( DHT11_MW33_COEF ) + DHT11_MIN_HUMID + MW33_HUMID_OFFSET;
      }
      //Adjustment
      humDHT = (humDHT * DHT_ADJ_FACTOR) / 100;
      if (humDHT > 95.0)
        humDHT = 95.0;
    }

    // Update only when data is valid
    HeatIndexDHT = 0;
    if ((tempDHT != 0) && (humDHT != 0))
    {
      HeatIndexDHT    = dht->computeHeatIndex(tempDHT, humDHT, false);
      HeatIndexF_DHT  = HeatIndexDHT * 1.8 + 32;
    }
  }

#define TESTING_DHT_BOUNDARY    false
#if TESTING_DHT_BOUNDARY
  static int testCount = 0;
  tempDHT   = testCount;
  tempF_DHT = tempDHT * 1.8 + 32;
  humDHT  = (testCount + 5) % 100;
  Serial.printf("tempDHT = %5.2f, tempF_DHT = %5.2, humDHT = %5.2f\n", tempDHT, tempF_DHT, humDHT);
  testCount = (testCount + 3) % 100;
#endif
}

void set_led(byte status)
{
  digitalWrite(PIN_LED, status);
}

void control_pump(boolean action)
{
#if RELAY_ACTIVE_HIGH
  digitalWrite(PUMP_RELAY_PIN, action);
  if (digitalRead(PUMP_RELAY_PIN) == HIGH)
#else
  digitalWrite(PUMP_RELAY_PIN, !action);
  if (digitalRead(PUMP_RELAY_PIN) == LOW)
#endif    //RELAY_ACTIVE_HIGH
  {
#if (DEBUG_LOOP > 1)
    Serial.println("Turn Pump ON");
#endif

    pumpStatus = RUN;

    if (pumpModeNotice)
      Blynk.notify("Pump ON");
  }
  else
  {
#if (DEBUG_LOOP > 1)
    Serial.println("Turn Pump OFF");
#endif

    pumpStatus = STOP;

    if (pumpModeNotice)
      Blynk.notify("Pump OFF");
  }
}

void addWater()
{
  control_pump(RUN);
  // TIME_PUMP_ON = 15s (selected from from 10-50s)
  relay_ticker.once_ms(TIME_PUMP_ON, control_pump, STOP);
}

void controlMoisture(void)
{
  if ((soilMoist < DRY_SOIL) && (soilMoist > 0 ))
  {
#if (DEBUG_LOOP > 1)
    Serial.printf("soilMoist = %5.2f < DRY_SOIL\n", soilMoist);
#endif

    addWater();
  }
#if (DEBUG_LOOP > 1)
  else
    Serial.printf("soilMoist = %5.2f => OK\n", soilMoist);
#endif
}

#if USE_DEEPSLEEP

#if USE_ESP32

//Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

#if (DEBUG_LOOP > 1)
  Serial.print("Wakeup by ");

  switch (wakeup_reason)
  {
    case 1  : Serial.println("RTC_IO"); break;
    case 2  : Serial.println("RTC_CNTL"); break;
    case 3  : Serial.println("timer"); break;
    case 4  : Serial.println("touchpad"); break;
    case 5  : Serial.println("ULP"); break;
    default : Serial.println("not deepsleep"); break;
  }
#endif
}

#endif

uint64_t uS_TO_SLEEP = 0;

void deepsleep()
{
  if (TIME_TO_DEEPSLEEP > 0)
  {
    uS_TO_SLEEP = (uint64_t) TIME_TO_DEEPSLEEP * uS_TO_MIN_FACTOR;
#if 1 //(DEBUG_LOOP > 1)
    Serial.printf("Blynk disconnecting.Start DeepSleep. TIME_TO_DEEPSLEEP = %ld, Time = %lld us\n", TIME_TO_DEEPSLEEP, uS_TO_SLEEP);
#endif

    Blynk.disconnect();
    WiFi.mode(WIFI_OFF);

#if USE_ESP32
    // Store important vars to RTC memory
    RTC_DEEPSLEEP_INTERVAL_FACTOR = DEEPSLEEP_INTERVAL_FACTOR;
    RTC_TIME_TO_DEEPSLEEP         = TIME_TO_DEEPSLEEP;

    RTC_DRY_SOIL                  = DRY_SOIL;
    RTC_DHTTYPE                   = DHTTYPE;

#if USE_BITFIELD
    RTC_Data.MW33             = (uint32_t) MW33;
    RTC_Data.sensorCapacitive = (uint32_t) sensorCapacitive;
    RTC_Data.USE_CELCIUS      = (uint32_t) USE_CELCIUS;
#else
    // Better to use bit field for boolean data
    RTC_MW33                      = MW33;
    RTC_sensorCapacitive          = sensorCapacitive;
    RTC_USE_CELCIUS               = USE_CELCIUS;
#endif

    esp_sleep_enable_timer_wakeup(uS_TO_SLEEP); // Setup wake up interval
    delay(500);
    esp_deep_sleep_start(); // Start deep sleep
#else

    // Store important vars to RTC memory
    ESP.rtcUserMemoryWrite(bootCountOffset + sizeof(bootCount), &DEEPSLEEP_INTERVAL_FACTOR, sizeof(DEEPSLEEP_INTERVAL_FACTOR));
    ESP.rtcUserMemoryWrite(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR), &TIME_TO_DEEPSLEEP, sizeof(TIME_TO_DEEPSLEEP));

    ESP.rtcUserMemoryWrite(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP), &DRY_SOIL, sizeof(DRY_SOIL));

    RTC_DHTTYPE = (uint32_t) DHTTYPE;
    ESP.rtcUserMemoryWrite(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                           sizeof(DRY_SOIL), &RTC_DHTTYPE, sizeof(RTC_DHTTYPE));


#if USE_BITFIELD
    RTC_Data.MW33             = (uint32_t) MW33;
    RTC_Data.sensorCapacitive = (uint32_t) sensorCapacitive;
    RTC_Data.USE_CELCIUS      = (uint32_t) USE_CELCIUS;

    //uint32_t temp_RTC_Data = (uint32_t) RTC_Data;
    ESP.rtcUserMemoryWrite(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                           sizeof(DRY_SOIL) + sizeof(RTC_DHTTYPE), (uint32_t *) &RTC_Data, sizeof(uint32_t));

#else
    RTC_MW33 = (uint32_t) MW33;
    ESP.rtcUserMemoryWrite(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                           sizeof(DRY_SOIL) + sizeof(RTC_DHTTYPE), &RTC_MW33, sizeof(RTC_MW33));

    RTC_sensorCapacitive = (uint32_t) sensorCapacitive;
    ESP.rtcUserMemoryWrite(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                           sizeof(DRY_SOIL) + sizeof(RTC_DHTTYPE) + sizeof(RTC_MW33), &RTC_sensorCapacitive, sizeof(RTC_sensorCapacitive));

    RTC_USE_CELCIUS = (uint32_t) USE_CELCIUS;
    ESP.rtcUserMemoryWrite(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                           sizeof(DRY_SOIL) + sizeof(RTC_DHTTYPE) + sizeof(RTC_MW33) + sizeof(sensorCapacitive), &RTC_USE_CELCIUS, sizeof(RTC_USE_CELCIUS));
#endif

    delay(500);
    ESP.deepSleep(uS_TO_SLEEP); // Setup wake up interval

    // Use this if wakeup from external SW
    //esp_sleep_enable_ext0_wakeup(PIN_D26, HIGH /*LOW*/);
#endif
  }
#if 1 //(DEBUG_LOOP > 1)
  else
    Serial.println("Not entering DeepSleep as TIME_TO_DEEPSLEEP = 0");
#endif
}

void safe_deepsleep()
{
  // Be sure pump if STOP before deepsleep
  if (pumpStatus == RUN)
    control_pump(STOP);
  if (pumpStatus == STOP)
  {
    delay(1000);
    deepsleep();
  }
}

#endif

int long imap(int x, int in_min, int in_max, int out_min, int out_max)
{
  int result = ((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);

  if (result > 100)
    return 100;
  else if (result < 0)
    return 0;
  else
    return result;
}

#define TIME_SYNC_TIMEOUT     1800
static ulong curr_time        = 0;
static ulong curr_hour        = 0;

void check_status()
{
  static ulong checkstatus_timeout              = 0;
  static ulong checkstatus_report_timeout       = 0;
  static ulong checkstatus_moist_timeout        = 0;
  static ulong checkstatus_moist_alarm_timeout  = 0;

#define ONE_MINUTE_IN_SEC         60
#define STATUS_CHECK_INTERVAL     5
#define STATUS_REPORT_INTERVAL    (STATUS_CHECK_INTERVAL * 2)
#define CONTROL_MOIST_INTERVAL    ONE_MINUTE_IN_SEC

  // Send status report every STATUS_REPORT_INTERVAL (10) seconds: we don't need to send updates frequently if there is no status change.
  if ((curr_time > checkstatus_timeout) || (checkstatus_timeout == 0))
  {

#if USE_DEEPSLEEP
    // Check to go to deepsleep every 60x2 = 2 mins if moistNormal
    static unsigned long DEEPSLEEP_INTERVAL;
    static ulong deepsleep_timeout = 0;
    static ulong deepsleep_factor  = DEEPSLEEP_INTERVAL_FACTOR;
    static ulong prev_time = 0;

    DEEPSLEEP_INTERVAL = (ONE_MINUTE_IN_SEC * DEEPSLEEP_INTERVAL_FACTOR);

    // Update deepsleep_timeout when first reset/wakeup or changing DEEPSLEEP_INTERVAL_FACTOR or Time has been synchronized / Blynk just connected
    if ( ((deepsleep_timeout == 0) && (curr_time > 0)) || (deepsleep_factor != DEEPSLEEP_INTERVAL_FACTOR) || ( curr_time > prev_time + 1451602800 ) )
    {
      deepsleep_timeout = curr_time + DEEPSLEEP_INTERVAL;

      if (deepsleep_factor != DEEPSLEEP_INTERVAL_FACTOR)
        deepsleep_factor = DEEPSLEEP_INTERVAL_FACTOR;

#if (DEBUG_LOOP > 1)
      Serial.printf("check_status: Update deepsleep_timeout => prev_time = %ld, curr_time = %ld, deepsleep_timeout = %ld\n", prev_time, curr_time, deepsleep_timeout);
#endif
    }

    prev_time = curr_time;
#endif

    // Restart if selected DHT sensor has changed
    if (lastDHTTYPE != DHTTYPE)
      restart();

    getDhtData();
    getSoilMoist();

    //Send current status only on change and longer interval
    if (curr_time > checkstatus_report_timeout)
    {
#if (DEBUG_LOOP > 1)
      if (pumpStatus)
        Serial.println("Pump ON");
      else
        Serial.println("Pump OFF");
#endif
      // report status to Blynk
      if (Blynk.connected())
      {

#if USE_ESP32
        //ESP32 LED_BUILDIN is correct polarity, HIGH to turn ON
        set_led(HIGH);
        aux_ticker.once_ms(111, set_led, (byte) LOW);
#else
        //ESP8266 LED_BUILDIN is reversed polarity, LOW to turn ON
        set_led(LOW);
        aux_ticker.once_ms(111, set_led, (byte) HIGH);
#endif

#if (DEBUG_LOOP > 0)
        Serial.println("B");
#endif

        static String RSSI;
        RSSI = "";
        curr_RSSI = (int16_t) WiFi.RSSI();

#define DISPLAY_RSSI_PERCENT      true

#if DISPLAY_RSSI_PERCENT
        percent_RSSI = (int16_t) imap((int) curr_RSSI, RSSI_0_PERCENT, RSSI_100_PERCENT, 0, 100);
        RSSI += percent_RSSI;
        RSSI += "%";
#else
        RSSI += (curr_RSSI > RSSI_OK) ? "OK" : ((curr_RSSI > RSSI_WEAK) ? "Wk" : "Bad");
        RSSI += "(";
        RSSI += curr_RSSI;
        RSSI += " dBm)";
#endif

        Blynk.virtualWrite(BLYNK_PIN_RSSI, RSSI);

        if (USE_CELCIUS)
        {
          //Use 1 decimal place for degC
          Blynk.virtualWrite(BLYNK_PIN_TEMP, String(tempDHT, 1));
          // Temp C only for Charting
          Blynk.virtualWrite(BLYNK_PIN_TEMPCHART, String(tempDHT));
          Blynk.virtualWrite(BLYNK_PIN_HEAT_INDEX, String(HeatIndexDHT, 1));
        }
        else
        {
          //Use 1 decimal place for degF
          Blynk.virtualWrite(BLYNK_PIN_TEMP, String(tempF_DHT, 1));
          // Temp F only for Charting
          Blynk.virtualWrite(BLYNK_PIN_TEMPCHART, String(tempF_DHT));
          Blynk.virtualWrite(BLYNK_PIN_HEAT_INDEX, String(HeatIndexF_DHT, 1));
        }

#if (DEBUG_LOOP > 1)
        Serial.printf("check_status: Blynk soilMoist = %4.1f \n", soilMoist);
#endif

        Blynk.virtualWrite(BLYNK_PIN_MOIST, String(soilMoist, 1));

        //Use 1 decimal place for Humidity
        Blynk.virtualWrite(BLYNK_PIN_HUMID, String(humDHT, 1));

        updateBlynkStatus();

        // Send status report every STATUS_REPORT_INTERVAL (15) seconds: we don't need to send updates frequently if there is no status change.
        checkstatus_report_timeout = curr_time + STATUS_REPORT_INTERVAL;
      }
#if (DEBUG_LOOP > 0)
      else
      {
        Serial.println("F");
      }
#endif
    }

#define TEST_AUTO_PUMP  0

    if (curr_time > checkstatus_moist_timeout)
    {
#if TEST_AUTO_PUMP
      //Testing auto pump ON by forcing soilMoist < DRY_SOIL
      static ulong test_count = 0;
      test_count++;
      Serial.printf("Count = %ld\n", test_count);
      if (test_count % 2 == 0)
      {
        soilMoist = DRY_SOIL - 1;
      }
#endif

#if USE_DEEPSLEEP
      //enter deepsleep only when moistNormal and DEEPSLEEP_INTERVAL != 0
      if ( (DEEPSLEEP_INTERVAL > 0) && (curr_time > deepsleep_timeout))
      {
#if (DEBUG_LOOP > 1)
        Serial.printf("curr_time = %ld, deepsleep_timeout = %ld\n", curr_time, deepsleep_timeout);
#endif
        deepsleep_timeout = curr_time + DEEPSLEEP_INTERVAL;

        if ((moistNormal) || (soilMoist == 0))
        {
          // Be sure pump STOP before deepsleep
          safe_deepsleep();
        }
#if (DEBUG_LOOP > 1)
        else
          Serial.println("Moist Abnormal, don't deepsleep");
#endif
      }
#if (DEBUG_LOOP > 1)
      else if (DEEPSLEEP_INTERVAL == 0)
        Serial.println("DEEPSLEEP_INTERVAL = 0, don't deepsleep");
#endif
#endif    //#if USE_DEEPSLEEP

      if (pumpModeAuto)
        controlMoisture();

      if (moist_alarm)
      {
#if (DEBUG_LOOP > 1)
        Serial.printf("Alarm Hi/Low soilMoist = %5.2f, alarm interval = %d\n", soilMoist, alarm_moist_interval);
#endif

        if ( (alarm_moist_interval != 0)  && (curr_time > checkstatus_moist_alarm_timeout) )
        {
#if USE_BLYNK_WM
          String notification = Blynk.getBoardName() + " SoilMoist";
#else
          String notification = "SF " + String(SMART_FARM_BOARD_NO) + " SoilMoist";
#endif

          if (soilMoist < moist_alarm_level)
          {
            notification = notification + " Lo Alarm";
#if (DEBUG_LOOP > 1)
            Serial.println(notification);
#endif
            Blynk.notify(notification);
          }
          else
          {
            notification = notification + " Hi Alarm";
#if (DEBUG_LOOP > 1)
            Serial.println(notification);
#endif
            Blynk.notify(notification);
          }
          checkstatus_moist_alarm_timeout = curr_time + alarm_moist_interval;
        }
      }
      else
      {
#if (DEBUG_LOOP > 1)
        Serial.printf("OK soilMoist = %5.2f, alarm interval = %d\n", soilMoist, alarm_moist_interval);
        Serial.printf("curr = %d, moist_alarm_timeout = %d\n", curr_time, checkstatus_moist_alarm_timeout);
#endif

        // Moisture returns to normal, reset the alarm interval, so that alarm in next check_moist_status if abnormal again
        checkstatus_moist_alarm_timeout = curr_time;
      }

      checkstatus_moist_timeout = curr_time + CONTROL_MOIST_INTERVAL;
    }

    checkstatus_timeout = curr_time + STATUS_CHECK_INTERVAL;
  }
}

void time_keeping()
{
  static ulong prev_millis = 0;
  static ulong time_keeping_timeout = 0;
  static ulong gt;

  if (!curr_time || (curr_time > time_keeping_timeout))
  {
    // get time from Blynk RTC, not from NTP

#if USE_BLYNK_RTC
    gt = now();
#else
    gt = time(nullptr);
#endif

    //if (!gt)
    // Fix NTP bug in core 2.4.1/SDK 2.2.1 (returns Thu Jan 01 08:00:10 1970 after power on)
    if (gt < 1451602800)
    {
      // if we didn't get response, re-try after 60 seconds
      DEBUG_PRINTLN("Failed updating time");
      time_keeping_timeout = curr_time + 60;    //2;
    }
    else
    {
      curr_time = gt;
      curr_hour = (curr_time % 86400) / 3600;
      DEBUG_PRINT("Updated time: ");
      DEBUG_PRINT(curr_time);
      DEBUG_PRINT(" Hour: ");
      DEBUG_PRINTLN(curr_hour);
      // if we got a response, re-try after TIME_SYNC_TIMEOUT (1800) seconds
      time_keeping_timeout = curr_time + TIME_SYNC_TIMEOUT;
      prev_millis = millis();
    }
  }

#if 1
  static ulong delta_millis;

  delta_millis = millis() - prev_millis;
  curr_time += delta_millis / 1000;
  prev_millis += (delta_millis / 1000) * 1000;

#else

  while (millis() - prev_millis >= 1000)
  {
    curr_time ++;
    prev_millis += 1000;
    delay(0);
  }

#endif

  curr_hour = (curr_time % 86400) / 3600;
}

void updatePumpLED()
{
#if 1
#if RELAY_ACTIVE_HIGH
  if (digitalRead(PUMP_RELAY_PIN) == HIGH)
#else
  if (digitalRead(PUMP_RELAY_PIN) == LOW)
#endif    //RELAY_ACTIVE_HIGH
  {
    //Override pumpStatus to reflect reality. There must be something wrong: shorted circuit, etc...
    pumpStatus = RUN;
    blynk_led_pump_on.on();
    blynk_led_pump_off.off();
  }
  else
  {
    //Override pumpStatus to reflect reality. There must be something wrong: shorted circuit, etc...
    pumpStatus = STOP;
    blynk_led_pump_on.off();
    blynk_led_pump_off.on();
  }
#endif
}

void startTimers(void)
{
  timer.setInterval(5000L, clockDisplay);  // update clock every 5 seconds
  // check every second if time has been obtained from the server
  timer.setInterval(1011L, time_keeping);
  timer.setInterval(1512L, updatePumpLED);
}

// Use this to avoid being blocked here if no WiFi
void connectWiFi(const char* ssid, const char* pass)
{
#if (DEBUG_LOOP > 1)
  Serial.printf("Connecting to %s\n", ssid);
#endif

  WiFi.mode(WIFI_STA);
  if (pass && strlen(pass))
  {
    WiFi.begin(ssid, pass);
  } else
  {
    WiFi.begin(ssid);
  }
  int i = 0;
  while ((i++ < 30) && (WiFi.status() != WL_CONNECTED))
  {
    BlynkDelay(500);
  }

#if (DEBUG_LOOP > 1)
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("Connected to WiFi");
  else
    Serial.println("Can't connect to WiFi");
#endif
}


void setup()
{
  Serial.begin(115200);

#if USE_ESP32
  adcAttachPin(SOIL_MOIST_PIN);
  // Use 12bit ADC, 0-4095
  analogSetWidth(12);
#endif

#if USE_ESP32
  firmware_version = firmware_version + "w";
#else
  firmware_version = firmware_version + "d";
#endif

#if USE_SSL
  firmware_version = firmware_version + "s";
#endif



  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
  control_pump(STOP);

  Serial.println("\nStarting");

#if (USE_DEEPSLEEP)

#if (USE_ESP32)
  // Read and restore important vars from RTC memory and only if they're valid by checking bootCount > 0
  if (bootCount > 0)
  {
    DEEPSLEEP_INTERVAL_FACTOR = RTC_DEEPSLEEP_INTERVAL_FACTOR;
    TIME_TO_DEEPSLEEP         = RTC_TIME_TO_DEEPSLEEP;

    DRY_SOIL                  = RTC_DRY_SOIL;
    DHTTYPE                   = RTC_DHTTYPE;

#if USE_BITFIELD
    MW33              = (boolean) RTC_Data.MW33;
    sensorCapacitive  = (boolean) RTC_Data.sensorCapacitive;
    USE_CELCIUS       = (boolean) RTC_Data.USE_CELCIUS;
#else
    // Better to use bit field for boolean data
    MW33              = (boolean) RTC_MW33;
    sensorCapacitive  = (boolean) RTC_sensorCapacitive;
    USE_CELCIUS       = (boolean) RTC_USE_CELCIUS;
#endif
  }

  //Increment boot number and print it every reboot
  ++bootCount;

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

#else   //#if (USE_ESP32)

  struct rst_info *pResetInfo = ESP.getResetInfoPtr();

  if ((pResetInfo->reason == REASON_DEEP_SLEEP_AWAKE))
  {
    // Read stored bootCount from RTC memory if booting from DeepSleep
    ESP.rtcUserMemoryRead(bootCountOffset, &bootCount, sizeof(bootCount));

    // Read and restore important vars from RTC memory and only if they're valid by checking bootCount > 0
    if (bootCount > 0)
    {
      ESP.rtcUserMemoryRead(bootCountOffset + sizeof(bootCount), &DEEPSLEEP_INTERVAL_FACTOR, sizeof(DEEPSLEEP_INTERVAL_FACTOR));
      ESP.rtcUserMemoryRead(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR), &TIME_TO_DEEPSLEEP, sizeof(TIME_TO_DEEPSLEEP));

      ESP.rtcUserMemoryRead(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP), &DRY_SOIL, sizeof(DRY_SOIL));
      ESP.rtcUserMemoryRead(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                            sizeof(DRY_SOIL), &RTC_DHTTYPE, sizeof(RTC_DHTTYPE));
      DHTTYPE = (DHTesp::DHT_MODEL_t) RTC_DHTTYPE;


#if USE_BITFIELD
      ESP.rtcUserMemoryRead(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                            sizeof(DRY_SOIL) + sizeof(RTC_DHTTYPE), (uint32_t *) &RTC_Data, sizeof(uint32_t));
      MW33              = (boolean) RTC_Data.MW33;
      sensorCapacitive  = (boolean) RTC_Data.sensorCapacitive;
      USE_CELCIUS       = (boolean) RTC_Data.USE_CELCIUS;

#else
      ESP.rtcUserMemoryRead(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                            sizeof(DRY_SOIL) + sizeof(RTC_DHTTYPE), &RTC_MW33, sizeof(RTC_MW33));
      MW33 = (boolean) RTC_MW33;

      ESP.rtcUserMemoryRead(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                            sizeof(DRY_SOIL) + sizeof(RTC_DHTTYPE) + sizeof(RTC_MW33), &RTC_sensorCapacitive, sizeof(RTC_sensorCapacitive));
      sensorCapacitive = (boolean) RTC_sensorCapacitive;

      ESP.rtcUserMemoryRead(bootCountOffset + sizeof(bootCount) + sizeof(DEEPSLEEP_INTERVAL_FACTOR) + sizeof(TIME_TO_DEEPSLEEP) +
                            sizeof(DRY_SOIL) + sizeof(RTC_DHTTYPE) + sizeof(RTC_MW33) + sizeof(RTC_sensorCapacitive), &RTC_USE_CELCIUS, sizeof(RTC_USE_CELCIUS));
      USE_CELCIUS = (boolean) RTC_USE_CELCIUS;
#endif

    }


#if (DEBUG_LOOP > 1)
    Serial.println("Read BootCount from RTC: " + String(bootCount));
#endif
  }

  //Increment boot number and print it every reboot
  ++bootCount;

  // Store current bootCount back in RTC memory
  ESP.rtcUserMemoryWrite(bootCountOffset, &bootCount, sizeof(bootCount));

  // ESP8266 has limit deepSleepMax ~ 230 mins
  deepSleepMax_mins = (uint64_t) ESP.deepSleepMax() / uS_TO_MIN_FACTOR;
  deepSleepMax_mins = deepSleepMax_mins - deepSleepMax_mins % 10;

#if 1 //(DEBUG_LOOP > 1)
  Serial.printf("ESP8266: Max deepSleep = %lld minutes\n", deepSleepMax_mins);
#endif

#endif    //(USE_ESP32)

#if (DEBUG_LOOP > 0)
  Serial.println("Boot number: " + String(bootCount));
  if (bootCount > 1)
  {
    Serial.printf("Restore DEEPSLEEP_INTERVAL_FACTOR = %ld, TIME_TO_DEEPSLEEP = %ld, DRY_SOIL = %ld, DHTTYPE = %ld\n", DEEPSLEEP_INTERVAL_FACTOR, TIME_TO_DEEPSLEEP, DRY_SOIL, (ulong) DHTTYPE);
    Serial.printf("Restore MW33 = %s, sensorCapacitive = %s, USE_CELCIUS = %s\n", MW33 ? "true" : "false", sensorCapacitive ? "Capacitive" : "Resistive", USE_CELCIUS ? "true" : "false" );
  }
#endif


#endif    //USE_DEEPSLEEP

#if 1
#if USE_BLYNK_WM
  Blynk.begin();
#else
  //WiFi.begin(ssid.c_str(), pass.c_str());
  // Use this to avoid being blocked here if no WiFi
  connectWiFi(ssid.c_str(), pass.c_str());
  Blynk.config(blynk_token.c_str(), cloudBlynkServer.c_str(), BLYNK_SERVER_HARDWARE_PORT);
#if (DEBUG_LOOP > 1)
  Serial.println("Done Blynk.config()");
#endif
  Blynk.connect();
#if (DEBUG_LOOP > 1)
  Serial.println("Done Blynk.connect()");
#endif
#endif
#else
  // Use this is bad, and will be blocked here if no WiFi
  Blynk.begin(blynk_token.c_str(), ssid.c_str(), pass.c_str(), cloudBlynkServer.c_str(), BLYNK_SERVER_HARDWARE_PORT);
#endif

  if (Blynk.connected())
  {
    updateBlynkStatus();
    Serial.println("Connected to BlynkServer");
    lastDHTTYPE = DHTTYPE;
    dht = new DHTesp();
    dht->setup(DHTPIN, DHTTYPE);
  }

  startTimers();
}

void loop()
{
  Blynk.run();
  timer.run();

  check_status();
}
