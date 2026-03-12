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
static const char *WEB_PROFILE_CONFIG_PATH = "/system/webbrowser_config.json";

static WebProfileConfig g_profiles[] = {
  {"profil1", "/rezepte",  "/products"},
  {"profil2", "/rezepte2", "/products2"},
  {"profil3", "/rezepte3", "/products3"}
};
static const size_t g_profileCount = sizeof(g_profiles) / sizeof(g_profiles[0]);
static WebProfileConfig g_activeProfile = g_profiles[0];

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

static String extractJsonStringValue(const String &json, const String &key)
{
  String pattern = "\"" + key + "\"";
  int keyPos = json.indexOf(pattern);
  if (keyPos < 0) return "";

  int colonPos = json.indexOf(':', keyPos + pattern.length());
  if (colonPos < 0) return "";

  int startQuote = json.indexOf('"', colonPos + 1);
  if (startQuote < 0) return "";

  int endQuote = json.indexOf('"', startQuote + 1);
  if (endQuote < 0) return "";

  return json.substring(startQuote + 1, endQuote);
}

static WebProfileConfig* findProfileById(const String &profileId)
{
  for (size_t i = 0; i < g_profileCount; ++i)
  {
    if (g_profiles[i].id.equalsIgnoreCase(profileId))
    {
      return &g_profiles[i];
    }
  }
  return nullptr;
}

static void ensureDir(const String &path)
{
  if (path.length() == 0) return;
  if (!FILESYSTEM.exists(path))
  {
    FILESYSTEM.mkdir(path);
    Serial.println("Ordner angelegt: " + path);
  }
}

static void ensureCommonFolders()
{
  ensureDir("/system");
  ensureDir("/user");
  ensureDir("/web");
  ensureDir("/web/pictures");
  ensureDir("/web/pictures/Profile");

  for (size_t i = 0; i < g_profileCount; ++i)
  {
    ensureDir(g_profiles[i].rezeptPath);
    ensureDir(g_profiles[i].productPath);
  }
}

static String activeProfileJson()
{
  String json = "{";
  json += "\"activeProfile\":\"" + g_activeProfile.id + "\",";
  json += "\"recipePath\":\"" + g_activeProfile.rezeptPath + "\",";
  json += "\"productPath\":\"" + g_activeProfile.productPath + "\",";
  json += "\"profiles\":[";
  for (size_t i = 0; i < g_profileCount; ++i)
  {
    if (i > 0) json += ",";
    json += "{";
    json += "\"id\":\"" + g_profiles[i].id + "\",";
    json += "\"recipePath\":\"" + g_profiles[i].rezeptPath + "\",";
    json += "\"productPath\":\"" + g_profiles[i].productPath + "\"";
    json += "}";
  }
  json += "]}";
  return json;
}

static void saveActiveProfileToFile()
{
  ensureDir("/system");
  if (FILESYSTEM.exists(WEB_PROFILE_CONFIG_PATH))
  {
    FILESYSTEM.remove(WEB_PROFILE_CONFIG_PATH);
  }

  File file = FILESYSTEM.open(WEB_PROFILE_CONFIG_PATH, FILE_WRITE);
  if (!file)
  {
    Serial.println("Konnte Webbrowser-Konfiguration nicht speichern");
    return;
  }

  file.print(activeProfileJson());
  file.close();
  Serial.println("Webbrowser-Konfiguration gespeichert: " + g_activeProfile.id);
}

static void loadActiveProfileFromFile()
{
  ensureCommonFolders();

  if (!FILESYSTEM.exists(WEB_PROFILE_CONFIG_PATH))
  {
    g_activeProfile = g_profiles[0];
    saveActiveProfileToFile();
    return;
  }

  File file = FILESYSTEM.open(WEB_PROFILE_CONFIG_PATH, FILE_READ);
  if (!file)
  {
    g_activeProfile = g_profiles[0];
    saveActiveProfileToFile();
    return;
  }

  String json = file.readString();
  file.close();

  String profileId = extractJsonStringValue(json, "activeProfile");
  WebProfileConfig *cfg = findProfileById(profileId);
  if (cfg)
  {
    g_activeProfile = *cfg;
  }
  else
  {
    g_activeProfile = g_profiles[0];
    saveActiveProfileToFile();
  }
}

