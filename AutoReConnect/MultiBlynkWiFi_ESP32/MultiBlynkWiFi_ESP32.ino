/****************************************************************************************************************************
 *  MultiBlynkWiFi_ESP32.ino
 *  For ESP8266 boards, testing Multi Blynk and Multi WiFi auto-reconnect features of ESP32 WiFi/WiFiMulti library
 *
 *  Built by Khoi Hoang /https://github.com/khoih-prog/SmallProjects/AutoReConnect
 *  Licensed under MIT license
 *  Version: 1.0.0
 *
 *
 *  Version Modified By   Date      Comments
 *  ------- -----------  ---------- -----------
 *   1.0.0   K Hoang      28/01/2020 Initial coding
 *****************************************************************************************************************************/

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
