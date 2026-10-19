#include <ESP8266WiFi.h>        // Library for Wi-Fi functionality
#include <ESP8266WebServer.h>   // Library to create and manage a web server
#include <FS.h>                 // Library for working with file systems (FS)
//#include <LittleFS.h>           // Library for faster loading
#include <ArduinoJson.h>        // Library for safe JSON formatting
//#include <Crypto.h>
//#include <SHA256.h>

const char* ssid = "REDACTED";   // Wi-Fi network name (SSID)
const char* password = "REDACTED";    // Wi-Fi network password
//const char* ssid = "REDACTED";   // Wi-Fi network name (SSID)
//const char* password = "REDACTED";    // Wi-Fi network password
const char* auth_name = "REDACTED";      // Authentication username
const char* auth_pass = "REDACTED";      // Authentication password
bool isAuthenticated = false;

String lidarData = "0 cm";            // Variable to store LIDAR data
String compassData = "0";
String color = "black";

ESP8266WebServer server(80);    // Create an instance of the WebServer on port 80 (default HTTP port)

void checkCredentials() {
  if (server.hasArg("username") && server.hasArg("password")) {
    String username = server.arg("username");
    String password = server.arg("password");

    if (username == auth_name && password == auth_pass) {
      isAuthenticated = true;
//      server.sendHeader("Location", "/index", true);
      server.send(302, "text/plain", "Redirecting to /index...");
      return;
    }
  }

  server.send(401, "text/plain", "Unauthorized");
}

void serveIndex() {
  if (!isAuthenticated) {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Redirecting to login...");
    return;
  }
  File file = SPIFFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void logout() {
  isAuthenticated = false;
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "Redirecting to login...");
}

// Gets move command from webpage ("/forward?value=10", "backward?value=20", etc.)
void getMoveCommand() {
  if (server.hasArg("distance")) {                  // Check if there is a "distance" argument in the request URL
    String distance = server.arg("distance");    
    Serial.println("Move:" + distance);             // Send command back to Arduino
    server.send(200);
    return;
  }
  server.send(400, "text/plain", "Bad Request");
}

// Gets compass command from webpage (like "/compass?value=30")
void getCompassCommand() {
  if (server.hasArg("value")) {                     // Check if there is a "value" argument in the request URL
    String valueString = server.arg("value");       // Get the "value" argument from the URL
    Serial.println("Turn:" + valueString);          // Sends command back to Arduino
    server.send(200);
    return;
  }
  server.send(400, "text/plain", "Bad request");
}

// Sends all current data to webpage
void sendAllData() {
  JsonDocument doc;
  doc["compass"] = compassData;
  doc["lidar"] = lidarData;
  doc["color"] = color;

  String jsonData;
  serializeJson(doc, jsonData);

  server.send(200, "application/json", jsonData);
}

// This function is called when a non-existing URL is accessed (404 error)
void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found"); // Send a 404 response with a plain text message
}

void setup() {
  Serial.begin(9600);

  // Initialize the file system (SPIFFS) on the ESP8266
  if (!SPIFFS.begin()) {        // Initialize and check success on SPIFFS
    Serial.println("Error while mounting SPIFFS");
    return;
  }

  // Connect to the Wi-Fi network using the provided SSID and password
  Serial.print("\nConnecting to " + (String)ssid);
  WiFi.begin(ssid, password);               // Begin connecting to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {   // Wait in a loop until the connection is successful
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nIP: " + WiFi.localIP().toString()); // Print the IP address of the ESP8266 when connected

  server.serveStatic("/", SPIFFS, "/login.html");  
  server.on("/login", HTTP_POST, checkCredentials);
  server.on("/logout", HTTP_GET, logout);
  server.on("/index", HTTP_GET, serveIndex);
  server.serveStatic("/script.js", SPIFFS, "/script.js");
  server.serveStatic("/style.css", SPIFFS, "/style.css");   
  // Cache images for quicker load   
  server.serveStatic("/assets/logo.jpg", 
                      SPIFFS, 
                      "/assets/logo.jpg", 
                      "max-age=86400, must-revalidate");
  server.serveStatic("/assets/favicon.ico", 
                      SPIFFS, 
                      "/assets/favicon.png",
                      "max-age=86400, must-revalidate");
  server.serveStatic("/assets/steering-wheel.svg", 
                      SPIFFS, 
                      "/assets/steering-wheel.svg",
                      "max-age=86400, must-revalidate");
  server.serveStatic("/assets/road.svg", 
                      SPIFFS, 
                      "/assets/road.svg",
                      "max-age=86400, must-revalidate");
  
  // Receiving commands from webpage
  server.on("/move", HTTP_POST, getMoveCommand);
  server.on("/compass", HTTP_POST, getCompassCommand);

  // Sending info to webpage
  server.on("/getData", HTTP_GET, sendAllData);

  // Handles unknown urls
  server.onNotFound(handleNotFound);

  // Start the web server
  server.begin();
}

void loop() {
  server.handleClient();  // Listen for incoming HTTP requests and respond to them
 
  // Check if data is available from the Arduino Mega
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    if (data.startsWith("LIDAR:")) {
      lidarData = data.substring(6);
    } else if (data.startsWith("COMP:")){
      compassData = data.substring(5);
    } else if (data.startsWith("RGB:")) {
      color = data.substring(4);
    } else {
      Serial.println("Command not found:" + String(data));
    }
  }

}