WebProfileConfig getActiveWebProfileConfig()
{
  return g_activeProfile;
}

String getActiveProfileId()
{
  return g_activeProfile.id;
}

bool setActiveWebProfile(const String &profileId)
{
  WebProfileConfig *cfg = findProfileById(profileId);
  if (!cfg) return false;

  g_activeProfile = *cfg;
  ensureDir(g_activeProfile.rezeptPath);
  ensureDir(g_activeProfile.productPath);
  saveActiveProfileToFile();
  Serial.println("Aktives Webprofil: " + g_activeProfile.id +
                 " | Rezepte=" + g_activeProfile.rezeptPath +
                 " | Products=" + g_activeProfile.productPath);
  return true;
}

// ───────────────────── Webserver Setup ──────────────────────────────

void configureWebServer(const String &profileId)
{
  loadActiveProfileFromFile();
  if (profileId.length() > 0)
  {
    setActiveWebProfile(profileId);
  }

  ensureCommonFolders();

  Serial.println("Webserver mit Profil gestartet: " + g_activeProfile.id);
  Serial.println("Aktiver Rezept-Pfad: " + g_activeProfile.rezeptPath);
  Serial.println("Aktiver Product-Pfad: " + g_activeProfile.productPath);

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

  server.on("/api/webbrowser/config", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "application/json", activeProfileJson());
  });

  server.on("/api/webbrowser/config", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    String requestedProfile;

    if (request->hasParam("profile", true))
    {
      requestedProfile = request->getParam("profile", true)->value();
    }
    else if (request->hasParam("body", true))
    {
      requestedProfile = extractJsonStringValue(request->getParam("body", true)->value(), "profile");
    }

    requestedProfile.trim();
    if (requestedProfile.length() == 0)
    {
      request->send(400, "application/json", "{\"ok\":false,\"message\":\"profile fehlt\"}");
      return;
    }

    if (!setActiveWebProfile(requestedProfile))
    {
      request->send(404, "application/json", "{\"ok\":false,\"message\":\"Profil nicht gefunden\"}");
      return;
    }

    request->send(200, "application/json", activeProfileJson());
  });

  // JS korrekt ausliefern
  server.on("/user/userHandler.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(FILESYSTEM, "/user/userHandler.js", "application/javascript");
  });

  // users.json speichern
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

  server.on(
    "/user/uploadProfileImage",
    HTTP_POST,
    [](AsyncWebServerRequest *request)
    {
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
  server.on("/products/products.json", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String path = g_activeProfile.productPath + "/products.json";
    if (!FILESYSTEM.exists(path))
    {
      request->send(404, "application/json", "{\"zutaten\":[]}");
      return;
    }
    request->send(FILESYSTEM, path, "application/json");
  });

  // --- Liste aller Rezepte ---
  server.on("/rezepte/list", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    File root = FILESYSTEM.open(g_activeProfile.rezeptPath);
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

  server.on("/rezepte/*", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String url = request->url();
    String tail = url.substring(String("/rezepte/").length());
    if (tail.length() == 0)
    {
      request->send(404, "text/plain", "Datei fehlt");
      return;
    }

    String path = g_activeProfile.rezeptPath + "/" + tail;
    if (!FILESYSTEM.exists(path))
    {
      request->send(404, "text/plain", "Datei nicht gefunden");
      return;
    }
    request->send(FILESYSTEM, path, "application/json");
  });

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

    String path = g_activeProfile.rezeptPath + "/" + name + ".json";

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

      String path = g_activeProfile.rezeptPath + "/" + name + ".json";

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

void configureWebServer()
{
  configureWebServer("");
}
