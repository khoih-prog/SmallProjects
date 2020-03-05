/*
   FireSmokeAlarm.ino
   Fire and Smoke Alarm and Blynk Notification
   For ESP8266 boards
   Written by Khoi Hoang
   Copyright (c) 2019 Khoi Hoang

   Built by Khoi Hoang https://github.com/khoih-prog/SmallProjects/FireSmokeAlarm
   Licensed under MIT license
   Version: 1.0.0

   Version Modified By   Date      Comments
   ------- -----------  ---------- -----------
    1.0.0   K Hoang     24/11/2019 Initial coding
*/

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>

#define USE_BLYNK_WM   true
//#define USE_BLYNK_WM   false

#if USE_BLYNK_WM
#include <BlynkSimpleEsp8266_WM.h>        //https://github.com/khoih-prog/Blynk_WM
#else
#include <BlynkSimpleEsp8266.h>
#endif

#include <Ticker.h>

BlynkTimer timer;

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

#if !USE_BLYNK_WM
#define USE_LOCAL_SERVER    true

// If local server
#if USE_LOCAL_SERVER
char server[]   = "192.168.2 110";
#define BLYNK_HARDWARE_PORT     8080
#else
char server[]   = "";
#endif

char auth[]     = "***";
char ssid[]     = "***";
char pass[]     = "***";
#endif

#define MQ135_PIN             A0
#define FLAME_SENSOR_PIN      D3
#define BUZZER_PIN            D7

#define GAS_HIGH_ALARM_LEVEL      120
#define GAS_HIGH_WARNING_LEVEL    80
#define GAS_LOW_RESET_LEVEL       60

unsigned int GAS_HIGH_ALARM     = GAS_HIGH_ALARM_LEVEL;
unsigned int GAS_HIGH_WARNING   = GAS_HIGH_WARNING_LEVEL;
unsigned int GAS_LOW_RESET      = GAS_LOW_RESET_LEVEL;

unsigned int GAS_HIGH_ALARM_TEST      = GAS_HIGH_ALARM_LEVEL;
unsigned int GAS_HIGH_WARNING_TEST    = GAS_HIGH_WARNING_LEVEL;

bool testFireAlarm    = false;
bool testSmokeAlarm   = false;

// Just to turn the buzzer OFF temporarily while testing
bool buzzerEnable               = true;
// Just to turn permit temporarily to change GAS_HIGH_ALARM/GAS_HIGH_WARNING while testing
bool gasAlarmLevelChangeEnable  = false;

bool alarmFlagFire    = false;
bool alarmFlagSmoke   = false;

int gasValue;

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

  Serial.println("Fire Alarm Test is: " + String(testFireAlarm ? "ON" : "OFF"));
}

BLYNK_WRITE(BLYNK_PIN_SMOKE_TEST)   //Alarm Test, make it a ON/OFF switch, not pushbutton
{
  testSmokeAlarm = param.asInt();

  Serial.println("Smoke Alarm Test is: " + String(testSmokeAlarm ? "ON" : "OFF"));
}

BLYNK_WRITE(BLYNK_PIN_BUZZER_CONTROL)   //BuzzerEnable, make it a pushbutton to be safe
{
  buzzerEnable = param.asInt();

  Serial.println("BuzzerEnable is: " + String(buzzerEnable ? "ON" : "OFF"));
}

BLYNK_WRITE(BLYNK_PIN_GAS_HIGH_VALUE)   // SLIDER to test Gas HIGH Value Alarm, only effective when BLYNK_PIN_GAS_TEST_ENABLE is ON
{
  GAS_HIGH_ALARM_TEST = param.asInt();

  Serial.println("GAS_HIGH_ALARM_TEST Level is: " + String(GAS_HIGH_ALARM));
}

BLYNK_WRITE(BLYNK_PIN_GAS_WARN_VALUE)   // SLIDER to test Gas WARN Value Alarm, only effective when BLYNK_PIN_GAS_TEST_ENABLE is ON
{
  GAS_HIGH_WARNING_TEST = param.asInt();

  Serial.println("GAS_HIGH_WARNING_TEST Level is: " + String(GAS_HIGH_WARNING));
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
  Serial.println("gasAlarmLevelChangeEnable is: " + String(gasAlarmLevelChangeEnable ? "ON" : "OFF"));
}

