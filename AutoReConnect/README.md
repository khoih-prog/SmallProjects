## AutoReconnect to demonstrate the usage of WiFi and Blynk AutoReConnect feature for ESP8266, ESP32

### Why do we need this `AutoReconnect`?

Imagine you have a system with `mission-semi-critical` functions, measuring water level and control the sump pump or doing something much more important using extensive GUI, such as medical equipments, security and/or fire-smoke alarm, etc. You normally use a `software timer to poll`, or even place the function in loop(). But what if another function is blocking the `loop()` or `setup()`.

So your function might not be executed, and the result would be disastrous.

You'd prefer to have your function called, no matter what happening with other functions (busy loop, bug, etc.).

The best choice is to use a `Hardware Timer with Interrupt` in cooperation with an `Input Pin Interrupt` to call your function.

The catch is your function is now part of an `ISR (Interrupt Service Routine)`, and must be `lean / mean`, and follow certain rules. More to read on:

[Attach Interrupt](https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/)

***What if your to-be-called function is a little bit `fat and lazy`, with some delay(), waiting loops, etc. making it's incompatible with `hardware interrupt` ?***

The second best choice is solutions which :

1. Use non-blocking functions in `loop()`
2. Permit `non-blocking AutoReConnect` feature to
  - auto(re)connect to the `best WiFi AP available` in the AP list (according to quality: highest RSSI/reliability APs first)
  - auto(re)connect to the `best Blynk server available` in the Blynk-Server list (according to priority: local Blynk servers first, then Cloud-based servers next)

## Design principles of AutoReConnect

The design principles are as follows:

1. Normal functions in the `loop()`, as well as those called by `timers` which are considered `mission-semi-critical`, and must not be interfered or blocked by any other bad tasks, `intentionally or unintentionally`. This principle can be applied to any of your projects.
2. WiFi is considered just a communications function. Being connected or not must not interfere with the `mission-semi-critical` tasks.
3. Enable multiple WiFi APs in a list, so that the program can search and use the best and still available AP in case the currently used AP is out-of-service.
4. Blynk is considered just a `Graphical-User-Interface (GUI)`. Being connected or not must not interfere with the `mission-semi-critical` tasks.
5. Enable multiple Blynk Servers (local and cloud-based servers unlimited) in a list, so that the program can search and use the best and still available Blynk Server in case the currently used server is out-of-service.

Certainly, with Blynk GUI, we can achieve many more great features, such as `remote check and control, configurable test case and value` , etc. when possible and available.

This can be applied in many projects requiring reliable system control, where `good, bad, or no connection has no effect on the operation of the system`.

### Examples

