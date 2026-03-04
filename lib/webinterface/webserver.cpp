#include "webserver.h"
#include "FS.h"
#include "FFat.h"
#include "SD.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

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

static String sanitizeFileBase(String s)
{
  s.replace("/", "_");
  s.replace("\\", "_");
  s.replace("..", "_");
  s.replace(" ", "_");
  s.replace(":", "_");
  s.replace(";", "_");
  s.replace("\"", "_");
  s.replace("'", "_");
  s.replace("|", "_");
  s.replace("?", "_");
  s.replace("*", "_");
  s.replace("<", "_");
  s.replace(">", "_");
  return s;
}

void configureWebServer()
{
  // Ordner sicherstellen
  if (!SD.exists("/rezepte"))
  {
    SD.mkdir("/rezepte");
    Serial.println("Ordner /rezepte angelegt.");
  }

  // user + web picture ordner
  if (!SD.exists("/user")) SD.mkdir("/user");
  if (!SD.exists("/web")) SD.mkdir("/web");
  if (!SD.exists("/web/pictures")) SD.mkdir("/web/pictures");
  if (!SD.exists("/web/pictures/Profile")) SD.mkdir("/web/pictures/Profile");

  // --- Login page ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/login.html", "text/html");
  });

  // --- HTML pages ---
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

  // --- JSON Dateien laden ---
  server.on("/user/users.json", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/user/users.json", "application/json");
  });

  server.on("/user/rols.json", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/user/rols.json", "application/json");
  });

  // JS korrekt ausliefern
  server.on("/user/userHandler.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/user/userHandler.js", "application/javascript");
  });

  // users.json speichern
  // Erwartet x-www-form-urlencoded: body=<json>
  server.on("/user/users.json", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (!request->hasParam("body", true))
    {
      request->send(400, "text/plain", "Kein Body empfangen");
      return;
    }

    String body = request->getParam("body", true)->value();

    if (!FILESYSTEM.exists("/user")) FILESYSTEM.mkdir("/user");
    if (FILESYSTEM.exists("/user/users.json")) FILESYSTEM.remove("/user/users.json");

    File file = FILESYSTEM.open("/user/users.json", FILE_WRITE);
    if (!file)
    {
      request->send(500, "text/plain", "Fehler Datei öffnen");
      return;
    }

    file.print(body);
    file.close();

    request->send(200, "text/plain", "users.json gespeichert");
  });

  // Profilbild Upload (Antwort erst bei final-Chunk)
  // POST /user/uploadProfileImage?base=profil_ADMIN
  // multipart/form-data Feldname: file
  server.on(
    "/user/uploadProfileImage",
    HTTP_POST,
    [](AsyncWebServerRequest *request)
    {
      // Antwort kommt erst im final-Chunk (stabiler)
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
    {
      if (!FILESYSTEM.exists("/web")) FILESYSTEM.mkdir("/web");
      if (!FILESYSTEM.exists("/web/pictures")) FILESYSTEM.mkdir("/web/pictures");
      if (!FILESYSTEM.exists("/web/pictures/Profile")) FILESYSTEM.mkdir("/web/pictures/Profile");

      String base = "profil_unknown";
      if (request->hasParam("base"))
      {
        base = request->getParam("base")->value();
      }
      base = sanitizeFileBase(base);
      if (base.length() == 0) base = "profil_unknown";

      // ext aus filename, fallback jpg
      String ext = ".jpg";
      int dot = filename.lastIndexOf('.');
      if (dot >= 0)
      {
        String e = filename.substring(dot);
        e.toLowerCase();
        if (e == ".jpg" || e == ".jpeg" || e == ".png" || e == ".webp") ext = e;
      }

      String path = "/web/pictures/Profile/" + base + ext;

      if (index == 0)
      {
        if (FILESYSTEM.exists(path)) FILESYSTEM.remove(path);
        request->_tempFile = FILESYSTEM.open(path, FILE_WRITE);
        if (!request->_tempFile)
        {
          request->send(500, "text/plain", "Fehler: Bilddatei nicht schreibbar");
          return;
        }
      }

      if (request->_tempFile)
      {
        request->_tempFile.write(data, len);
      }

      if (final)
      {
        if (request->_tempFile) request->_tempFile.close();
        request->send(200, "text/plain", "OK");
      }
    }
  );

  server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/plain", "File handler stub");
  });

  // --- Static assets ---
  server.serveStatic("/web/", FILESYSTEM, "/web/");
  server.serveStatic("/products/", FILESYSTEM, "/products/");

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

  server.serveStatic("/rezepte/", FILESYSTEM, "/rezepte/");

  // --- Speichern eines Rezeptes ---
  server.on("/saveRecipe", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (!request->hasParam("body", true))
    {
      request->send(400, "text/plain", "Kein Body empfangen");
      return;
    }

    String body = request->getParam("body", true)->value();

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
    name.replace("/", "_");
    name.replace("\\", "_");
    name.replace("..", "_");

    String path = "/rezepte/" + name + ".json";

    if (FILESYSTEM.exists(path))
    {
      FILESYSTEM.remove(path);
    }

    File file = FILESYSTEM.open(path, FILE_WRITE);
    if (!file)
    {
      request->send(500, "text/plain", "Fehler Datei öffnen");
      return;
    }

    file.print(body);
    file.close();

    request->send(200, "text/plain", "Gespeichert: " + path);
  });

  // --- Rezept löschen ---
  server.on("/deleteRecipe", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (request->hasParam("body", true))
    {
      String body = request->getParam("body", true)->value();

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

  server.onNotFound(notFound);
  server.begin();
  Serial.println("Webserver started");
}