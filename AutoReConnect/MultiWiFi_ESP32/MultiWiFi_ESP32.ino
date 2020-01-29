/****************************************************************************************************************************
 *  MultiWiFi_ESP32.ino
 *  For ESP32 boards, testing Multi WiFi auto-reconnect features of ESP32 WiFi library
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

uint8_t status;

uint8_t connectMultiWiFi(void)
{
  Serial.println("\nConnecting Wifi...");

  int i = 0;
  status = wifiMulti.run();
  delay(2000);

  while ( ( i++ < 10 ) && ( status != WL_CONNECTED ) )
  {
    status = wifiMulti.run();

    if ( status == WL_CONNECTED )
      break;
    else
      delay(2000);
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

void setup()
{
  Serial.begin(115200);

  for (int i = 0; i < NUMBER_SSIDS; i++)
  {
    wifiMulti.addAP(WiFi_Creds[i].ssid, WiFi_Creds[i].pass);
  }

  connectMultiWiFi();
}

void loop()
{
  if ( ( status = WiFi.status() ) != WL_CONNECTED )
  {
    connectMultiWiFi();
  }
}