```cpp

#ifndef ESP32
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiMulti.h>
#include <BlynkSimpleEsp32.h>

#define BLYNK_CONNECT_TIMEOUT_MS      5000L

#define BLYNK_SERVER_MAX_LEN      32
#define BLYNK_TOKEN_MAX_LEN       32

typedef struct
{
  char blynk_server[BLYNK_SERVER_MAX_LEN + 1];
  char blynk_token [BLYNK_TOKEN_MAX_LEN + 1];
}  Blynk_Credentials;

Blynk_Credentials Blynk_Creds[] =
{
  { "acct1.duckdns.org",    "blynk-token1"},
  { "acct2.duckdns.org",    "blynk-token2"},
  { "192.168.2.110",        "blynk-token3"},
  { "192.168.2.112",        "blynk-token4"},
  { "blynk-cloud.com",      "blynk-token5"},
  { "blynk-cloud.com",      "blynk-token6"}
};

#define NUMBER_BLYNK_SERVERS    ( sizeof(Blynk_Creds) / sizeof(Blynk_Credentials) )

#define BLYNK_SERVER_HARDWARE_PORT    8080

#define SSID_MAX_LEN      32
#define PASS_MAX_LEN      64

typedef struct
{
  char ssid[SSID_MAX_LEN + 1];
  char pass[PASS_MAX_LEN + 1];
}  WiFi_Credentials;

WiFi_Credentials WiFi_Creds[] =
{
  { "ssid1", "pass1"},
  { "ssid2", "pass2"},
  { "ssid3", "pass3"},
  { "ssid4", "pass4"},
};

#define NUMBER_SSIDS    ( sizeof(WiFi_Creds) / sizeof(WiFi_Credentials) )

WiFiMulti wifiMulti;

// For ESP32, this better be 2000 to enable connect the 1st time
#define WIFI_MULTI_CONNECT_WAITING_MS      2000L

uint8_t status;

uint8_t connectMultiWiFi(void)
{
  Serial.println("\nConnecting Wifi...");

  int i = 0;
  status = wifiMulti.run();
  delay(WIFI_MULTI_CONNECT_WAITING_MS);

  while ( ( i++ < 10 ) && ( status != WL_CONNECTED ) )
  {
    status = wifiMulti.run();

    if ( status == WL_CONNECTED )
      break;
    else
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }

  if ( status == WL_CONNECTED )
  {
    Serial.println("WiFi connected to after " + String(i) + " times.");
    Serial.println("SSID = " + WiFi.SSID());
    Serial.println("RSSI = " + String(WiFi.RSSI()));
    Serial.println("Channel: " + String(WiFi.channel()));
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
    Serial.println("WiFi not connected");

  return status;
}

bool connectMultiBlynk(void)
{
  for (int i = 0; i < NUMBER_BLYNK_SERVERS; i++)
  {
    Blynk.config(Blynk_Creds[i].blynk_token, Blynk_Creds[i].blynk_server, BLYNK_SERVER_HARDWARE_PORT);

    if ( Blynk.connect(BLYNK_CONNECT_TIMEOUT_MS) )
    {
      Serial.println("Blynk connected to #" + String(i));
      Serial.println("Blynk Server = " + String(Blynk_Creds[i].blynk_server));
      Serial.println("Blynk Token  = " + String(Blynk_Creds[i].blynk_token));
      return true;
    }
  }

  Serial.println("Blynk not connected");
  return false;

}

void heartBeatPrint(void)
{
  static int num = 1;

  if (Blynk.connected())
  {
    Serial.print("B");
  }
  else
  {
    Serial.print("F");
  }
  
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

void check_status()
{
  static unsigned long checkstatus_timeout = 0;

#define STATUS_CHECK_INTERVAL     15000L

  // Send status report every STATUS_REPORT_INTERVAL (60) seconds: we don't need to send updates frequently if there is no status change.
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    // report status to Blynk
    heartBeatPrint();

    checkstatus_timeout = millis() + STATUS_CHECK_INTERVAL;
  }
}

void setup()
{
  Serial.begin(115200);

  Serial.println("\nStart MultiBlynkWiFi_ESP32");

  for (int i = 0; i < NUMBER_SSIDS; i++)
  {
    wifiMulti.addAP(WiFi_Creds[i].ssid, WiFi_Creds[i].pass);
  }

  if ( connectMultiWiFi() )
  {
    connectMultiBlynk();
  }
}

void loop()
{
  if ( ( status = WiFi.status() ) != WL_CONNECTED )
  {
    if ( connectMultiWiFi() )
    {
      connectMultiBlynk();
    }
  }
  else if ( !Blynk.connected() )
  {
    connectMultiBlynk();
  }

  if ( Blynk.connected() )
    Blynk.run();
    
  check_status();
}

```

Please take a look at examples, as well.

Also see examples: 
1. [BlynkWiFi_ESP8266](./BlynkWiFi_ESP8266)
2. [MultiWiFi_ESP32](./MultiWiFi_ESP32)
3. [MultiWiFi_ESP8266](./MultiWiFi_ESP8266)
4. [MultiBlynkWiFi_ESP32](./MultiBlynkWiFi_ESP32)
5. [MultiBlynkWiFi_ESP8266](./MultiBlynkWiFi_ESP8266)



### Contributions and thanks

1. Thanks to [Madhukesh](https://community.blynk.cc/u/Madhukesh) for repetitively giving a lots of unusual requests and giving reasons to start this library.

## Contributing
If you want to contribute to this project:
- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library

## Copyright
Copyright 2020- Khoi Hoang
