#include <Arduino.h>
#include <Golioth.h>
#include <SPI.h>
#include <SparkFunBME280.h>
#include <SparkFun_AS3935.h>
#include <SparkFun_VEML6075_Arduino_Library.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#include "secrets.h"

int status = WL_IDLE_STATUS;
WiFiClientSecure net;
GoliothClient *client = GoliothClient::getInstance();

BME280 tempSensor;
VEML6075 uv;
SparkFun_AS3935 lightning;

#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

int LED_BUILTIN = 5;
#define A0 34
int G0 = 4;
int D0 = 23;
int A1 = 35;
int D1 = 27;
const int G3 = 17;
int G1 = 12;

int soilPin = A0;    // Pin number that measures analog moisture signal
int soilPower = G0;  // Pin number that will power the soil moisture sensor
const int lightningInt = G3;  // Interrupt pin for lightning detection
int spiCS = G1;               // SPI chip select pin

int readSoil();

#ifdef USE_RAIN_SENSOR
int RAIN = D1;  // Digital I/O pin for rain fall
volatile bool rainFlag = false;
// Function is called every time the rain bucket tips
void rainIRQ() { rainFlag = true; }
#endif

#ifdef USE_WIND_SENSOR
int WSPEED = D0;  // Digital I/O pin for wind speed
int WDIR = A1;    // Analog pin for wind direction
volatile bool windFlag = false;
// Function is called when the magnet in the anemometer is activated
int getWindDirection();
void wspeedIRQ() { windFlag = true; }
#endif

// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector.
int intVal = 0;
int noise = 2;      // Value between 1-7
int disturber = 2;  // Value between 1-10

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

  Serial.println("connecting to broker...");
  tries = 0;

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

  Serial.println("Connected to Golioth");
  client->onHello([](String name) { Serial.println(name); });
  client->listenHello();
}

void reboot() {
  Serial.println("Rebooting...");
  delay(5000);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("MicroMod Weather Carrier Board Test");
  Serial.println();

  Wire.begin();
  SPI.begin();

  if (tempSensor.beginI2C() == false) {  // Begin communication over I2C
    Serial.println("BME280 did not respond.");
    reboot();
  }
  if (uv.begin() == false) {
    Serial.println("VEML6075 did not respond.");
    reboot();
  }

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(soilPower, OUTPUT);
  digitalWrite(soilPower, LOW);
  // When lightning is detected the interrupt pin goes HIGH.
  pinMode(lightningInt, INPUT);

  // Initialization for weather meter
  // attach external interrupt pins to IRQ functions
#ifdef USE_RAIN_SENSOR
  pinMode(RAIN, INPUT_PULLUP);  // Input from wind meters rain gauge sensor
  attachInterrupt(digitalPinToInterrupt(RAIN), rainIRQ, FALLING);
#endif
#ifdef USE_WIND_SENSOR
  pinMode(WSPEED, INPUT_PULLUP);  // Input from wind meters windspeed sensor
  attachInterrupt(digitalPinToInterrupt(WSPEED), wspeedIRQ, FALLING);
#endif
  // turn on interrupts
  interrupts();

  if (lightning.beginSPI(spiCS, 2000000) == false) {
    Serial.println("Lightning Detector did not start up, freezing!");
    reboot();
  } else
    Serial.println("Schmow-ZoW, Lightning Detector Ready!");

  // The lightning detector defaults to an indoor setting at
  // the cost of less sensitivity, if you plan on using this outdoors
  // uncomment the following line:
  // lightning.setIndoorOutdoor(OUTDOOR);

  connect();
}

