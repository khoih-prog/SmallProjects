/****************************************************************************************************************************
 *  BlynkWiFi_ESP8266.ino
 *  For ESP8266 boards, testing Multi Blynk and Multi WiFi auto-reconnect features of ESP8266WiFi library
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

#ifndef ESP8266
#error This code is intended to run on the ESP8266 platform! Please check your Tools->Board setting.
#endif

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char blynk_server[] = "account.duckdns.org";
char blynk_token [] = "your-token" ;

#define BLYNK_SERVER_HARDWARE_PORT    8080
#define BLYNK_CONNECT_TIMEOUT_MS      15000L

char wifi_ssid[] = "your-ssid";
char wifi_passphrase[] = "your-password";

// For ESP8266, this better be 3000 to enable connect the 1st time
#define WIFI_MULTI_CONNECT_WAITING_MS      3000L

uint8_t status;

uint8_t connectMultiWiFi(void)
{
  Serial.println("\nConnecting Wifi...");
  WiFi.begin(wifi_ssid, wifi_passphrase);
  delay(WIFI_MULTI_CONNECT_WAITING_MS);

  return true;
}

bool connectMultiBlynk(void)
{
  Blynk.config(blynk_token, blynk_server, BLYNK_SERVER_HARDWARE_PORT);

  if ( Blynk.connect(BLYNK_CONNECT_TIMEOUT_MS) )
  {
    Serial.println("Blynk connected");
    return true;
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

  Serial.print("\nStart");

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
