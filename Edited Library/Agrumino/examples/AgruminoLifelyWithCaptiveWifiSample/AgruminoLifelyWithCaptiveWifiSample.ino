/*
  AgruminoCaptiveWiSample.ino - Sample project for Agrumino board using the Agrumino library.
  Created by giuseppe.broccia@lifely.cc on October 2017.

  @see Agrumino.h for the documentation of the lib
*/
#include "Agrumino.h"           // Our super cool lib ;)
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <DNSServer.h>          // Installed from ESP8266 board
#include <ESP8266WebServer.h>   // Installed from ESP8266 board
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson

// Time to sleep in second between the readings/data sending
#define SLEEP_TIME_SEC 30


// Web Server data, in our sample we use Dweet.io.
const char* WEB_SERVER_HOST = "api.lifely.cc";
const String WEB_SERVER_API_REGISTER = "/api/v1/objects/device_token/";
const String WEB_SERVER_API_SEND_DATA = "/api/v1/objects/device_observation/";

// Our super cool lib
Agrumino agrumino;

// Used for sending Json POST requests
StaticJsonBuffer<500> jsonBuffer;
// Used to create TCP connections and make Http calls
WiFiClient client;

void setup() {

  Serial.begin(115200);

  // Setup our super cool lib
  agrumino.setup();

  // Turn on the board to allow the usage of the Led
  agrumino.turnBoardOn();


  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // If the S1 button is pressed for 5 seconds then reset the wifi saved settings.
  if (checkIfResetWiFiSettings()) {
    wifiManager.resetSettings();
    Serial.println("Reset WiFi Settings Done!");
    // Blink led for confirmation :)
    blinkLed(100, 10);
  }

  // Set timeout until configuration portal gets turned off
  // useful to make it all retry or go to sleep in seconds
  wifiManager.setTimeout(180); // 3 minutes

  // Customize the web configuration web page
  wifiManager.setCustomHeadElement("<h1>Agrumino</h1>");

  // Fetches ssid and pass and tries to connect
  // If it does not connect it starts an access point with the specified name here
  // and goes into a blocking loop awaiting configuration
  String ssidAP = "Agrumino-AP-" + getChipId();
  if (!wifiManager.autoConnect(ssidAP.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    // Reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  // If you get here you have connected to the WiFi :D
  Serial.println("\nConnected to WiFi ...yeey :)\n");
}

void loop() {
  Serial.println("#########################\n");

  agrumino.turnBoardOn();

  float temperature =         agrumino.readTempC();
  unsigned int soilMoisture = agrumino.readSoil();
  float illuminance =         agrumino.readLux();
  float batteryVoltage =      agrumino.readBatteryVoltage();
  unsigned int batteryLevel = agrumino.readBatteryLevel();
  boolean isAttachedToUSB =   agrumino.isAttachedToUSB();
  boolean isBatteryCharging = agrumino.isBatteryCharging();

  Serial.println("temperature:       " + String(temperature) + "°C");
  Serial.println("soilMoisture:      " + String(soilMoisture) + "%");
  Serial.println("illuminance :      " + String(illuminance) + " lux");
  Serial.println("batteryVoltage :   " + String(batteryVoltage) + " V");
  Serial.println("batteryLevel :     " + String(batteryLevel) + "%");
  Serial.println("isAttachedToUSB:   " + String(isAttachedToUSB));
  Serial.println("isBatteryCharging: " + String(isBatteryCharging));
  Serial.println();

  // Change this if you whant to change your thing name
  // We use the chip Id to avoid name clashing
  String thingName = "Agrumino-" + getChipId();

  // Send data to our web service
  sendData(thingName, temperature, soilMoisture, illuminance, batteryVoltage, batteryLevel, isAttachedToUSB, isBatteryCharging);

  // Blink when the business is done for giving an Ack to the user
  blinkLed(200, 1);

  // Board off before delay/sleep to save battery :)
  agrumino.turnBoardOff();

  // delaySec(SLEEP_TIME_SEC); // The ESP8266 stays powered, executes the loop repeatedly
  deepSleepSec(SLEEP_TIME_SEC); // ESP8266 enter in deepSleep and after the selected time starts back from setup() and then loop()
}

//////////////////
// HTTP methods //
//////////////////

void sendData(String thingName, float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge) {

  Serial.println("##################################");
  Serial.println("### Your Lifely thing name is ###");
  Serial.println("###  --> " + thingName + " <--  ###");
  Serial.println("##################################\n");

  // TODO: Call registerDevice one time and save token in eeprom
  String registerBody = getRegisterBodyJsonString(thingName);
  doPostApiCall(WEB_SERVER_HOST, WEB_SERVER_API_REGISTER , registerBody);

  // This shoul be read from the register API response
  String deviceToken = "TODO";

  // TODO: Send a real token when the backend will use it
  String sendDataBody = getSendDataBodyJsonString(thingName, deviceToken, temp,  soil,  lux,  batt, battLevel, usb, charge);
  doPostApiCall(WEB_SERVER_HOST, WEB_SERVER_API_SEND_DATA , sendDataBody);
}

void doPostApiCall(const char* host, String api, String bodyJsonString) {

  // Use WiFiClient class to create TCP connections, we try until the connection is estabilished
  while (!client.connect(host, 80)) {
    Serial.println("connection failed\n");
    delay(1000);
  }
  Serial.println("connected to " + String(host) + " ...yeey :)\n");

  // Print the HTTP POST API data for debug
  Serial.println("Requesting POST: " + String(host) + api);
  Serial.println("Requesting POST: " + bodyJsonString);

  // This will send the request to the server
  client.println("POST " + api + " HTTP/1.1");
  client.println("Host: " + String(host) + ":80");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(bodyJsonString.length()));
  client.println();
  client.println(bodyJsonString);

  delay(10);

  int timeout = 300; // 100 ms per loop so 30 sec.
  while (!client.available()) {
    // Waiting for server response
    delay(100);
    Serial.print(".");
    timeout--;
    if (timeout <= 0) {
      Serial.print("Err. client.available() timeout reached!");
      return;
    }
  }

  String response = "";

  while (client.available() > 0) {
    char c = client.read();
    response = response + c;
  }

  // Remove bad chars from response
  response.trim();
  response.replace("/n", "");
  response.replace("/r", "");

  Serial.println("\nAPI Update successful! Response: \n");
  Serial.println(response);
}

// Returns the Json body that will be sent to the register device HTTP POST API
String getRegisterBodyJsonString(String deviceKey) {
  JsonObject& jsonPost = jsonBuffer.createObject();
  jsonPost["key"] = String(deviceKey);

  String jsonPostString;
  jsonPost.printTo(jsonPostString);

  return jsonPostString;
}

// Returns the Json body that will be sent to the send data HTTP POST API
String getSendDataBodyJsonString(String deviceKey, String deviceToken, float temp, int soil, unsigned int lux, float batt, unsigned int battLevel, boolean usb, boolean charge) {
  JsonObject& jsonPost = jsonBuffer.createObject();

  jsonPost["device_key"] = String(deviceKey);
  jsonPost["device_token"] = String(deviceKey);
  jsonPost["temperature"] = String(temp);
  jsonPost["humidity"] = String(soil);
  jsonPost["battery"] = String(battLevel);
  jsonPost["lum"]  = String(lux);
  // jsonPost["water"]  = Not present

  // Present but not supported yet by the backend
  jsonPost["battVolt"] = String(batt);
  jsonPost["battCharging"] = String(charge);
  jsonPost["usbConnected"]  = String(usb);



  String jsonPostString;
  jsonPost.printTo(jsonPostString);

  return jsonPostString;
}


/////////////////////
// Utility methods //
/////////////////////

void blinkLed(int duration, int blinks) {
  for (int i = 0; i < blinks; i++) {
    agrumino.turnLedOn();
    delay(duration);
    agrumino.turnLedOff();
    if (i < blinks) {
      delay(duration); // Avoid delay in the latest loop ;)
    }
  }
}

void delaySec(int sec) {
  delay (sec * 1000);
}

void deepSleepSec(int sec) {
  Serial.print("\nGoing to deepSleep for ");
  Serial.print(sec);
  Serial.println(" seconds... (ー。ー) zzz\n");
  ESP.deepSleep(sec * 1000000); // microseconds
}

const String getChipId() {
  // Returns the ESP Chip ID, Typical 7 digits
  return String(ESP.getChipId());
}

// If the Agrumino S1 button is pressed for 5 seconds then reset the wifi saved settings.
boolean checkIfResetWiFiSettings() {

  int remainingsLoops = (5 * 1000) / 100;

  Serial.print("Check if reset WiFi settings: ");

  while (agrumino.isButtonPressed() && remainingsLoops > 0) {
    delay(100);
    remainingsLoops--;
    Serial.print(".");
  }

  if (remainingsLoops == 0) {
    // Reset Wifi Settings
    Serial.println(" YES!");
    return true;
  } else {
    Serial.println(" NO");
    return false;
  }
}
