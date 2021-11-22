#include <Arduino.h>
#ifdef ARDUINO_SAMD_MKR1010
#include <WiFiNINA.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif
#include <ArduinoJson.h>
#include <Golioth.h>

#include "secrets.h"

#define LED0 5
#define LED1 4
#define LED2 3
#define LED3 2

int status = WL_IDLE_STATUS;
#ifdef ARDUINO_SAMD_MKR1010
WiFiSSLClient net;
#endif
#ifdef ESP32
WiFiClientSecure net;
#endif
GoliothClient *client = GoliothClient::getInstance();

unsigned long lastMillis = 0;
unsigned long counter = 0;

void onLightDBMessage(String path, String payload) {
  Serial.println("incoming: " + path + " - " + payload);
  if (path.endsWith("led")) {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload);
    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
      bool value = kv.value().as<bool>();
      if (strcmp(kv.key().c_str(), "0") == 0) {
        Serial.println("Changing LED0");
        digitalWrite(LED0, value ? HIGH : LOW);
      }
      if (strcmp(kv.key().c_str(), "1") == 0) {
        Serial.println("Changing LED1");
        digitalWrite(LED1, value ? HIGH : LOW);
      }
      if (strcmp(kv.key().c_str(), "2") == 0) {
        Serial.println("Changing LED2");
        digitalWrite(LED2, value ? HIGH : LOW);
      }
      if (strcmp(kv.key().c_str(), "3") == 0) {
        Serial.println("Changing LED3");
        digitalWrite(LED3, value ? HIGH : LOW);
      }
    }
  }
}

void connect() {
  Serial.print("checking wifi...");
  int tries = 0;
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    if (tries > 10) {
      Serial.println("Wifi not connected");
      return;
    }
    Serial.print(".");
    delay(1000);
    tries++;
  }
  Serial.println("\nconnected to WiFi!\n");

  Serial.println("connecting to broker...");
  tries = 0;

#ifdef ESP32
  net.setCACert(GOLIOTH_ROOT_CA);
#endif

  client->setClient(net);
  client->setPSKId(PSK_ID);
  client->setPSK(PSK);
  while (!client->connect()) {
    if (tries > 10) {
      Serial.println("Broker not connected");
      return;
    }
    Serial.print(".");
    delay(1000);
    tries++;
  }

  Serial.println("Connected to MQTT");

  client->onHello([](String name) { Serial.println(name); });
  client->listenHello();
  client->onLightDBMessage(onLightDBMessage);
  client->listenLightDBStateAtPath("/led");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  digitalWrite(LED0, LOW);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);

  connect();
}

void loop() {
  client->poll();

  if (!net.connected() || !client->connected()) {
    connect();
  }

  if (millis() - lastMillis > 5 * 1000) {
    lastMillis = millis();
    counter++;
    client->setLightDBStateAtPath("/counter", String(counter).c_str());

    lastMillis = millis();
  }
}