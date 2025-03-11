#include <ESP8266WiFi.h>        // Library for Wi-Fi functionality
#include <ESP8266WebServer.h>   // Library to create and manage a web server
#include <FS.h>                 // Library for working with file systems (SPIFFS)

const char* ssid = "OnePlus 7 Pro";          // Wi-Fi network name (SSID)
const char* password = "77777771";    // Wi-Fi network password
String lidarData = "0 cm";            // Variable to store LIDAR data
String compassData = "0";
String color = "black";

ESP8266WebServer server(80);    // Create an instance of the WebServer on port 80 (default HTTP port)

void setup() {
  Serial.begin(9600);
  //Serial.setTimeout(500);

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

  // Set up the web pages (URLS) and files that the server will show/use when someone visits the site
  server.serveStatic("/", SPIFFS, "/index.html");             // Serves and shows the main webpage (HTML file) when someone visits the home page
  server.serveStatic("/style.css", SPIFFS, "/style.css");     // Serve the CSS file for styling
  server.serveStatic("/script.js", SPIFFS, "/script.js");     // Serve the JavaScript file
  server.serveStatic("/assets/favicon.ico", SPIFFS, "/assets/favicon.png"); // Serve a favicon (small icon) for the website
  server.serveStatic("/assets/wheel.png", SPIFFS, "/assets/wheel.png");
  server.serveStatic("/assets/pedal.jpg", SPIFFS, "/assets/pedal.jpg");
  server.serveStatic("/assets/road.jpg", SPIFFS, "/assets/road.jpg");
  
  // Receiving commands from webpage
  server.on("/forwards", getMoveCommand);
  server.on("/backwards", getMoveCommand);
  server.on("/compass", getCompassCommand);

  // Sending info to webpage
  server.on("/realComp", sendCompass);
  server.on("/lidar", sendLidar);
  server.on("/rgb", sendColor);

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

// Gets move command from webpage ("/forward?value=10", "backward?value=20", etc.)
void getMoveCommand() {
  if (server.hasArg("value")) {                     // Check if there is a "value" argument in the request URL
    String valueString = server.arg("value");       // Get the "value" argument from the URL
    int distance = valueString.toInt();
    if (server.uri().startsWith("/backwards")) {
      distance = -distance;                         // Negate for backwards movement
    }
    Serial.println("Move:" + String(distance));
    server.send(200);
  }
}

// Gets compass command from webpage (like "/compass?value=30")
void getCompassCommand() {
  if (server.hasArg("value")) {                     // Check if there is a "value" argument in the request URL
    String valueString = server.arg("value");       // Get the "value" argument from the URL
    Serial.println("Turn:" + valueString);          // Sends command back to Arduino
    server.send(200);
  }
}

// Sends compass degrees to webpage ("/realComp")
void sendCompass() {
   server.send(200, "text/plain", compassData);
}

// Sends lidar distance (cm) to webpage ("/lidar")
void sendLidar() {
  server.send(200, "text/plain", lidarData); // Send the latest Lidar value with a 200 OK status.
}

// Sends color string to webpage ("/rgb")
void sendColor() {
  server.send(200, "text/plain", color);
}

// This function is called when a non-existing URL is accessed (404 error)
void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found"); // Send a 404 response with a plain text message
}
