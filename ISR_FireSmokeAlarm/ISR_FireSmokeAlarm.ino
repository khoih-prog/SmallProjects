/****************************************************************************************************************************
   ISR_FireSmokeAlarm.ino
   ISR-based Fire and Smoke Alarm and Blynk Notification
   For ESP8266 boards
   Written by Khoi Hoang
   Copyright (c) 2019 Khoi Hoang

   Built by Khoi Hoang https://github.com/khoih-prog/SmallProjects/ISR_FireSmokeAlarm
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
    1.0.0   K Hoang     24/11/2019 Initial coding
    1.0.2   K Hoang     28/11/2019 Move to ISR-based to demonstrate how to use ISR-based timed in mission-critical projects
*****************************************************************************************************************************/
/****************************************************************************************************************************
   This example will demonstrate the inherently un-blockable feature of ISR-based timers compared to software timers..
   Being ISR-based timers, their executions are not blocked by bad-behaving functions / tasks, such as connecting to WiFi, Internet
   and Blynk services. You can also have many (up to 16) timers to use.
   This non-being-blocked important feature is absolutely necessary for mission-critical tasks.
   You'll see blynkTimer is blocked while connecting to WiFi / Internet / Blynk, or some other possible bad behaving tasks..
 *****************************************************************************************************************************/
//These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
#define TIMER_INTERRUPT_DEBUG       1

#define LOCAL_DEBUG                 1

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>

//You have to download Blynk WiFiManager Blynk_WM library at //https://github.com/khoih-prog/Blynk_WM
// In order to enable (USE_BLYNK_WM = true). Otherwise, use (USE_BLYNK_WM = false)
#define USE_BLYNK_WM   true
//#define USE_BLYNK_WM   false

#define USE_SSL     false

#if USE_BLYNK_WM
#if USE_SSL
#include <BlynkSimpleEsp8266_SSL_WM.h>        //https://github.com/khoih-prog/Blynk_WM
#else
#include <BlynkSimpleEsp8266_WM.h>            //https://github.com/khoih-prog/Blynk_WM
#endif
#else
#if USE_SSL
#include <BlynkSimpleEsp8266_SSL.h>
#define BLYNK_HARDWARE_PORT     9443
#else
#include <BlynkSimpleEsp8266.h>
#define BLYNK_HARDWARE_PORT     8080
#endif
#endif

#if !USE_BLYNK_WM
#define USE_LOCAL_SERVER    true

// If local server
#if USE_LOCAL_SERVER
char blynk_server[]   = "yourname.duckdns.org";
//char blynk_server[]   = "192.168.2.110";
#else
char blynk_server[]   = "";
#endif

char auth[]     = "***";
char ssid[]     = "***";
char pass[]     = "***";

#endif

#include "ESP8266TimerInterrupt.h"
#include "ESP8266_ISR_Timer.h"

#include <Ticker.h>

// Init ESP8266 ISR-based timer
#define HW_TIMER_INTERVAL_MS        50
ESP8266Timer ITimer;

// Init ESP8266_ISR_Timer to provide 16 ISR-based timer
ESP8266_ISR_Timer ISR_Timer;

BlynkTimer blynkTimer;

#define BLYNK_PIN_FIRE_TEST       V0     // ON/OFF Button
#define BLYNK_PIN_SMOKE_TEST      V1     // ON/OFF Button
#define BLYNK_PIN_BUZZER_CONTROL  V2     // PUSH Button,to be safe so that buzzer will sound later

#define BLYNK_PIN_GAS_HIGH_VALUE  V3      // SLIDER to test Gas HIGH Value Alarm
#define BLYNK_PIN_GAS_WARN_VALUE  V4      // SLIDER to test Gas WARNING Value Alarm

#define BLYNK_PIN_GAS_VALUE       V5     // GAUGE
#define BLYNK_PIN_LED_FIRE        V6     // LED widget
#define BLYNK_PIN_LED_SMOKE       V7     // LED widget

#define BLYNK_PIN_GAS_TEST_ENABLE V8     // PUSH Button, to be safe so that gas alarm level are not changed / messed up

//Blynk Color in format #RRGGBB
#define BLYNK_GREEN     "#23C48E"
#define BLYNK_BLUE      "#04C0F8"
#define BLYNK_YELLOW    "#ED9D00"
#define BLYNK_RED_0     "#D3435C"
#define BLYNK_RED       "#FF0000"
#define BLYNK_DARK_BLUE "#5F7CD8"

WidgetLED  BlynkLedFire (BLYNK_PIN_LED_FIRE);
WidgetLED  BlynkLedSmoke(BLYNK_PIN_LED_SMOKE);

#define MQ135_PIN             A0
#define FLAME_SENSOR_PIN      D3
#define BUZZER_PIN            D7

