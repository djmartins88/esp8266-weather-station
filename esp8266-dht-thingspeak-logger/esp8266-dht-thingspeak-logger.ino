/**

https://github.com/sunrobotics/Ideaspark_Weather_Station_Kit/blob/master/2_Source_Code/WeatherStationDemo/WeatherStationDemo.ino <<< base

https://github.com/adafruit/DHT-sensor-library/blob/master/examples/DHT_Unified_Sensor/DHT_Unified_Sensor.ino <<< upgrade DHT (humidity) based on
*/

// Wifi
#include <ESP8266WiFi.h>

// display
#include <Wire.h>
#include "SSD1306Wire.h"

// atmosphere and light
#include <Adafruit_BMP085.h>

// humidity
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// DHT Settings (Humidity)
#define DHTPIN D5  // what digital pin we're connected to. If you are not using NodeMCU change D6 to real pin
#define DHTTYPE DHT11

// Display pins
#define SCK_PIN D4
#define SDA_PIN D3

// 12v fan controlled by relay - relay pin
#define FAN_RELAY_PIN  D8  // The ESP8266 pin connected to the IN pin of relay

// Water atomizer controlled by relay - relay pin
#define ATOMIZER_RELAY_PIN D7 // TODO

const char* SSID = "NAUGHTY";
const char* PASSWORD = "VERYGOODBOY"; 
const char* THINGSPEAK_API_KEY = "DONTFOMOONCRYPTO";
const char* thingspeak_host = "api.thingspeak.com";

SSD1306Wire display(0x3c, SDA_PIN, SCK_PIN);

// Atmosphere sensor setup
Adafruit_BMP085 bmp;
const int Atom_ADDR = 0b1110111;  // address:0x77
bool ATMOSPHERE_ON = false;

// Ligth sensor setup
const int Light_ADDR = 0b0100011;  // address:0x23

// Humidity sensor setup
DHT_Unified dht(DHTPIN, DHTTYPE);
sensors_event_t event;

// Update Things speak every 600 seconds = 10 minutes. Min with Thingspeak is ~20 seconds
const int UPDATE_INTERVAL_SECONDS = 600;

// Read sensors every 10 seconds
const int READ_INTERVAL_SECONDS = 60;

// Turn on Fan when temperature is above this value
const int TEMPERATURE_TRESHOLD = 15;

// Turn on water atomizer when humidity is bellow this value
const int HUMIDITY_TRESHOLD = 80;

// Control atomizer to work in x minutes intervals
const int ATOMIZER_REST_INTERVAL_SECONDS = 3 * 60;

long READ_TIME = 0;
long UPDATE_TIME = 0;

long ATOMIZER_TIME_START = -360000; // so automizer can start at beggining if needed

bool FAN_WORKING = false;
bool ATOM_WORKING = false;

long TIME = 0;

int humidity = 0;
int temperature = 0;
int light = 0;
int pressure = 0;

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println("- - - - -  SETUP  - - - - -");

  Wire.pins(SDA_PIN, SCK_PIN);
  Wire.begin(0, 2);
  // initialize Atmosphere sensor
  Wire.beginTransmission(Atom_ADDR);
  if (!bmp.begin()) {
    Serial.println("Could not find BMP180 at 0x77");
  }else{
    Serial.println("Found BMP180 at 0x77");
    ATMOSPHERE_ON = true;
  }
  Wire.endTransmission();

  // initialize light sensor
  Wire.beginTransmission(Light_ADDR);
  Wire.write(0b00000001);
  Wire.endTransmission();

  // initialize humidity sensor
  dht.begin();

  // Display setup
  display.init();
  display.clear();
  display.display();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.flipScreenVertically();

  // Wifi setup
  Serial.print("Connecting to "); Serial.print(SSID); Serial.print(" ");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(" connected: "); Serial.println(WiFi.localIP());

  // 12v fan relay setup
  pinMode(FAN_RELAY_PIN, OUTPUT);

  // Water atomizer setup
  pinMode(ATOMIZER_RELAY_PIN, OUTPUT);

  Serial.println("- - - - -  SETUP COMPLETE  - - - - -");
}

// read light value from light sensor
void readLight() {

  Serial.print("Light = ");
  int tempLight = 0;

  // reset
  Wire.beginTransmission(Light_ADDR);
  Wire.write(0b00000111);
  Wire.endTransmission();

  Wire.beginTransmission(Light_ADDR);
  Wire.write(0b00100000);
  Wire.endTransmission();
  // typical read delay 120ms
  delay(120);
  Wire.requestFrom(Light_ADDR, 2);  // 2byte every time
  for (tempLight = 0; Wire.available() >= 1;) {
    char c = Wire.read();
    tempLight = (tempLight << 8) + (c & 0xFF);
  }
  light = tempLight / 1.2;
  Serial.println(light);
}

// read pressure from Atmosphere sensor - not being used
void readPressure(){

  if (ATMOSPHERE_ON) {
    Serial.print("Pressure = ");
    pressure = bmp.readPressure();

    //String p = "P=" + String(bmp.readPressure()) + " Pa";
    //String a = "A=" + String(bmp.readAltitude(103000)) + " m";

    Serial.print(pressure); Serial.println(" Pascal");
    //Serial.print(p); Serial.println(a);
  } else {
    Serial.println("No atmosphere sensor found on startup.");
  }
}