void notifyOnFire()
{
  bool flameActive = !digitalRead(FLAME_SENSOR_PIN);

  if ( (flameActive || testFireAlarm) && !alarmFlagFire )
  {
    Serial.println("Alert: FIRE");
    Blynk.notify("Alert: FIRE");
    Blynk.setProperty(BLYNK_PIN_LED_FIRE, "color", BLYNK_RED);
    alarmFlagFire = true;
  }
  else if ( alarmFlagFire && (!flameActive && !testFireAlarm )  )
  {
    Blynk.setProperty(BLYNK_PIN_LED_FIRE, "color", BLYNK_GREEN);
    alarmFlagFire = false;
  }
}

void notifyOnSmoke()
{
  gasValue = analogRead(MQ135_PIN);

  Blynk.virtualWrite(BLYNK_PIN_GAS_VALUE, gasValue);

  if (gasValue >= GAS_HIGH_ALARM)
    Blynk.setProperty(BLYNK_PIN_GAS_VALUE, "color", BLYNK_RED);
  else if ( (gasValue < GAS_HIGH_ALARM) && (gasValue >= GAS_HIGH_WARNING) )
    Blynk.setProperty(BLYNK_PIN_GAS_VALUE, "color", BLYNK_RED_0);
  else
    Blynk.setProperty(BLYNK_PIN_GAS_VALUE, "color", BLYNK_GREEN);

  if ( ((gasValue > GAS_HIGH_ALARM) || testSmokeAlarm) && !alarmFlagSmoke)
  {
    Serial.println("Alert: SMOKE");
    Blynk.notify("Alert: SMOKE");
    Blynk.setProperty(BLYNK_PIN_LED_SMOKE, "color", BLYNK_RED);

    alarmFlagSmoke = true;
  }
  else if ( alarmFlagSmoke && ( (gasValue < GAS_LOW_RESET) && !testSmokeAlarm ) )
  {
    Blynk.setProperty(BLYNK_PIN_LED_SMOKE, "color", BLYNK_GREEN);
    alarmFlagSmoke = false;
  }
}

void checkingFireAndSmoke()
{
  notifyOnFire();
  notifyOnSmoke();
}

void playNote(unsigned int frequency)
{
  if (frequency > 0)
  {
    analogWrite(BUZZER_PIN, 512);
    analogWriteFreq(frequency);
  }
  else
  {
    analogWrite(BUZZER_PIN, 0);
  }
}

#define MS_IN_HALFSEC         500
#define ALARM_FREQ_HI         1000
#define ALARM_FREQ_LO         400

void playAlarmSound()
{
  static Ticker alarmTicker;
  static ulong prev_millis = 0;
  static ulong curr_millis;

  curr_millis = millis();

  // Change sound every 0.5s
  if ( buzzerEnable && ( alarmFlagSmoke || alarmFlagFire ) && (curr_millis - prev_millis >= 2 * MS_IN_HALFSEC) )
  {
    prev_millis = curr_millis;
    playNote(ALARM_FREQ_HI);

    alarmTicker.once_ms(MS_IN_HALFSEC, playNote, (unsigned int) ALARM_FREQ_LO);
  }
  else
  {
    playNote(0);
  }
}

void BlynkNotify(void)
{
  Serial.println("B");

  if (alarmFlagFire)
  {
    Serial.println("Alert: FIRE");
    Blynk.notify("Alert: FIRE");
  }

  if (alarmFlagSmoke)
  {
    Serial.println("Alert: SMOKE");
    Blynk.notify("Alert: SMOKE");
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP);
  pinMode (MQ135_PIN, INPUT_PULLUP);
  pinMode (BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

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

  timer.setInterval(1111L, playAlarmSound);
  timer.setInterval(2500L, checkingFireAndSmoke);
  // Every Minute to send Blynk notify
  timer.setInterval(60000L, BlynkNotify);
}

void loop()
{
  Blynk.run();
  timer.run();
}
