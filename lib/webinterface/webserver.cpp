#include "webserver.h"
#include "FS.h"
#include "FFat.h"
#include "SD.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// Arduino.h

#define FILESYSTEM SD
#define USE_SD_CARD true

AsyncWebServer server(80);

const String default_httpuser = "Admin";
const String default_httppassword = "1111";

// ───────────────────── Helpers ──────────────────────────────

bool checkUserWebAuth(AsyncWebServerRequest *request)
{
  return request->authenticate(default_httpuser.c_str(), default_httppassword.c_str());
}

String humanReadableSize(size_t bytes)
{
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < 1024 * 1024) return String(bytes / 1024.0) + " KB";
  else return String(bytes / 1024.0 / 1024.0) + " MB";
}

bool initFileSystem()
{
  if (USE_SD_CARD)
  {
    Serial.println("Init SD...");
    if (SD.begin())
    {
      Serial.println("SD OK"); return true;
    }
    Serial.println("SD FAIL");
  } 
  else
  {
    Serial.println("Init FFat...");
    if (FFat.begin())
    {
      Serial.println("FFat OK"); return true;
    }
    Serial.println("FFat FAIL");
  }
  return false;
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "404 Not found");
}

// ───────────────────── Webserver Setup ──────────────────────────────

void configureWebServer()
{

  // --- Login page ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/login.html", "text/html");
  });

  // --- Protected HTML pages served only after login ---
  const char* protectedHTML[] = 
  {
    "/menu.html",
    "/dashboard.html",
    "/profil.html",
    "/system.html",
    "/produkte.html",
    "/files.html",
    "/reboot.html"
  };

  for (String page : protectedHTML)
  {
    server.on(page.c_str(), HTTP_GET, [page](AsyncWebServerRequest *request)
    {
      if (!checkUserWebAuth(request)) return request->requestAuthentication();
      request->send(FILESYSTEM, page, "text/html");
    });
  }

  // --- Protected JSON/API Endpoints ---
  server.on("/user/users.json", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (!checkUserWebAuth(request)) return request->requestAuthentication();
    request->send(FILESYSTEM, "/user/users.json", "application/json");
  });

  server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (!checkUserWebAuth(request)) return request->requestAuthentication();
    // Place your file download/delete handler here
    request->send(200, "text/plain", "File handler stub");
  });

  // --- Serve static web assets (JS, CSS, Images) ---
  server.serveStatic("/web/", FILESYSTEM, "/web/");

  // --- Do NOT serve "/" again here! break auth! ---

  // --- 404 handler ---
  server.onNotFound(notFound);

  server.begin();
  Serial.println("✅ Webserver started");
}
