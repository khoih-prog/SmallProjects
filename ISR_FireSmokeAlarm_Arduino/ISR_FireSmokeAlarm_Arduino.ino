/****************************************************************************************************************************
 * ISR_FireSmokeAlarm_Arduino.ino
 * ISR-based Fire and Smoke Alarm and Blynk Notification
 * For ESP8266 boards
 * Written by Khoi Hoang
 * Copyright (c) 2019 Khoi Hoang
 * 
 * Built by Khoi Hoang https://github.com/khoih-prog/SmallProjects/ISR_FireSmokeAlarm
 * Licensed under MIT license
 * Version: 1.0.2
 *
 * Now we can use these new 16 ISR-based timers, while consuming only 1 hardware Timer.
 * Their independently-selected, maximum interval is practically unlimited (limited only by unsigned long miliseconds)
 * The accuracy is nearly perfect compared to software timers. The most important feature is they're ISR-based timers
 * Therefore, their executions are not blocked by bad-behaving functions / tasks.
 * This important feature is absolutely necessary for mission-critical tasks.
 *
 * Notes:
 * Special design is necessary to share data between interrupt code and the rest of your program.
 * Variables usually need to be "volatile" types. Volatile tells the compiler to avoid optimizations that assume 
 * variable can not spontaneously change. Because your function may change variables while your program is using them, 
 * the compiler needs this hint. But volatile alone is often not enough.
 * When accessing shared variables, usually interrupts must be disabled. Even with volatile, 
 * if the interrupt changes a multi-byte variable between a sequence of instructions, it can be read incorrectly. 
 * If your data is multiple variables, such as an array and a count, usually interrupts need to be disabled 
 * or the entire sequence of your code which accesses the data.
 *
 * Version Modified By   Date      Comments
 * ------- -----------  ---------- -----------
 *  1.0.0   K Hoang      23/11/2019 Initial coding
 *  1.0.1   K Hoang      25/11/2019 New release fixing compiler error
 *  1.0.2   K.Hoang      29/11/2019 Permit up to 16 super-long-time, super-accurate ISR-based timers to avoid being blocked
 *****************************************************************************************************************************/
 /**************************************************************************************************************************** 
 * FireSmokeAlarm_Arduino demontrates the use of ISR-based Timer to avoid being blocked by other CPU-monopolizing task
 *  
 * In this fairly complex example: CPU is connecting to WiFi, Internet and finally Blynk service (https://docs.blynk.cc/)
 * Many important tasks are fighting for limited CPU resource in this no-controlled single-tasking environment.
 * In certain period, mission-critical tasks (you name it) could be deprived of CPU time and have no chance
 * to be executed. This can lead to disastrous results at critical time. For example for this Fire and Smoke Alarm.
 * We hereby will use ISR-base Timer to poll and detect if there is Fire or Smoke (MQ135), then flag and process correspondngly. 
 * Alarm sound, considered similarly critical, is controlled by another ISR-based timer.. 
 * We'll see this ISR-based operation will always have highest priority, preempt all remaining tasks to assure its
 * functionality.
 *****************************************************************************************************************************/
/****************************************************************************************************************************
 * This example will demonstrate the inherently un-blockable feature of ISR-based timers compared to software timers..
 * Being ISR-based timers, their executions are not blocked by bad-behaving functions / tasks, such as connecting to WiFi, Internet
 * and Blynk services. You can also have many (up to 16) timers to use.
 * This non-being-blocked important feature is absolutely necessary for mission-critical tasks. 
 * You'll see blynkTimer is blocked while connecting to WiFi / Internet / Blynk, or some other possible bad behaving tasks..
 *****************************************************************************************************************************/
/****************************************************************************************************************************
 *  This example is currently written for Arduino Mega 2560 with ESP-01 WiFi or Mega2560-WiFi-R3
 *  You can easily convert to UNO and ESP-01
 *  Mega: Digital pin 18 – 21,2 and 3 can be used to provide hardware interrupt from external devices.
 *  UNO/Nano: Digital pin 2 and 3 can be used to provide hardware interrupt from external devices.
 *  To upload program to MEGA2560+WiFi, only turn ON SW 3+4 (USB <-> MCU).
 *  To run MEGA+WiFi combined, turn ON SW 1+2 (MCU <-> ESP) and SW 3+4 (USB <-> MCU) 
 *****************************************************************************************************************************/
 
//These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
#define TIMER_INTERRUPT_DEBUG       0

#define LOCAL_DEBUG                 1

#define BLYNK_PRINT Serial

#ifdef BLYNK_DEBUG
  #undef BLYNK_DEBUG
#endif

#define USE_TIMER_1     true
#define USE_TIMER_2     false
#define USE_TIMER_3     false
#define USE_TIMER_4     false
#define USE_TIMER_5     false

#include "TimerInterrupt.h"
#include "ISR_Timer.h"

#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>

#define USE_LOCAL_SERVER    true

// If local server
#if USE_LOCAL_SERVER
  char blynk_server[]   = "yourname.duckdns.org";
  //char blynk_server[]   = "192.168.2 110";
#define BLYNK_HARDWARE_PORT     8080
#else
  char blynk_server[]   = "";
#endif