// read temperature from Atmosphere sensor
void readTemperature(){

  if (ATMOSPHERE_ON) {
    Serial.print("Temperature = ");
    temperature = bmp.readTemperature();
    Serial.print(temperature); Serial.println("º C");
  } else {
    Serial.println("No atmosphere sensor found on startup.");
  }

}

// Read humidity from DHT sensor
void readHumidity() {

  Serial.print("Humidity = ");
  dht.humidity().getEvent(&event);
  humidity = event.relative_humidity;
  Serial.print(humidity); Serial.println(" %");

  // if u want to use for temp: dht.temperature().getEvent(&event);
}

// update latest sensor readings to ThingSpeak
void updateThingsSpeak() {

  // Serial.print("connecting to "); Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(thingspeak_host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/update?api_key=";
  url += THINGSPEAK_API_KEY;
  url += "&field1="; url += temperature;
  url += "&field2="; url += humidity;
  url += "&field3="; url += light;
  url += "&field4="; url += pressure;
  url += "&field5="; url += FAN_WORKING;
  url += "&field6="; url += ATOM_WORKING;

  // Serial.print("Requesting URL: "); Serial.println(url);

  // This will send the request to the server
  client.print("GET " + url + " HTTP/1.1\r\nHost: " + thingspeak_host + "\r\nConnection: close\r\n\r\n");
  delay(10);
  while (!client.available()) {
    delay(100);
    Serial.print(".");
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
}

// Simply write a message on the display
void displayMessage(String message) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, message);
  display.display();
}

// print sensor data to serial
void printWeatherStationSensorData() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, "Humidity: " + String(humidity) + " %");
  display.drawStringMaxWidth(0, 12, 128, "Temperature: " + String(temperature) + " ºC");
  display.drawStringMaxWidth(0, 24, 128, "Light: " + String(light));
  display.drawStringMaxWidth(0, 48, 128, "Fan: " + String(FAN_WORKING ? "On" : "Off"));
  display.drawStringMaxWidth(64, 48, 128, "Atom: " + String(ATOM_WORKING ? "On" : "Off"));
  display.display();
}

void checkAndAdjustTemperature() {

  // check temperature is above treshold
  if (temperature > TEMPERATURE_TRESHOLD && !FAN_WORKING) {
    digitalWrite(FAN_RELAY_PIN, HIGH); // turn on fan via relay
    FAN_WORKING = true;

    Serial.println("Staring Fan - " + String(temperature) + " ºC temperature is above treshold (" + String(TEMPERATURE_TRESHOLD) + ") :: " + String(millis()));
  } else if (temperature < TEMPERATURE_TRESHOLD && FAN_WORKING) {
    digitalWrite(FAN_RELAY_PIN, LOW); // turn off fan via relay
    FAN_WORKING = false;

    Serial.println("Stopping Fan - " + String(temperature) + " ºC temperature is bellow or equal treshold (" + String(TEMPERATURE_TRESHOLD) + ") :: " + String(millis()));
  }

}

void checkAndAdjustHumidity() {

  // check humidity is bellow treshold
  if (humidity < HUMIDITY_TRESHOLD) {

    // atomizer is supposed to be working enforce he works X time, turns off, then starts again, until the reading is bellow treshold
    if (TIME - ATOMIZER_TIME_START > 2 * ATOMIZER_REST_INTERVAL_SECONDS * 1000) {
      digitalWrite(ATOMIZER_RELAY_PIN, HIGH); // turn on water atomizer
      ATOMIZER_TIME_START = TIME;
      ATOM_WORKING = true;

      Serial.println("Starting atomizer - " + String(humidity) + " % humidity is bellow treshold (" + String(HUMIDITY_TRESHOLD) + ") :: " + String(TIME));
    } 
    // atomizer is supposed to be working but the first X seconds have passed, turn off atomizer
    else if (TIME - ATOMIZER_TIME_START > ATOMIZER_REST_INTERVAL_SECONDS * 1000 && ATOM_WORKING) { 
        
        digitalWrite(ATOMIZER_RELAY_PIN, LOW); // turn off water atomizer
        ATOM_WORKING = false;
        
        Serial.println("Making a break on Atomizer - " + String(humidity) + " % humidity is bellow treshold (" + String(HUMIDITY_TRESHOLD) + ") :: " + String(TIME));
    }

  } else if (ATOM_WORKING) {

    digitalWrite(ATOMIZER_RELAY_PIN, LOW); // turn off water atomizer
    ATOM_WORKING = false;

    Serial.println("Stopping Atomizer - " + String(humidity) + " % humidity is above or equal treshold (" + String(HUMIDITY_TRESHOLD) + ") :: " + String(TIME));
  }

}

// check if sensors are above treshold and act on it - play God.
void adjustWeather() {
  checkAndAdjustTemperature();
  checkAndAdjustHumidity();
}

void loop() {

  TIME = millis();

  // read values from sensors
  if (TIME - READ_TIME > READ_INTERVAL_SECONDS * 1000) {

    readTemperature();
    readLight();
    //readPressure();
    readHumidity();

    READ_TIME = TIME;

    // play God.
    adjustWeather();
  }

  // update ThingSpeak
  if (TIME - UPDATE_TIME > UPDATE_INTERVAL_SECONDS * 1000) {
    updateThingsSpeak();

    UPDATE_TIME = TIME;
  }

  printWeatherStationSensorData();

  // Go back to sleep. If your sensor is battery powered you might
  delay(1000 * READ_INTERVAL_SECONDS);
}
