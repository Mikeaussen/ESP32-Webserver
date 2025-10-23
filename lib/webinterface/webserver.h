#include "Arduino.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
void configureWebServer();
String humanReadableSize(const size_t bytes);
String processor(const String& var) ;
String listFiles(bool ishtml,const char *path);
bool initFileSystem();
// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);