/**

https://github.com/sunrobotics/Ideaspark_Weather_Station_Kit/blob/master/2_Source_Code/WeatherStationDemo/WeatherStationDemo.ino <<< base

https://github.com/adafruit/DHT-sensor-library/blob/master/examples/DHT_Unified_Sensor/DHT_Unified_Sensor.ino <<< upgrade DHT (humidity) based on
*/

// Wifi
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

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
#define ATOMIZER_RELAY_PIN D7 

#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define DATACAKE_HOST "api.datacake.co"
#define DATACAKE_PORT 443
#define SUPABASE_HOST "aqyrdklquuwloithqtal.supabase.co"
#define SUPABASE_PORT 443

const char* SSID = "NAUGHTY";                        // > your wifi ssid
const char* PASSWORD = "VERYGOODBOY";                // > your wifi password
const char* THINGSPEAK_API_KEY = "DONTFOMOONCRYPTO"; // > use your API key

const char* DATA_CAKE_API_URL = "https://api.datacake.co/integrations/api/a3f2102d-****-****-****-ed1d240d8ff9/"; // > your device api url from data cake device Configuration
const char* DATA_CAKE_API_KEY = "DONTFOMOONCRYPTO"; // > your API token from data cake account settings
const char* DATA_CAKE_DEVICE_ID = "53283b61-****-****-****-e26079f231ac"; // > your device id from data cake device Configuration

const char* SUPABASE_DB_API_ENDPOINT = "https://aqyrdklquuwloithqtal.supabase.co"; // > your supabase project url, you can check on project settings
// > your access token from supabase project settings
const char* SUPABASE_ACCESS_TOKEN = "DONTFOMOONCRYPTO";
const char* SUPABASE_CONFIG_TABLE = "weather-station-config"; // > your table name as created on supabase

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

// Control atomizer to work in x minutes intervals
const int ATOMIZER_REST_INTERVAL_SECONDS = 3 * 60;

// Turn on Fan when temperature is above this value
int TEMPERATURE_TRESHOLD = 20;

// Turn on water atomizer when humidity is bellow this value
int HUMIDITY_TRESHOLD = 60;

long READ_TIME = 0;
long UPDATE_TIME = 0;

long ATOMIZER_TIME_START = -360000; // so automizer can start at beggining if needed

bool FAN_WORKING = false;
bool ATOM_WORKING = false;
bool RELAY_CHANGE = false;

long TIME = 0;

int humidity = 0;
int temperature = 0;
int light = 0;
int pressure = 0;

void setup() {
  Serial.begin(115200);
  delay(10);

  String setupStatus = ".";

  Serial.println("- - - - -  SETUP  - - - - -");

  // Display setup
  display.init();
  display.clear();
  display.flipScreenVertically();

  displayMessage(setupStatus);
  delay(500);

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

  setupStatus += ATMOSPHERE_ON ? "." : " ";
  displayMessage(setupStatus);
  delay(500);

  // initialize light sensor
  Wire.beginTransmission(Light_ADDR);
  Wire.write(0b00000001);
  Wire.endTransmission();
  
  setupStatus += ".";
  displayMessage(setupStatus);
  delay(500);

  // initialize humidity sensor
  dht.begin();

  setupStatus += ".";
  displayMessage(setupStatus);
  delay(500);

  // 12v fan relay setup
  pinMode(FAN_RELAY_PIN, OUTPUT);
  setupStatus += ".";
  displayMessage(setupStatus);
  delay(500);

  // Water atomizer setup
  pinMode(ATOMIZER_RELAY_PIN, OUTPUT);
  setupStatus += ".";
  displayMessage(setupStatus);
  delay(500);

  // Wifi setup
  Serial.print("Connecting to "); Serial.print(SSID); Serial.print(" ");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(" connected: "); Serial.println(WiFi.localIP());

  setupStatus = WiFi.localIP().toString();
  displayMessage(setupStatus);

  Serial.println("- - - - -  SETUP COMPLETE  - - - - -");
}

// read light value from light sensor
void readLight() {

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
  Serial.print("l: "); Serial.print(light); Serial.print("\t");
}

// read pressure from Atmosphere sensor - not being used
void readPressure(){

  if (ATMOSPHERE_ON) {
    Serial.print("Pressure = ");
    pressure = bmp.readPressure();

    //String p = "P=" + String(bmp.readPressure()) + " Pa";
    //String a = "A=" + String(bmp.readAltitude(103000)) + " m";

    Serial.print("p: "); Serial.print(pressure); Serial.print(" Pascal\t");
    //Serial.print(p); Serial.println(a);
  } else {
    Serial.println("No atmosphere sensor found on startup.");
  }
}

