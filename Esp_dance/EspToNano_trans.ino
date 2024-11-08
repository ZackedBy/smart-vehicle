#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "RedmiNote11";         //name of WIFI
const char* password = "1234567890";      // password

AsyncWebServer server(80);                 // making a server on port 80
HardwareSerial NanoSerial(1);              // Serial for connection with Arduino Nano on TX and RX

void setup() 
{
  Serial.begin(115200);
  NanoSerial.begin(9600, SERIAL_8N1, 16, 17);  //  Starting Serial for Nano
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)       //check connection
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  //main page with CSS and AJAX
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/html",
      "<html><head><style>"
      "body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f9; color: #333; }"
      "h1 { color: #007acc; }"
      "button { padding: 10px 20px; margin: 10px; border: none; color: white; background-color: #007acc; border-radius: 5px; cursor: pointer; }"
      "button:hover { background-color: #005f8c; }"
      "</style>"
      "<script>"
      "function sendCommand(command) {"
      "  var xhr = new XMLHttpRequest();"
      "  xhr.open('GET', '/send/' + command, true);"
      "  xhr.onload = function() {"
      "    if (xhr.status === 200)    { "
      "      document.getElementById('response').innerHTML = xhr.responseText;"
      "    }"
      "  };"
      "  xhr.send();"
      "}"
      "</script>"
      "</head>"
      "<body><h1>ESP32 LED Control</h1>"
      "<button onclick=\"sendCommand('on')\">Turn LED On</button>"
      "<button onclick=\"sendCommand('off')\">Turn LED Off</button>"
      "<div id='response'></div>"
      "</body></html>"
    );
  });

  // URL for sendind "ON" to Arduino
  server.on("/send/on", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    NanoSerial.write('1');  // Sending '1' as turning-on command
    request->send(200, "text/plain", "LED is ON");
  });

  // URL for sendind "OFF" to Arduino
  server.on("/send/off", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    NanoSerial.write('0');  // Sending '0' as turning-off command
    request->send(200, "text/plain", "LED is OFF");
  });

  server.begin();
  Serial.println("Server started");
  Serial.println("Open http://" + WiFi.localIP().toString() + " in your browser");
}

void loop() 
{

}
