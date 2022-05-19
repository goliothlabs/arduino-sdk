#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_ThinkInk.h>
#include <Adafruit_LIS3DH.h>
#include <Golioth.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "coin.h"
#include "magtaglogo.h"
#include "secrets.h"

int status = WL_IDLE_STATUS;
WiFiClientSecure net;
GoliothClient *client = GoliothClient::getInstance();

Adafruit_NeoPixel intneo = Adafruit_NeoPixel(4, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, -1, -1);
Adafruit_LIS3DH lis = Adafruit_LIS3DH();


void setHexColor(int pixel, String color) {
  int number = (int)strtol(&color[1], NULL, 16);
  // Split them up into r, g, b values
  int r = number >> 16;
  int g = number >> 8 & 0xFF;
  int b = number & 0xFF;
  Serial.println("color: " + color + " - " + String(r) + " - " + String(g) +
                 " - " + String(b));
  intneo.setPixelColor(pixel, r, g, b);
  intneo.show();
}


void play_tune(const uint8_t *audio, uint32_t audio_length) {
  uint32_t t;
  uint32_t prior, usec = 1000000L / SAMPLE_RATE;

  for (uint32_t i=0; i<audio_length; i++) {
    while((t = micros()) - prior < usec);
    dacWrite(A0, audio[i]);
    prior = t;
  }
}

String currentText = "";
String currentColor0 = "#000000";
String currentColor1 = "#000000";
String currentColor2 = "#000000";
String currentColor3 = "#000000";
void onLightDBMessage(String path, String payload) {
  Serial.println("incoming: " + path + " - " + payload);
  if (path.endsWith("desired")) {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload);

    if (doc.containsKey("buzz")) {
      digitalWrite(SPEAKER_SHUTDOWN, HIGH);
      play_tune(audio, sizeof(audio));
      digitalWrite(SPEAKER_SHUTDOWN, LOW);
      client->deleteLightDBStateAtPath("/desired/buzz");
    }

    JsonObject leds = doc["leds"].as<JsonObject>();
    for (JsonPair kv : leds) {
      String value = kv.value().as<String>();
      if (strcmp(kv.key().c_str(), "0") == 0) {
        if (!value.equals(currentColor0)) {
          Serial.println("Changing LED0");
          currentColor0 = value;
          setHexColor(0, value);
        }
      }
      if (strcmp(kv.key().c_str(), "1") == 0) {
        if (!value.equals(currentColor1)) {
          Serial.println("Changing LED1");
          currentColor1 = value;
          setHexColor(1, value);
        }
      }
      if (strcmp(kv.key().c_str(), "2") == 0) {
        if (!value.equals(currentColor2)) {
          Serial.println("Changing LED2");
          currentColor2 = value;
          setHexColor(2, value);
        }
      }
      if (strcmp(kv.key().c_str(), "3") == 0) {
        if (!value.equals(currentColor3)) {
          Serial.println("Changing LED3");
          currentColor3 = value;
          setHexColor(3, value);
        }
      }
    }

    if (doc.containsKey("text")) {
      String msg = doc["text"].as<String>();
      if (!msg.equals(currentText)) {
        currentText = msg;
        display.clearBuffer();
        display.setTextSize(3);
        display.setTextColor(EPD_BLACK);
        display.setCursor(0, 0);
        if (msg.indexOf("\n") == -1) {
          display.print(msg);
        } else {
          if (!msg.endsWith("\n")){
            msg += "\n";
          }
          do {
            String line = msg.substring(0, msg.indexOf("\n"));
            display.print(line);
            display.setCursor(0, display.getCursorY() + 24);
            msg = msg.substring(msg.indexOf("\n") + 1);
          } while(msg.indexOf("\n") != -1);
        }
        display.display();
      }
    }
    if (client->connected()) {
      String textToSend = currentText;
      textToSend.replace("\n", "\\n");
      String state = "{\"leds\": { \"0\": \"" + currentColor0 + "\", \"1\": \"" + currentColor1 + "\", \"2\": \"" + currentColor2 + "\", \"3\": \"" + currentColor3+ "\" },\"text\": \"" + String(textToSend) + "\" }";
      Serial.println("sending: " + state);
      client->setLightDBStateAtPath("/state", state.c_str());
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
  client->onHello([](String name) {
    Serial.println(name);
    display.clearBuffer();
    display.setTextSize(3);
    display.setTextColor(EPD_BLACK);
    display.setCursor(0, 0);
    display.println("Connected to Golioth");
    display.println("Device name");
    display.println(name);
    display.display();
  });
  client->listenHello();
  client->onLightDBMessage(onLightDBMessage);
  client->listenLightDBStateAtPath("/desired");
}

void setup() {
  Serial.begin(115200);
  //while (!Serial) { delay(10); }
  delay(100);
  Serial.println("Adafruit EPD Portal demo");

  intneo.begin();
  intneo.setBrightness(50);
  intneo.show(); // Initialize all pixels to 'off'

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(BUTTON_D, INPUT_PULLUP);

  pinMode(SPEAKER_SHUTDOWN, OUTPUT);
  digitalWrite(SPEAKER_SHUTDOWN, LOW);

  // Red LED
  pinMode(13, OUTPUT);

  // Neopixel power
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, LOW); // on

  display.begin(THINKINK_MONO);

  if (! lis.begin(0x19)) {
    Serial.println("Couldnt start LIS3DH");
    display.clearBuffer();
    display.setTextSize(3);
    display.setTextColor(EPD_BLACK);
    display.setCursor(20, 40);
    display.print("No LIS3DH?");
    display.display();
    while (1) delay(100);
  }

  analogReadResolution(12); //12 bits
  analogSetAttenuation(ADC_11db);  //For all pins
  display.clearBuffer();
  display.clearDisplay();
  //display.drawBitmap(0, 38, magtaglogo_mono, MAGTAGLOGO_WIDTH, MAGTAGLOGO_HEIGHT, EPD_BLACK);
  display.display();

  connect();
}

