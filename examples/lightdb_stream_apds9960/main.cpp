#include <SparkFun_APDS9960.h>
#ifdef ARDUINO_SAMD_MKR1010
#include <WiFiNINA.h>
#endif
#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Golioth.h>

#include "secrets.h"

SparkFun_APDS9960 apds = SparkFun_APDS9960();

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
  Serial.println("Connected to WiFi!");

  Serial.println("connecting to cloud gateway...");
  tries = 0;
#ifdef ESP32
  net.setCACert(GOLIOTH_ROOT_CA);
#endif

  client->setClient(net);
  client->setPSKId(PSK_ID);
  client->setPSK(PSK);
  while (!client->connect()) {
    if (tries > 10) {
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
  if (apds.init()) {
    Serial.println(F("APDS-9960 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }
  apds.enableLightSensor(false);
  apds.enableProximitySensor(false);
  apds.setProximityGain(PGAIN_2X);

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
    uint8_t proximity = 0;
    uint16_t lux = 0;
    uint16_t r = 0;
    uint16_t g = 0;
    uint16_t b = 0;
    apds.readProximity(proximity);
    apds.readRedLight(r);
    apds.readGreenLight(g);
    apds.readBlueLight(b);
    apds.readAmbientLight(lux);
    String historicalData = "{\"lux\":" + String(lux) +
                            ", \"proximity\":" + String(proximity) +
                            ", \"r\":" + String(r) + ", \"g\":" + String(g) +
                            ", \"b\":" + String(b) + "}";
    String rtData =
        "{\"counter\":" + String(counter) + ",\"env\":" + historicalData + "}";
    Serial.println("Updating `state` to " + rtData);
    Serial.println("Sending timeseries " + historicalData);

    client->setLightDBStateAtPath("/", rtData.c_str());
    client->sendLightDBStream("/env/", historicalData.c_str());

    lastMillis = millis();
  }
}