#define GAS_HIGH_ALARM_LEVEL      120
#define GAS_HIGH_WARNING_LEVEL    80
#define GAS_LOW_RESET_LEVEL       60

unsigned int volatile GAS_HIGH_ALARM     = GAS_HIGH_ALARM_LEVEL;
unsigned int volatile GAS_HIGH_WARNING   = GAS_HIGH_WARNING_LEVEL;
unsigned int volatile GAS_LOW_RESET      = GAS_LOW_RESET_LEVEL;

unsigned int volatile GAS_HIGH_ALARM_TEST      = GAS_HIGH_ALARM_LEVEL;
unsigned int volatile GAS_HIGH_WARNING_TEST    = GAS_HIGH_WARNING_LEVEL;

bool volatile testFireAlarm    = false;
bool volatile testSmokeAlarm   = false;

// Just to turn the buzzer OFF temporarily while testing
bool buzzerEnable               = true;
// Just to turn permit temporarily to change GAS_HIGH_ALARM/GAS_HIGH_WARNING while testing
bool gasAlarmLevelChangeEnable  = false;

bool volatile alarmFlagFire    = false;
bool volatile alarmFlagSmoke   = false;

int volatile gasValue;

BLYNK_CONNECTED()
{
  BlynkLedFire.on();
  BlynkLedSmoke.on();
  Blynk.setProperty(BLYNK_PIN_LED_FIRE, "color", BLYNK_GREEN);
  Blynk.setProperty(BLYNK_PIN_LED_SMOKE, "color", BLYNK_GREEN);
  Blynk.setProperty(BLYNK_PIN_GAS_VALUE, "color", BLYNK_GREEN);

  Blynk.virtualWrite(BLYNK_PIN_GAS_HIGH_VALUE, GAS_HIGH_ALARM_LEVEL);
  Blynk.virtualWrite(BLYNK_PIN_GAS_WARN_VALUE, GAS_HIGH_WARNING_LEVEL);

  // Reset everything
  GAS_HIGH_ALARM     = GAS_HIGH_ALARM_LEVEL;
  GAS_HIGH_WARNING   = GAS_HIGH_WARNING_LEVEL;
  GAS_LOW_RESET      = GAS_LOW_RESET_LEVEL;

  //synchronize the state of widgets with hardware states
  Blynk.syncAll();
}

BLYNK_WRITE(BLYNK_PIN_FIRE_TEST)   //Fire Alarm Test, make it a ON/OFF switch, not pushbutton
{
  testFireAlarm = param.asInt();

#if (LOCAL_DEBUG > 1)
  Serial.println("Fire Alarm Test is: " + String(testFireAlarm ? "ON" : "OFF"));
#endif
}

BLYNK_WRITE(BLYNK_PIN_SMOKE_TEST)   //Alarm Test, make it a ON/OFF switch, not pushbutton
{
  testSmokeAlarm = param.asInt();

#if (LOCAL_DEBUG > 1)
  Serial.println("Smoke Alarm Test is: " + String(testSmokeAlarm ? "ON" : "OFF"));
#endif
}

BLYNK_WRITE(BLYNK_PIN_BUZZER_CONTROL)   //BuzzerEnable, make it a pushbutton to be safe
{
  buzzerEnable = param.asInt();

#if (LOCAL_DEBUG > 1)
  Serial.println("BuzzerEnable is: " + String(buzzerEnable ? "ON" : "OFF"));
#endif
}

BLYNK_WRITE(BLYNK_PIN_GAS_HIGH_VALUE)   // SLIDER to test Gas HIGH Value Alarm, only effective when BLYNK_PIN_GAS_TEST_ENABLE is ON
{
  GAS_HIGH_ALARM_TEST = param.asInt();

#if (LOCAL_DEBUG > 1)
  Serial.println("GAS_HIGH_ALARM_TEST Level is: " + String(GAS_HIGH_ALARM));
#endif
}

BLYNK_WRITE(BLYNK_PIN_GAS_WARN_VALUE)   // SLIDER to test Gas WARN Value Alarm, only effective when BLYNK_PIN_GAS_TEST_ENABLE is ON
{
  GAS_HIGH_WARNING_TEST = param.asInt();

#if (LOCAL_DEBUG > 1)
  Serial.println("GAS_HIGH_WARNING_TEST Level is: " + String(GAS_HIGH_WARNING));
#endif
}