// read temperature from Atmosphere sensor
void readTemperature(){

  if (ATMOSPHERE_ON) {
    temperature = bmp.readTemperature();
    Serial.print("t: "); Serial.print(temperature); Serial.print("º C\t");
  } else {
    Serial.println("No atmosphere sensor found on startup.");
  }

}

// Read humidity from DHT sensor
void readHumidity() {

  dht.humidity().getEvent(&event);
  humidity = event.relative_humidity;
  Serial.print("h: ");Serial.print(humidity); Serial.println(" %");

  // if u want to use for temp: dht.temperature().getEvent(&event);
}

// update latest sensor readings to ThingSpeak
void updateThingSpeak() {

  // Serial.print("connecting to "); Serial.println(THINGSPEAK_HOST);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(THINGSPEAK_HOST, THINGSPEAK_PORT)) {
    Serial.println("ThingSpeak - connection failed");
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
  client.print("GET " + url + " HTTP/1.1\r\nHost: " + THINGSPEAK_HOST + "\r\nConnection: close\r\n\r\n");
  delay(10);
  while (!client.available()) {
    delay(100);
    Serial.print(".");
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line);
  }

  client.stop();
  Serial.println();

}

// update latest sensor readings to Datacake
void updateDatacake() {

  // Serial.print("connecting to "); Serial.println(THINGSPEAK_HOST);

  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect(DATACAKE_HOST, DATACAKE_PORT)) {
    Serial.println("connection failed.");
    return;
  }

  HTTPClient http;

  StaticJsonDocument<512> doc;
  
  doc["device"] = DATA_CAKE_DEVICE_ID;
  doc["TEMPERATURE"] = temperature;
  doc["HUMIDITY"] = humidity;
  doc["LIGHT"] = light;
  doc["PRESSURE"] = pressure;
  doc["FAN_ON"] = FAN_WORKING;
  doc["ATOM_ON"] = ATOM_WORKING;

  char post_message[512]; 
  serializeJsonPretty(doc, post_message);
  //Serial.println(post_message);

  http.begin(client, DATA_CAKE_API_URL); 
  http.addHeader("Authorization", "Token " + String(DATA_CAKE_API_KEY)); 
  http.addHeader("Content-Type", "application/json"); 
  int code = http.PUT(post_message); 
  
  if (code < 300){
      //Serial.print("Datacake: Posted sucessfuly");
      String response = http.getString();  
      //Serial.println(response );
  } else {
    Serial.print("Failed to post to Datacake: "); Serial.println(code); 
  }

  // free resources
  http.end();
  client.stop();

}

// get tresholds from supabase
void getSupabaseInfo() {

  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect(SUPABASE_HOST, SUPABASE_PORT)) {
    Serial.println("connection failed.");
    return;
  }

  HTTPClient http;
  
  // Read data from supabase_config_table
  String api_url = String(SUPABASE_DB_API_ENDPOINT) + "/rest/v1/" + String(SUPABASE_CONFIG_TABLE);
  //Serial.println(api_url);

  http.begin(client, api_url); 
  http.addHeader("apikey", String(SUPABASE_ACCESS_TOKEN));
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_ACCESS_TOKEN));
  int code = http.GET();

  if (code < 300){
      //Serial.print("Supabased Get sucessfuly: ");
      String response = http.getString();  
      //Serial.println(response);

      StaticJsonDocument<512> doc;
      deserializeJson(doc, response);

      // set tresholds with retrieved data
      TEMPERATURE_TRESHOLD = doc[0]["TEMPERATURE_TRESHOLD"];
      HUMIDITY_TRESHOLD = doc[0]["HUMIDITY_TRESHOLD"];

      //Serial.println("Temp treshold: " + String(TEMPERATURE_TRESHOLD));
      //Serial.println("Humid treshold: " + String(HUMIDITY_TRESHOLD));

  } else {
    Serial.print("Failed to get from Firebase - "); Serial.println(code); 
  } 

  // Free resources
  http.end();
  client.stop();

  /**
  API DOCS from supabase lets u know what to send in your request, example:

Insert a row
curl -X POST 'https://aqyrdklquuwloithqtal.supabase.co/rest/v1/weather-station-config' \
-H "apikey: SUPABASE_CLIENT_ANON_KEY" \
-H "Authorization: Bearer SUPABASE_CLIENT_ANON_KEY" \
-H "Content-Type: application/json" \
-H "Prefer: return=minimal" \
-d '{ "some_column": "someValue", "other_column": "otherValue" }'

Insert many rows
curl -X POST 'https://aqyrdklquuwloithqtal.supabase.co/rest/v1/weather-station-config' \
-H "apikey: SUPABASE_CLIENT_ANON_KEY" \
-H "Authorization: Bearer SUPABASE_CLIENT_ANON_KEY" \
-H "Content-Type: application/json" \
-d '[{ "some_column": "someValue" }, { "other_column": "otherValue" }]'

Upsert matching rows
curl -X POST 'https://aqyrdklquuwloithqtal.supabase.co/rest/v1/weather-station-config' \
-H "apikey: SUPABASE_CLIENT_ANON_KEY" \
-H "Authorization: Bearer SUPABASE_CLIENT_ANON_KEY" \
-H "Content-Type: application/json" \
-H "Prefer: resolution=merge-duplicates" \
-d '{ "some_column": "someValue", "other_column": "otherValue" }'


  // Example code:
  HTTPClient http;
  StaticJsonDocument<512> doc;
  
  doc["TEMPERATURE"] = temperature;
  doc["HUMIDITY"] = humidity;
  doc["LIGHT"] = light;
  doc["PRESSURE"] = pressure;
  doc["FAN_ON"] = FAN_WORKING;
  doc["ATOM_ON"] = ATOM_WORKING;

  char message[512]; 
  serializeJsonPretty(doc, message);
  
  String api_url = String(SUPABASE_DB_API_ENDPOINT) + "/rest/v1/" + String(SUPABASE_CONFIG_TABLE);

  Serial.println(api_url);

  http.begin(client, api_url); 
  http.addHeader("apikey", String(SUPABASE_ACCESS_TOKEN));
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_ACCESS_TOKEN));
  int code = http.POST();

  **/
}

