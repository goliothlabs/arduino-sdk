
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Golioth.h>

#include "secrets.h"

#define CURRENT_VERSION "1.0.0"
int status = WL_IDLE_STATUS;
WiFiClientSecure net;
GoliothClient *client = GoliothClient::getInstance();

unsigned long lastMillis = 0;
unsigned long counter = 0;

void connect()
{
  Serial.print("checking wifi...");
  int tries = 0;
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    if (tries > 10)
    {
      Serial.println("Wifi not connected");
      return;
    }
    Serial.print(".");
    delay(1000);
    tries++;
  }
  Serial.println("Connected to WiFi!");

  Serial.println("connecting to broker...");
  tries = 0;

  client->setClient(net);
  client->setPSKId(PSK_ID);
  client->setPSK(PSK);
  while (!client->connect())
  {
    if (tries > 10)
    {
      Serial.println("Broker not connected");
      return;
    }
    Serial.print(".");
    delay(1000);
    tries++;
  }

  Serial.println("Connected to Golioth");

  client->onHello([](String name)
                  { Serial.println(name); });
  client->listenHello();
  client->onDesiredVersionChanged([](String pkg, String version, String hash)
                                  {
                                    Serial.println("New version - " + pkg + " " + version + " " + hash);
                                    if (pkg == "main" && version != CURRENT_VERSION)
                                    {
                                      Serial.println("Update available");
                                      client->downloadArtifact(pkg.c_str(), version.c_str());
                                    }
                                  });
  client->onDownloadArtifact([](String pkg, String version, uint8_t *buf, size_t buf_size, int current, int total)
                             {
                               Serial.print("Downloading " + pkg + " " + version + " " + String(current) + "/" + String(total) + " ");
                               Serial.print(buf_size);
                               Serial.println(" bytes");
                               if (current == 0)
                               {
                                 if (!Update.begin(total))
                                 {
                                   Serial.println("Not enough space to begin OTA");
                                 }
                               }
                               if (buf_size)
                               {
                                 Update.write(buf, buf_size);
                               }
                               if (current >= total)
                               {
                                 if (Update.end())
                                 {
                                   Serial.println("Update complete");
                                   if (Update.isFinished())
                                   {
                                     Serial.println("Update successfully completed. Rebooting.");
                                     ESP.restart();
                                   }
                                   else
                                   {
                                     Serial.println("Update not finished? Something went wrong!");
                                   }
                                 }
                                 else
                                 {
                                   Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                                 }
                               }
                             });
  client->listenDesiredVersion();
}

void setup()
{
  Serial.begin(115200);
  connect();
  Serial.println("Starting with version " + String(CURRENT_VERSION));
}

void loop()
{
  client->poll();

  if (!net.connected() || !client->connected())
  {
    connect();
  }

  if (millis() - lastMillis > 5 * 1000)
  {
    lastMillis = millis();
    counter++;
    client->setLightDBStateAtPath("/counter", String(counter).c_str());
    lastMillis = millis();
  }
}