#include <WiFiClient.h>
// #include <ESP32WebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "FFat.h"

#include "webserver.h"



// const char ssid_1[]     = "GASTROMATIX";
// const char password_1[] = "22809860032031599074";// const char* ssid = "GASTROMATIX";
const String ssid = "SSID";
const String wifipassword = "Password";







void setup()
{
  Serial.begin(115200);

  Serial.print("Firmware: ");
 // Serial.println(FIRMWARE_VERSION);

  Serial.println("Booting ...");

  Serial.println("Mounting FFat ...");
  initFileSystem();

  Serial.print("FFat Free: "); Serial.println(humanReadableSize((FFat.totalBytes() - FFat.usedBytes())));
  Serial.print("FFat Used: "); Serial.println(humanReadableSize(FFat.usedBytes()));
  Serial.print("FFat Total: "); Serial.println(humanReadableSize(FFat.totalBytes()));

  

  Serial.println("Loading Configuration ...");



  Serial.print("\nConnecting to Wifi: ");
  WiFi.begin(ssid.c_str(), wifipassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // configure web server
  Serial.println("Configuring Webserver ...");
 configureWebServer();

  // startup web server
  Serial.println("Starting Webserver ...");



}

void loop()
{



}