String buildData() {
  float temp = tempSensor.readTempC();
  float hum = tempSensor.readFloatHumidity();
  float press = tempSensor.readFloatPressure();
  float alt = tempSensor.readFloatAltitudeMeters();
  Serial.print("Temperature: ");
  Serial.println(temp, 2);
  Serial.print("Humidity: ");
  Serial.println(hum, 0);
  Serial.print("Pressure: ");
  Serial.println(press, 0);
  Serial.print("Altitude: ");
  Serial.println(alt, 1);

  float uva = uv.uva();
  float uvb = uv.uvb();
  float uvIndex = uv.index();
  Serial.print("UV A, B, index: ");
  Serial.println(String(uva) + ", " + String(uvb) + ", " + String(uvIndex));

  int soil = readSoil();
  Serial.print("Soil Moisture = ");
  Serial.println(soil);

#ifdef USE_WIND_SENSOR
  if (windFlag == true) {
    Serial.println("Wind click!");
    windFlag = false;
  }

  Serial.print("Wind direction: ");
  Serial.print(getWindDirection());
  Serial.println(" degrees");
#endif
  // Check interrupt flags
#ifdef USE_RAIN_SENSOR
  if (rainFlag == true) {
    Serial.println("Rain click!");
    rainFlag = false;
  }
#endif

  String data = "";
  data += "{";
  data += "\"temp\":" + String(temp) + ",";
  data += "\"humidity\":" + String(hum) + ",";
  data += "\"pressure\":" + String(press) + ",";
  data += "\"alt\":" + String(alt) + ",";
  data += "\"uva\":" + String(uva) + ",";
  data += "\"uvb\":" + String(uvb) + ",";
  data += "\"uvIndex\":" + String(uvIndex) + ",";
  data += "\"soil\":" + String(readSoil()) + ",";

  // Hardware has alerted us to an event, now we read the interrupt register
  if (digitalRead(lightningInt) == HIGH) {
    intVal = lightning.readInterruptReg();
    if (intVal == NOISE_INT) {
      Serial.println("Noise.");
      // Too much noise? Uncomment the code below, a higher number means better
      // noise rejection.
      // lightning.setNoiseLevel(noise);
    } else if (intVal == DISTURBER_INT) {
      Serial.println("Disturber.");
      // Too many disturbers? Uncomment the code below, a higher number means
      // better disturber rejection.
      // lightning.watchdogThreshold(disturber);
    } else if (intVal == LIGHTNING_INT) {
      Serial.println("Lightning Strike Detected!");
      // Lightning! Now how far away is it? Distance estimation takes into
      // account any previously seen events in the last 15 seconds.
      int distance = lightning.distanceToStorm();
      Serial.print("Approximately: ");
      Serial.print(distance);
      Serial.println("km away!");
      client->logInfo("Lightning Strike Detected! " + String(distance) +
                      "km away!");
      data += "\"lightning\":" + String(distance) + ",";
    }
  }
  if (data.endsWith(",")) {
    data.remove(data.length() - 1);
  }
  data += "}";
  return data;
}

// the loop function runs over and over again forever
unsigned long lastMillis = 0;
bool first = true;
void loop() {
  client->poll();

  if (!net.connected() || !client->connected()) {
    connect();
  }

  if (millis() - lastMillis > 5 * 60 * 1000 || first) {
    first = false;
    digitalWrite(LED_BUILTIN,
                 HIGH);  // turn the LED on (HIGH is the voltage level)

    String data = buildData();
    Serial.println();
    Serial.println(data);

    client->setLightDBStateAtPath("/station", data.c_str());
    client->sendLightDBStream("/station", data.c_str());
    digitalWrite(LED_BUILTIN,
                 LOW);  // turn the LED off by making the voltage LOW
    lastMillis = millis();
  }
}

int readSoil() {
  int moistVal = 0;  // Variable for storing moisture value
  // Power Senor
  digitalWrite(soilPower, HIGH);
  delay(10);
  moistVal = analogRead(soilPin);  // Read the SIG value from sensor
  digitalWrite(soilPower, LOW);    // Turn the sensor off
  int mappedVal = map(moistVal, 0, 512, 100, 0);  // Map the value to 0-100
  return mappedVal;  // Return current moisture value
}

#ifdef USE_WIND_SENSOR
int getWindDirection() {
  unsigned int adc;
  adc = analogRead(WDIR);  // get the current readings from the sensor

  if (adc < 380) return (113);
  if (adc < 393) return (68);
  if (adc < 414) return (90);
  if (adc < 456) return (158);
  if (adc < 508) return (135);
  if (adc < 551) return (203);
  if (adc < 615) return (180);
  if (adc < 680) return (23);
  if (adc < 746) return (45);
  if (adc < 801) return (248);
  if (adc < 833) return (225);
  if (adc < 878) return (338);
  if (adc < 913) return (0);
  if (adc < 940) return (293);
  if (adc < 967) return (315);
  if (adc < 990) return (270);
  return (-1);
}
#endif