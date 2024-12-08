#include <ESP8266WiFi.h>        // Library for Wi-Fi functionality
#include <ESP8266WebServer.h>   // Library to create and manage a web server
#include <FS.h>                 // Library for working with file systems (SPIFFS)

const char* ssid = "SOURCE";         // Wi-Fi network name (SSID)
const char* password = "Pelle!23";  // Wi-Fi network password

ESP8266WebServer server(80);    // Create an instance of the WebServer on port 80 (default HTTP port)

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

  // Set up the web pages (URLS) and files that the server will show/use when someone visits the site
  server.serveStatic("/", SPIFFS, "/index.html");             // Serves and shows the main webpage (HTML file) when someone visits the home page
  server.serveStatic("/style.css", SPIFFS, "/style.css");     // Serve the CSS file for styling
  server.serveStatic("/script.js", SPIFFS, "/script.js");     // Serve the JavaScript file
  server.serveStatic("/favicon.ico", SPIFFS, "/favicon.png"); // Serve a favicon (small icon) for the website


  // Define custom actions/function calls for specific URLs (ie when a button press from webpage requests a particular URL)
  server.on("/forwards", handleMove);
  server.on("/backwards", handleMove);
  server.on("/compass", handleCompass);
  server.onNotFound(handleNotFound);
  
  // Start the web server
  server.begin();
}

void loop() {
  server.handleClient();  // Listen for incoming HTTP requests and respond to them
}



// Function handles movement ("/forward?value=10", "backward?value=20", etc.)
void handleMove() {
  if (server.hasArg("value")) {                     // Check if there is a "value" argument in the request URL
    String valueString = server.arg("value");       // Get the "value" argument from the URL
    int distance = valueString.toInt();
    if (server.uri().startsWith("/backwards")) {
      distance = -distance;                         // Negate for backwards movement
    }
    Serial.println("Move:" + String(distance)); 
  }
  server.send(200);
}

// This function handles the compass command (like "/compass?value=30")
void handleCompass() {
  if (server.hasArg("value")) {                     // Check if there is a "value" argument in the request URL
    String valueString = server.arg("value");       // Get the "value" argument from the URL
    Serial.println("Turn:" + valueString);          // Print the compass value to the serial monitor
  }
  server.send(200);
}

// This function is called when a non-existing URL is accessed (404 error)
void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found"); // Send a 404 response with a plain text message
}
