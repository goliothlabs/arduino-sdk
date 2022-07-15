
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Golioth.h>

#include "secrets.h"

#define LED0 LED_BUILTIN
#define LED1 4
#define LED2 3
#define LED3 2

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
  //net.setInsecure();
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
  client->onRemoteFunction("reset", [](String callID, JsonArray params){
    client->ackRemoteFunction(callID, RPC_OK);
    delay(1000);
    ESP.restart();
  });
  client->onRemoteFunction("random", [](String callID, JsonArray params){
    client->ackRemoteFunction(callID, RPC_OK, String(rand()));    
  });
  client->onRemoteFunction("toggle", [](String callID, JsonArray params)
                             {
                               Serial.println("toggle");
                               int ledIndex = params.getElement(0).as<int>();
                               int ledPin = -1;
                               switch (ledIndex)
                               {
                                  case 0:
                                    ledPin = LED0;
                                    break;
                                  case 1:
                                    ledPin = LED1;
                                    break;
                                  case 2:
                                    ledPin = LED2;
                                    break;
                                  case 3:
                                    ledPin = LED3;
                                    break;
                                  default:
                                    ledPin = -1;
                                    break;
                               }
                               if (ledIndex < 0 || ledIndex > 4) {
                                Serial.println("invalid pin");
                                client->ackRemoteFunction(callID, RPC_INVALID_ARGUMENT);
                                return;
                               }
                               Serial.println("valid pin");
                               digitalWrite(ledPin, !digitalRead(ledPin));
                               client->ackRemoteFunction(callID, RPC_OK);
                             });

  client->logInfo("Connected to Golioth with IP: ");
}

void setup()
{
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
    client->logDebug("updated counter to: " + String(counter));

    lastMillis = millis();
  }
}