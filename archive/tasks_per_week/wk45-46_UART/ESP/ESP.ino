/*
  ESP Steering wheel example 14.11.2019 KN
  Updated 2024 SV
*/

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h> // download and as .zip file
#include <ESPAsyncWebServer.h> // download and as .zip file
#include <FS.h>
#include <Wire.h>

// Network credentials
const char* ssid = "REDACTED";
const char* password = "REDACTED";

#define TESTFILE "/index.html"
bool spiffsActive = false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
  // Serial port for debugging purposes
  Serial.begin(9600);

  // Start filing subsystem
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS Active");
    spiffsActive = true;
  } else {
    Serial.println("Unable to activate SPIFFS");
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());


  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);

    int paramsNr = request->params();

    for (byte i = 0; i < paramsNr; i++){
      AsyncWebParameter* p = request->getParam(0);
      if ((p->name()) == "dist") {
        Serial.print("Move:");
        Serial.println(p->value());
      }
      if ((p->name()) == "dire") {
        Serial.print("Turn:");
        Serial.println(p->value());
      }
    }
  });


  // Route to load style.css file
  server.on("/jquery-1.11.3.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/jquery-1.11.3.min.js", "text/javascript");
    Serial.println(" JS jquery GET ");
  });

  // Route to load style.css file
  server.on("/roundslider.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/roundslider.min.js", "text/javascript");
    Serial.println(" min JS GET ");
  });

  // Route to load style.css file
  server.on("/roundslider.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/roundslider.min.css", "text/css");
    Serial.println(" min css GET ");
  });

  // Start server
  server.begin();
}

void loop(){

}


String processor(const String& var){
  return String();
}