// Simply write a message on the display
void displayMessage(String message) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, message);
  display.display();
}

// show sensor data on display
void displayWeatherStationSensorData() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, "Humidity: " + String(humidity) + " %");
  display.drawStringMaxWidth(0, 12, 128, "Temperature: " + String(temperature) + " ºC");
  display.drawStringMaxWidth(0, 24, 128, "Light: " + String(light));
  display.drawStringMaxWidth(0, 36, 128, "Max ºC: " + String(TEMPERATURE_TRESHOLD));
  display.drawStringMaxWidth(0, 48, 128, "Fan: " + String(FAN_WORKING ? "On" : "Off"));
  display.drawStringMaxWidth(64, 36, 128, "min %: " + String(HUMIDITY_TRESHOLD));
  display.drawStringMaxWidth(64, 48, 128, "Atom: " + String(ATOM_WORKING ? "On" : "Off"));
  display.display();
}

void checkAndAdjustTemperature() {

  // check temperature is above treshold
  if (temperature > TEMPERATURE_TRESHOLD && !FAN_WORKING) {
    digitalWrite(FAN_RELAY_PIN, HIGH); // turn on fan via relay
    FAN_WORKING = true;

    Serial.println("Staring Fan - " + String(temperature) + " ºC temperature is above treshold (" + String(TEMPERATURE_TRESHOLD) + ") :: " + String(millis()));
    RELAY_CHANGE = true;
  } else if (temperature < TEMPERATURE_TRESHOLD && FAN_WORKING) {
    digitalWrite(FAN_RELAY_PIN, LOW); // turn off fan via relay
    FAN_WORKING = false;

    Serial.println("Stopping Fan - " + String(temperature) + " ºC temperature is bellow or equal treshold (" + String(TEMPERATURE_TRESHOLD) + ") :: " + String(millis()));
    RELAY_CHANGE = true;
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
      RELAY_CHANGE = true;
    } 
    // atomizer is supposed to be working but the first X seconds have passed, turn off atomizer
    else if (TIME - ATOMIZER_TIME_START > ATOMIZER_REST_INTERVAL_SECONDS * 1000 && ATOM_WORKING) { 
        
        digitalWrite(ATOMIZER_RELAY_PIN, LOW); // turn off water atomizer
        ATOM_WORKING = false;
        
        Serial.println("Making a break on Atomizer - " + String(humidity) + " % humidity is bellow treshold (" + String(HUMIDITY_TRESHOLD) + ") :: " + String(TIME));
        RELAY_CHANGE = true;
    }

  } else if (ATOM_WORKING) {

    digitalWrite(ATOMIZER_RELAY_PIN, LOW); // turn off water atomizer
    ATOM_WORKING = false;

    Serial.println("Stopping Atomizer - " + String(humidity) + " % humidity is above or equal treshold (" + String(HUMIDITY_TRESHOLD) + ") :: " + String(TIME));
    RELAY_CHANGE = true;
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

    displayWeatherStationSensorData();

    getSupabaseInfo();

    // play God.
    adjustWeather();

    // update tresholds and fan/atom modes
    displayWeatherStationSensorData();
  }

  // update ThingSpeak
  if (TIME - UPDATE_TIME > UPDATE_INTERVAL_SECONDS * 1000 || RELAY_CHANGE) {
    updateThingSpeak();
    updateDatacake();
    UPDATE_TIME = TIME;
    RELAY_CHANGE = false;
  }

  // Go back to sleep. If your sensor is battery powered you might
  delay(1000 * READ_INTERVAL_SECONDS);
}
