#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "RedmiNote11";          // Your network SSID (name)
const char* password = "1234567890";  // Your network password

const int ledPin = 12;                   // LED connected to digital pin 12
AsyncWebServer server(80);               // Create server object on port 80

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);               // Set LED pin as output
  digitalWrite(ledPin, LOW);             // Ensure the LED is off initially

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Route to control the LED
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<h1>ESP32 LED Control</h1><p><a href=\"/on\">Turn LED On</a></p><p><a href=\"/off\">Turn LED Off</a></p>");
  });

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);          // Turn LED on
    request->send(200, "text/html", "<p>LED is ON</p><p><a href=\"/off\">Turn LED Off</a></p>");
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);           // Turn LED off
    request->send(200, "text/html", "<p>LED is OFF</p><p><a href=\"/on\">Turn LED On</a></p>");
  });

  // Start server
  server.begin();
  Serial.println("Server started");
  Serial.println("Open http://" + WiFi.localIP().toString() + " in your browser");
}

void loop() {
  // No code needed in loop as AsyncWebServer handles everything asynchronously
}