BLYNK_WRITE(BLYNK_PIN_GAS_TEST_ENABLE)   // Gas Level Alarm / Warning changing Enable, make it a pushbutton to be safe
{
  gasAlarmLevelChangeEnable = param.asInt();

  if (gasAlarmLevelChangeEnable)
  {
    GAS_HIGH_ALARM     = GAS_HIGH_ALARM_TEST;
    GAS_HIGH_WARNING   = GAS_HIGH_WARNING_TEST;
    GAS_LOW_RESET      = 0;
  }
  else
  {
    GAS_HIGH_ALARM     = GAS_HIGH_ALARM_LEVEL;
    GAS_HIGH_WARNING   = GAS_HIGH_WARNING_LEVEL;
    GAS_LOW_RESET      = GAS_LOW_RESET_LEVEL;
  }

#if (LOCAL_DEBUG > 1)
  Serial.println("gasAlarmLevelChangeEnable is: " + String(gasAlarmLevelChangeEnable ? "ON" : "OFF"));
#endif
}

void notifyOnFire()
{
  static bool flagFireACK = false;

  if (alarmFlagFire  && !flagFireACK)
  {
    flagFireACK = true;
#if (LOCAL_DEBUG > 0)
    Serial.println("Alert: FIRE");
#endif
    Blynk.notify("Alert: FIRE");
    Blynk.setProperty(BLYNK_PIN_LED_FIRE, "color", BLYNK_RED);
  }
  else if (!alarmFlagFire && flagFireACK)
  {
    flagFireACK = false;
    Blynk.setProperty(BLYNK_PIN_LED_FIRE, "color", BLYNK_GREEN);
  }
}

void notifyOnSmoke()
{
  static bool flagSmokeACK = false;

  Blynk.virtualWrite(BLYNK_PIN_GAS_VALUE, gasValue);

  if (gasValue >= GAS_HIGH_ALARM)
    Blynk.setProperty(BLYNK_PIN_GAS_VALUE, "color", BLYNK_RED);
  else if ( (gasValue < GAS_HIGH_ALARM) && (gasValue >= GAS_HIGH_WARNING) )
    Blynk.setProperty(BLYNK_PIN_GAS_VALUE, "color", BLYNK_RED_0);
  else
    Blynk.setProperty(BLYNK_PIN_GAS_VALUE, "color", BLYNK_GREEN);

  if (alarmFlagSmoke && !flagSmokeACK)
  {
    flagSmokeACK = true;

#if (LOCAL_DEBUG > 0)
    Serial.println("Alert: SMOKE = " + String(gasValue));
#endif

    Blynk.notify("Alert: SMOKE");
    Blynk.setProperty(BLYNK_PIN_LED_SMOKE, "color", BLYNK_RED);
  }
  else if (!alarmFlagSmoke && flagSmokeACK)
  {
#if (LOCAL_DEBUG > 0)
    Serial.println("notifyOnSmoke: Color reset");
#endif

    Blynk.setProperty(BLYNK_PIN_LED_SMOKE, "color", BLYNK_GREEN);
    flagSmokeACK = false;
  }
}

void checkingFireAndSmoke()
{
  notifyOnFire();
  notifyOnSmoke();
}

void ICACHE_RAM_ATTR ISR_notifyOnFire()
{
  bool flameActive = !digitalRead(FLAME_SENSOR_PIN);

  if ( (flameActive || testFireAlarm) && !alarmFlagFire )
  {
    alarmFlagFire = true;

#if (LOCAL_DEBUG > 0)
    Serial.println("iAlert: FIRE");
#endif
  }
  else if ( alarmFlagFire && (!flameActive && !testFireAlarm )  )
  {
    alarmFlagFire = false;

#if (LOCAL_DEBUG > 0)
    Serial.println("iAlert: FIRE reset");
#endif
  }
}

// Use true to silumate testing gas level
#define SIMULATE_SMOKE_LEVEL_TEST         true
//#define SIMULATE_SMOKE_LEVEL_TEST         false

#define SIMULATE_SMOKE_LEVEL              30

// Get CO2 PPM,no himidity and temperature corrected, see (https://github.com/smilexth/MQ135)
int ICACHE_RAM_ATTR getPPM(unsigned int _pin)
{
  static int val;
  static float resistance;

  /// The load resistance on the board
#define RLOAD   10.0
  /// Calibration resistance at atmospheric CO2 level
#define RZERO   76.63
  /// Parameters for calculating ppm of CO2 from sensor resistance
#define PARA    116.6020682
#define PARB    2.769034857

  val = analogRead(_pin);
  resistance = (( 1023.0f / (float) val ) * 5.0f - 1.0f) * RLOAD;

  return (int) ( PARA * pow((resistance / RZERO), -PARB) );
}

