#include <WiFiClient.h>
// #include <ESP32WebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "FFat.h"

#include "webserver.h"

#define DEST_FS_USES_SD
#include "ESP32-targz.h"
#define FILESYSTEM SD

const String ssid    = "GASTROMATIX";
const String wifipassword = "22809860032031599074";// const char* ssid = "GASTROMATIX";
// const String ssid = "SSID";
// const String wifipassword = "Password";

#pragma GCC diagnostic ignored "-Wformat"

char tmp_path[255] = {0};
const char* src_path   = "/";            // source folder (no ending slash except if root)
const char* tar_path   = "/test.tar";    // output tar archive to create
const char* targz_path = "/test.tar.gz"; // output tar archive to create
const char* dst_path   = nullptr;        // optional virtual folder in the tar archive, no ending slash, set to NULL to put everything in the root

std::vector<TAR::dir_entity_t> dirEntities; // storage for scanned dir entities

void removeTempFiles()
{
  Serial.println("Cleaning up temporary files");
  // cleanup test from other examples to prevent SPIFFS from exploding :)
  if(tarGzFS.exists(tar_path))
    tarGzFS.remove(tar_path);
  if(tarGzFS.exists(targz_path))
    tarGzFS.remove(targz_path);
}


void testTarPacker()
{
  // tar_path: destination, .tar packed file
  // dst_path: optional path prefix in tar archive
  removeTempFiles(); // cleanup previous examples
  Serial.println();
  Serial.printf("TarPacker::pack_files(&tarGzFS, dirEntities, &tarGzFS, tar_path, dst_path); Free heap: %lu bytes\n", HEAP_AVAILABLE() );
  TarPacker::setProgressCallBack( LZPacker::defaultProgressCallback );
  auto ret = TarPacker::pack_files(&tarGzFS, dirEntities, &tarGzFS, tar_path, dst_path);
  Serial.printf("Wrote %d bytes to %s\n", ret, tar_path);
}

// void testTarPackerStream()
// {
//   // tar_path: destination, .tar packed file
//   // dst_path: optional path prefix in tar archive
//   removeTempFiles(); // cleanup previous examples
//   Serial.println();
//   Serial.printf("TarPacker::pack_files(&tarGzFS, dirEntities, &tar, dst_path); Free heap: %lu bytes\n", HEAP_AVAILABLE() );
//   TarPacker::setProgressCallBack( LZPacker::defaultProgressCallback );
//   auto tar = tarGzFS.open(tar_path, "w");
//   if(!tar)
//     return;
//   auto ret = TarPacker::pack_files(&tarGzFS, dirEntities, &tar, dst_path);
//   tar.close();
//   Serial.printf("Wrote %d bytes to %s\n", ret, tar_path);
// }


void myTarMessageCallback(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  vsnprintf(tmp_path, 255, format, args);
  va_end(args);
}

void unzipTarArchive()
{

  if (FILESYSTEM.exists("/backup.tar"))
  {
    Serial.print("############## Decompress ");
    Serial.println("BACKUP ##########");
    TarUnpacker *TARUnpacker = new TarUnpacker();

    TARUnpacker->haltOnError(false);                                                           // stop on fail (manual restart/reset required)
    TARUnpacker->setTarVerify(true);                                                           // true = enables health checks but slows down the overall process
    TARUnpacker->setupFSCallbacks(targzTotalBytesFn, targzFreeBytesFn);                        // prevent the partition from exploding, recommended
    TARUnpacker->setTarProgressCallback(BaseUnpacker::defaultProgressCallback);                // prints the untarring progress for each individual file
    TARUnpacker->setTarStatusProgressCallback(BaseUnpacker::defaultTarStatusProgressCallback); // print the filenames as they're expanded
    TARUnpacker->setTarMessageCallback(BaseUnpacker::targzPrintLoggerCallback);                // tar log verbosity

    if (!TARUnpacker->tarExpander(FILESYSTEM, "/backup.tar", FILESYSTEM, "/"))
    {
      Serial.printf("tarExpander failed with return code #%d\n", TARUnpacker->tarGzGetError());
    }

    if (FILESYSTEM.remove("/backup.tar"))
    {
      Serial.println(".tar Archive gelöscht");
    }
  }
  if (FILESYSTEM.exists("/rezepte/rezepte.tar"))
  {
    Serial.print("############## Decompress ");
    Serial.println("REZEPTE ##########");
    TarUnpacker *TARUnpacker = new TarUnpacker();

    TARUnpacker->haltOnError(false);                                                           // stop on fail (manual restart/reset required)
    TARUnpacker->setTarVerify(true);                                                           // true = enables health checks but slows down the overall process
    TARUnpacker->setupFSCallbacks(targzTotalBytesFn, targzFreeBytesFn);                        // prevent the partition from exploding, recommended
    TARUnpacker->setTarProgressCallback(BaseUnpacker::defaultProgressCallback);                // prints the untarring progress for each individual file
    TARUnpacker->setTarStatusProgressCallback(BaseUnpacker::defaultTarStatusProgressCallback); // print the filenames as they're expanded
    TARUnpacker->setTarMessageCallback(BaseUnpacker::targzPrintLoggerCallback);                // tar log verbosity

    if (!TARUnpacker->tarExpander(FILESYSTEM, "/rezepte/rezepte.tar", FILESYSTEM, "/rezepte"))
    {
      Serial.printf("tarExpander failed with return code #%d\n", TARUnpacker->tarGzGetError());
    }

    if (FILESYSTEM.remove("/rezepte/rezepte.tar"))
    {
      Serial.println(".tar Archive gelöscht");
    }
  }
  else
    Serial.println("Kein .tar Archiv gefunden !");
}

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
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // configure web server
  Serial.println("Configuring Webserver ...");
  configureWebServer();

//   // startup web server
//   Serial.println("Starting Webserver ...");

//   unzipTarArchive();
//   Serial.println("TarPacker/TarGzPacker test");



//   removeTempFiles(); // cleanup previous examples
//   Serial.println("Gathering directory entitites");
//   TarPacker::collectDirEntities(&dirEntities, &tarGzFS, src_path); // collect dir and files at %{src_path}
//   // LZPacker::setProgressCallBack( LZPacker::defaultProgressCallback );

//   Serial.printf("Free heap: %lu bytes\n", HEAP_AVAILABLE());

//  // testTarPacker(); #### Aufrufen für .tar File


//   Serial.printf("Free heap: %lu bytes\n", HEAP_AVAILABLE());
}

void loop()
{



}