uint8_t rotation = 0;
unsigned long lastMillis = 0;
bool first = true;
void loop() {
  client->poll();

  if (!net.connected() || !client->connected()) {
    connect();
  }

  if (millis() - lastMillis > 1 * 1000 || first) {
      first = false;
      sensors_event_t event;
      lis.getEvent(&event);

      /* Display the results (acceleration is measured in m/s^2) */
      Serial.print("X: "); Serial.print(event.acceleration.x);
      Serial.print(" \tY: "); Serial.print(event.acceleration.y);
      Serial.print(" \tZ: "); Serial.print(event.acceleration.z);
      Serial.println(" m/s^2 ");
      if ((event.acceleration.x < -5) && (abs(event.acceleration.y) < 5)) {
        rotation = 1;
      }
      if ((abs(event.acceleration.x) < 5) && (event.acceleration.y > 5)) {
        rotation = 0;
      }
      if ((event.acceleration.x > 5) && (abs(event.acceleration.y) < 5)) {
        rotation = 3;
      }
      if ((abs(event.acceleration.x) < 5) && (event.acceleration.y < -5)) {
        rotation = 2;
      }

      int light = analogRead(LIGHT_SENSOR);
      Serial.print("Light sensor: ");
      Serial.println(light);

      String data = "";
      data += "{";
      data += "\"light\":" + String(light) + ",";
      data += "\"accX\":" + String(event.acceleration.x) + ",";
      data += "\"accY\":" + String(event.acceleration.y) + ",";
      data += "\"accZ\":" + String(event.acceleration.z) + ",";
      if (data.endsWith(",")) {
        data.remove(data.length() - 1);
      }
      data += "}";
      Serial.println("Sending data: " + data);
      client->setLightDBStateAtPath("/state", data.c_str());
      lastMillis = millis();
  }

  if (! digitalRead(BUTTON_A)) {
    Serial.println("Button A pressed");
  }
  else if (! digitalRead(BUTTON_B)) {
    Serial.println("Button B pressed");
  }
  else if (! digitalRead(BUTTON_C)) {
    Serial.println("Button C pressed");
  }
  else if (! digitalRead(BUTTON_D)) {
    Serial.println("Button D pressed");
    digitalWrite(SPEAKER_SHUTDOWN, HIGH);
    play_tune(audio, sizeof(audio));
    digitalWrite(SPEAKER_SHUTDOWN, LOW);
  }

  delay(10);
}