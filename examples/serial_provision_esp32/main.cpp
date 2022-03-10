
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Golioth.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "settings.h"

int status = WL_IDLE_STATUS;

WiFiClientSecure net;
GoliothClient* client = GoliothClient::getInstance();

unsigned long lastMillis = 0;
unsigned long counter = 0;

void connect() {
  String wifissid = getWifiSSID();
  String wifipass = getWifiPSK();
  if (wifissid.length() == 0 || wifipass.length() == 0) {
    delay(1000);
    return;
  }
  String pskid = getGoliothPSKID();
  String psk = getGoliothPSK();
  if (pskid.length() == 0 || psk.length() == 0) {
    delay(1000);
    return;
  }
  Serial.print("checking wifi...");
  int tries = 0;
  WiFi.begin(wifissid.c_str(), wifipass.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    if (tries > 5) {
      Serial.println("Wifi not connected");
      return;
    }
    Serial.print(".");
    delay(1000);
    tries++;
  }
  Serial.println("Connected to WiFi!");

  Serial.println("connecting to cloud gateway...");
  tries = 0;

  net.setCACert(GOLIOTH_ROOT_CA);

  client->setClient(net);
  client->setPSKId(pskid.c_str());
  client->setPSK(psk.c_str());
  while (!client->connect()) {
    if (tries > 1) {
      Serial.println("not connected");
      return;
    }
    Serial.print(".");
    delay(1000);
    tries++;
  }

  Serial.println("Connected to Golioth");

  client->onHello([](String name) { Serial.println(name); });
  client->listenHello();
}

void setup() {
  Serial.begin(115200);

  client->setClient(net);

  setupSettings();

  delay(100);
  connect();
}

void loop() {
  client->poll();
  handleSettingsCLI();

  if (!net.connected() || !client->connected()) {
    connect();
  }

  if (millis() - lastMillis > 5 * 1000 && client->connected()) {
    lastMillis = millis();
    counter++;
    client->setLightDBStateAtPath("/counter", String(counter).c_str());
    lastMillis = millis();
  }
}