#include "Arduino.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

struct WebProfileConfig
{
  String id;
  String rezeptPath;
  String productPath;
};

void configureWebServer();
void configureWebServer(const String &profileId);
String humanReadableSize(const size_t bytes);
String processor(const String& var);
String listFiles(bool ishtml,const char *path);
bool initFileSystem();

WebProfileConfig getActiveWebProfileConfig();
bool setActiveWebProfile(const String &profileId);
String getActiveProfileId();

// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