char auth[]     = "***";     // Arduino FireSmokeAlarm
char ssid[]     = "***";
char pass[]     = "***";

//Mega2560
// Hardware Serial on Mega, Leonardo, Micro...
#define EspSerial Serial3   //Serial1

// Your MEGA <-> ESP8266 baud rate:
#define ESP8266_BAUD 115200

ESP8266 wifi(&EspSerial);

// Init ISR-based timer to provide 16 ISR-based timer
ISR_Timer ISR_Timer1;
#define HW_TIMER_INTERVAL_MS        50

// Init BlynkTimer
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

#define MQ135_PIN             A0          // Analog input
#define FLAME_SENSOR_PIN      3           // Pin 3 can be used as Interrupt input pin later
#define BUZZER_PIN            4           // Digital Output

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
    Serial.println("Fire Alarm Test is: " + String(testFireAlarm? "ON" : "OFF"));
  #endif
}

BLYNK_WRITE(BLYNK_PIN_SMOKE_TEST)   //Alarm Test, make it a ON/OFF switch, not pushbutton
{
  testSmokeAlarm = param.asInt();

  #if (LOCAL_DEBUG > 1)
    Serial.println("Smoke Alarm Test is: " + String(testSmokeAlarm? "ON" : "OFF"));
  #endif
}

BLYNK_WRITE(BLYNK_PIN_BUZZER_CONTROL)   //BuzzerEnable, make it a pushbutton to be safe
{
  buzzerEnable = param.asInt();

  #if (LOCAL_DEBUG > 1)
    Serial.println("BuzzerEnable is: " + String(buzzerEnable? "ON" : "OFF"));
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
    Serial.println("gasAlarmLevelChangeEnable is: " + String(gasAlarmLevelChangeEnable? "ON" : "OFF"));
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

void ISR_notifyOnFire()
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
int getPPM(unsigned int _pin)
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
  resistance = (( 1023.0f/(float) val ) * 5.0f - 1.0f) * RLOAD;
  
  return (int) ( PARA * pow((resistance/RZERO), -PARB) );
}

void ISR_notifyOnSmoke()
{
  #if (SIMULATE_SMOKE_LEVEL_TEST)
    gasValue = SIMULATE_SMOKE_LEVEL;
  #else
    gasValue = getPPM(MQ135_PIN);
  #endif

  #if (LOCAL_DEBUG > 0)
    if (alarmFlagSmoke)
      Serial.println("iAlert: testSmokeAlarm = "  + String(testSmokeAlarm? "ON" : "OFF"));
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

void ISR_checkingFireAndSmoke()
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


void TimerHandler(void)
{
  ISR_Timer1.run();
}

void setup()
{
  Serial.begin(115200);

  // Set ESP8266 baud rate
  EspSerial.begin(ESP8266_BAUD);
  delay(10);

  Serial.println("\nStarting");
  Serial.print("ESPSerial using ");
  Serial.println(ESP8266_BAUD);
  
  pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP);
  pinMode (MQ135_PIN, INPUT_PULLUP);
  pinMode (BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("\nStarting Timer Interrupt");

  // Select Timer 1-2 for UNO, 0-5 for MEGA
  // Timer 2 is 8-bit timer, only for higher frequency   
  ITimer1.init();

  // Using ATmega328 used in UNO => 16MHz CPU clock , 
  // For 16-bit timer 1, 3, 4 and 5, set frequency from 0.2385 to some KHz
  // For 8-bit timer 2 (prescaler up to 1024, set frequency from 61.5Hz to some KHz
  
  // Interval in millisecs
  if (ITimer1.attachInterruptInterval(HW_TIMER_INTERVAL_MS, TimerHandler))
  {
    Serial.println("Starting  ITimer1 OK");
  }
  else
    Serial.println("Can't set ITimer1 correctly. Select another freq. or interval");
  
  // This is critical task, to be serviced by ISR-timer
  ISR_Timer1.setInterval(CHECK_FIRE_AND_SMOKE_INTERVAL_MS, ISR_checkingFireAndSmoke);  
  // This is critical task, to be serviced by ISR-timer
  ISR_Timer1.setInterval(PLAY_ALARM_SOUND_INTERVAL_MS, ISR_playAlarmSound);

  // This is non-critical GUI task, to be serviced by software timer
  blynkTimer.setInterval(CHECK_FIRE_AND_SMOKE_INTERVAL_MS, checkingFireAndSmoke);  
  // Every Minute to send Blynk notify
  // This is just GUI purpose, and we can just usse software-based timer, such as BlynkTimer
  // Anyway, if there is no connection to Blynk, there is no use to to Blynk notification  
  blynkTimer.setInterval(BLYNK_NOTIFY_INTERVAL_MS, BlynkNotify);

  #if USE_LOCAL_SERVER
    Blynk.begin(auth, wifi, ssid, pass, blynk_server, BLYNK_HARDWARE_PORT); 
  #else
    Blynk.begin(auth, wifi, ssid, pass); 
  #endif
  
  if (Blynk.connected())
    Serial.println("Blynk connected");
  else
    Serial.println("Blynk not connected yet");  
}

void loop()
{
  Blynk.run();
  blynkTimer.run();
}
