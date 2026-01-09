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

// --- Helpers ---

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
  // Ordner sicherstellen
  if (!SD.exists("/rezepte")) {
      SD.mkdir("/rezepte");
      Serial.println("Ordner /rezepte angelegt.");
  }

  // --- Login page ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/login.html", "text/html");
  });

  // --- Protected HTML pages served only after login ---
  const char* htmlPages[] = 
  {
    "/menu.html",
    "/dashboard.html",
    "/profil.html",
    "/system.html",
    "/produkte.html",
    "/files.html",
    "/reboot.html",
    "/login.html",
    "/rezepte.html"
  };

for (String page : htmlPages)
{
  server.on(page.c_str(), HTTP_GET, [page](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, page, "text/html");
  });
}
  // --- Protected JSON/API Endpoints ---
  server.on("/user/users.json", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/user/users.json", "application/json");
  });

  server.on("/user/userHandler.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/user/userHandler.js", "application/json");
  });

  server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    // File download/delete handler
    request->send(200, "text/plain", "File handler stub");
  });

  // --- Serve static web assets (JS, CSS, Images) ---
  server.serveStatic("/web/", FILESYSTEM, "/web/");

 // --- Liste aller Rezepte ---
  server.on("/rezepte/list", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    File root = FILESYSTEM.open("/rezepte");
    if (!root || !root.isDirectory())
    {
      request->send(500, "application/json", "[]");
      return;
    }

    String json = "[";
    File file = root.openNextFile();
    bool first = true;
    while (file)
    {
      String name = file.name();
      if (name.endsWith(".json"))
      {
        if (!first) json += ",";
        json += "\"" + name.substring(name.lastIndexOf('/') + 1) + "\"";
        first = false;
      }
      file = root.openNextFile();
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  // Statische Auslieferung des /rezepte Ordners (wichtig!)
  server.serveStatic("/rezepte/", FILESYSTEM, "/rezepte/");

    // --- Speichern eines Rezeptes (RAW JSON) ---
  server.on("/saveRecipe", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    // JSON aus dem Body holen
    if (request->hasParam("body", true))
    {
      String body = request->getParam("body", true)->value();

      Serial.println("=== Kompletter JSON Body ===");
      Serial.println(body);
      Serial.println("===========================");

      // Name aus JSON holen
      int pos1 = body.indexOf("\"name\"");
      int pos2 = body.indexOf(":", pos1);
      int pos3 = body.indexOf("\"", pos2 + 1);
      int pos4 = body.indexOf("\"", pos3 + 1);

      if (pos1 < 0 || pos2 < 0 || pos3 < 0 || pos4 < 0)
      {
        request->send(400, "text/plain", "JSON ohne name");
        return;
      }

      String name = body.substring(pos3 + 1, pos4);
      name.replace(" ", "_");

      String path = "/rezepte/" + name + ".json";
      File file = FILESYSTEM.open(path, FILE_WRITE);
      if (!file)
      {
        request->send(500, "text/plain", "Fehler Datei öffnen");
        return;
      }

      file.print(body);
      file.close();

      request->send(200, "text/plain", "Gespeichert: " + path);
    }
    else
    {
      request->send(400, "text/plain", "Kein Body empfangen");
    }
  });

  // --- Löschen eines Rezeptes (RAW JSON als Form-Param "body") ---
  server.on("/deleteRecipe", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (request->hasParam("body", true))
    {
      String body = request->getParam("body", true)->value();

      Serial.println("=== Delete JSON Body ===");
      Serial.println(body);
      Serial.println("=======================");

      // Name aus JSON holen
      int pos1 = body.indexOf("\"name\"");
      int pos2 = body.indexOf(":", pos1);
      int pos3 = body.indexOf("\"", pos2 + 1);
      int pos4 = body.indexOf("\"", pos3 + 1);

      if (pos1 < 0 || pos2 < 0 || pos3 < 0 || pos4 < 0)
      {
        request->send(400, "text/plain", "JSON ohne name");
        return;
      }

      String name = body.substring(pos3 + 1, pos4);

      // gleicher Dateiname wie beim Speichern
      name.replace(" ", "_");

      // minimale Pfad-Sicherheit
      name.replace("/", "_");
      name.replace("\\", "_");
      name.replace("..", "_");

      String path = "/rezepte/" + name + ".json";

      if (!FILESYSTEM.exists(path))
      {
        request->send(404, "text/plain", "Datei nicht gefunden: " + path);
        return;
      }

      if (!FILESYSTEM.remove(path))
      {
        request->send(500, "text/plain", "Konnte nicht löschen: " + path);
        return;
      }

      request->send(200, "text/plain", "Gelöscht: " + path);
    }
    else
    {
      request->send(400, "text/plain", "Kein Body empfangen");
    }
  });


  // --- 404 handler ---
  server.onNotFound(notFound);

  server.begin();
  Serial.println("Webserver started");
}