void ICACHE_RAM_ATTR ISR_notifyOnSmoke()
{
#if (SIMULATE_SMOKE_LEVEL_TEST)
  gasValue = SIMULATE_SMOKE_LEVEL;
#else
  gasValue = getPPM(MQ135_PIN);
#endif

#if (LOCAL_DEBUG > 0)
  if (alarmFlagSmoke)
    Serial.println("iAlert: testSmokeAlarm = "  + String(testSmokeAlarm ? "ON" : "OFF"));
#endif

  if ( ((gasValue > GAS_HIGH_ALARM) || testSmokeAlarm) && !alarmFlagSmoke)
  {
    alarmFlagSmoke = true;

#if (LOCAL_DEBUG > 0)
    Serial.println("iAlert: SMOKE");
#endif
  }
  else if ( alarmFlagSmoke && ( (gasValue < GAS_LOW_RESET) && !testSmokeAlarm ) )
  {
    alarmFlagSmoke = false;

#if (LOCAL_DEBUG > 0)
    Serial.println("iAlert: SMOKE reset : GAS_LOW_RESET = " + String(GAS_LOW_RESET));
#endif
  }
}

void ICACHE_RAM_ATTR ISR_checkingFireAndSmoke()
{
  ISR_notifyOnFire();
  ISR_notifyOnSmoke();
}

// Use relay or direct digital control to drive Alarm Horn or an active buzzer
// analogWrite() / AnalogWriteFreq() somehow interferes with the ISR-based Timer
void ISR_playAlarmSound()
{
  static bool buzzerON = false;
  static bool soundAlarm;

  soundAlarm = buzzerEnable && ( alarmFlagSmoke || alarmFlagFire );

  // Change sound every 0.5s
  if ( soundAlarm && !buzzerON )
  {
#if (LOCAL_DEBUG > 0)
    Serial.println("Buzzer ON");
#endif
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerON = true;
  }
  else if ( soundAlarm && buzzerON )
  {
#if (LOCAL_DEBUG > 0)
    Serial.println("Buzzer OFF");
#endif
    digitalWrite(BUZZER_PIN, LOW);
    buzzerON = false;
  }
  else if (buzzerON)
  {
#if (LOCAL_DEBUG > 0)
    Serial.println("Buzzer OFF");
#endif

    digitalWrite(BUZZER_PIN, LOW);
    buzzerON = false;
  }
}

void heartBeatPrint(void)
{
  static int num = 1;

  Serial.print("B");

  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(" ");
  }
}

void BlynkNotify(void)
{
  heartBeatPrint();

  if (alarmFlagFire)
  {
#if (LOCAL_DEBUG > 0)
    Serial.println("Blynk Notify Alert: FIRE");
#endif

    Blynk.notify("Alert: FIRE");
  }

  if (alarmFlagSmoke)
  {
#if (LOCAL_DEBUG > 0)
    Serial.println("Blynk Notify Alert: SMOKE");
#endif

    Blynk.notify("Alert: SMOKE");
  }
}

// This will use ISR-based hardware timer as we consider it's a critical task
#define CHECK_FIRE_AND_SMOKE_INTERVAL_MS  1000L

// This one is less critical, so we use ISR-based timer to put a flag, then software timer will handle the sound playing
#define PLAY_ALARM_SOUND_INTERVAL_MS      511L

// This is just GUI purpose, and we can just usse software-based timer, such as BlynkTimer
// Anyway, if there is no connection to Blynk, there is no use to to Blynk notification
#define BLYNK_NOTIFY_INTERVAL_MS          60011L


void ICACHE_RAM_ATTR TimerHandler(void)
{
  ISR_Timer.run();
}

void setup()
{
  Serial.begin(115200);
  pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP);
  pinMode (MQ135_PIN, INPUT_PULLUP);
  pinMode (BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Interval in microsecs
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.println("Starting  ITimer OK");
  }
  else
    Serial.println("Can't set ITimer correctly. Select another freq. or interval");

  // This is critical task, to be serviced by ISR-timer
  ISR_Timer.setInterval(CHECK_FIRE_AND_SMOKE_INTERVAL_MS, ISR_checkingFireAndSmoke);
  // This is critical task, to be serviced by ISR-timer
  ISR_Timer.setInterval(PLAY_ALARM_SOUND_INTERVAL_MS, ISR_playAlarmSound);

  // This is non-critical GUI task, to be serviced by software timer
  blynkTimer.setInterval(CHECK_FIRE_AND_SMOKE_INTERVAL_MS, checkingFireAndSmoke);
  // Every Minute to send Blynk notify
  // This is just GUI purpose, and we can just usse software-based timer, such as BlynkTimer
  // Anyway, if there is no connection to Blynk, there is no use to to Blynk notification
  blynkTimer.setInterval(BLYNK_NOTIFY_INTERVAL_MS, BlynkNotify);

#if USE_BLYNK_WM
  Blynk.begin();
#else
  WiFi.begin(ssid, pass);

#if USE_LOCAL_SERVER
  Blynk.config(auth, server, BLYNK_HARDWARE_PORT);
#else
  Blynk.config(auth);
#endif

  Blynk.connect();
  if ( Blynk.connected())
    Serial.println("Connected to Blynk");
#endif
}

void loop()
{
  Blynk.run();
  blynkTimer.run();
}
