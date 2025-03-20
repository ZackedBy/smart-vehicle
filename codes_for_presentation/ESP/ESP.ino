#include <ESP8266WiFi.h>        // Library for Wi-Fi functionality
#include <ESP8266WebServer.h>   // Library to create and manage a web server
#include <FS.h>                 // Library for working with file systems (SPIFFS)

//const char* ssid = "Titenet-IoT";   // Wi-Fi network name (SSID)
//const char* password = "7kDtaphg";    // Wi-Fi network password
const char* ssid = "OnePlus 7 Pro";   // Wi-Fi network name (SSID)
const char* password = "77777771";    // Wi-Fi network password
const char* auth_name = "admin";      // Authentication username
const char* auth_pass = "admin";      // Authentication password
bool isAuthenticated = false;

String lidarData = "0 cm";            // Variable to store LIDAR data
String compassData = "0";
String color = "black";

ESP8266WebServer server(80);    // Create an instance of the WebServer on port 80 (default HTTP port)

//void handleAuth() {
//  if (!server.authenticate(auth_name, auth_pass)) {
//    return server.requestAuthentication();
//  } 
//  
//  // Manually serve the HTML file after successful authentication
//  File file = SPIFFS.open("/index.html", "r");  // Open the index.html file from SPIFFS
//  if (!file) {
//    server.send(500, "text/plain", "Failed to open file.");
//    return;
//  }
//
//  // Stream the file to the client
//  server.streamFile(file, "text/html");
//  file.close();  // Don't forget to close the file after sending it
//}
 
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



  // Maybe move these to handleLogin?
  server.serveStatic("/index", SPIFFS, "/index.html");
  server.serveStatic("/", SPIFFS, "/login.html");
  server.serveStatic("/style.css", SPIFFS, "/style.css");     // Serve the CSS file for styling
  server.serveStatic("/script.js", SPIFFS, "/script.js");     // Serve the JavaScript file
  server.serveStatic("/assets/favicon.ico", SPIFFS, "/assets/favicon.ico");
  server.serveStatic("/assets/wheel.png", SPIFFS, "/assets/wheel.png");
  server.serveStatic("/assets/pedal.jpg", SPIFFS, "/assets/pedal.jpg");
  server.serveStatic("/assets/road.jpg", SPIFFS, "/assets/road.jpg");
  server.serveStatic("/assets/JIT-logo2.jpg", SPIFFS, "/assets/JIT-logo2.jpg");
  
//  server.on("/style.css", HTTP_GET, handleSecureFileRequest);
//  server.on("/script.js", HTTP_GET, handleSecureFileRequest);
//  server.on("/index", HTTP_GET, handleSecureFileRequest, supplyStyle);
//  server.on("/assets/favicon.ico", HTTP_GET, handleSecureFileRequest); // Serve a favicon (small icon) for the website
//  server.on("/assets/wheel.png", HTTP_GET, handleSecureFileRequest);
//  server.on("/assets/pedal.jpg", HTTP_GET, handleSecureFileRequest);
//  server.on("/assets/road.jpg", HTTP_GET, handleSecureFileRequest);


//  server.on("/", HTTP_GET, checkCredentials);
  server.on("/login", HTTP_POST, checkCredentials);
  server.on("/logout", logout);
  
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

  /**
   * TO-DO:
   * Look into ArduinoOTA for easy 02A authentication
   */

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

//void handleAuth() {
//  if (checkCredentials) {
//    server.sendHeader("Location", "/index", true); // Redirect to /
//    server.send(200, "text/plain", "test1"); // HTTP 302 Found (Redirect)
//  } else {
//    server.sendHeader("Location", "/", true); // Redirect to /login
//    server.send(200, "text/plain", "test2"); 
//  }
//}
bool handleFileRead(String path) {
    if (path.endsWith("/")) path += "index.html"; // Default file
    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".png")) contentType = "image/png";
    else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) contentType = "image/jpeg";
    
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void handleSecureFileRequest() {
  if (!checkCredentials()) return;
  if (!handleFileRead(server.uri())) {
    server.send(404, "text/plain", "File not found.");
  }
}

void supplyStyle() {
    File css = SPIFFS.open("/style.css", "r");
    server.streamFile(css, "text/css");
    css.close();

    File js = SPIFFS.open("/script.js", "r");
    server.streamFile(js, "application/javascript");
    js.close();
}

void logout() {
  isAuthenticated = false;
  server.send(200, "text/plain", "Logging out");
}

boolean checkCredentials() {
  if (server.hasArg("username") && server.hasArg("password")) {
    String username = server.arg("username");
    String password = server.arg("password");

    if (username == auth_name && password == auth_pass) {
      isAuthenticated = true;
      server.send(200, "text/plain", "Redirecting...");
      return true;
    }
  }
  if (isAuthenticated) {
    server.send(200);
    return true;
  }
  
  server.send(401, "text/plain", "Unauthorized");
  return false;
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
