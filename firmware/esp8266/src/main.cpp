#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include <stdio.h>
#include <string.h>

#include <vehicle_core/protocol.hpp>

#if !__has_include("secrets.h")
#error "Copy include/secrets.example.h to include/secrets.h and set the Wi-Fi credentials."
#endif
#include "secrets.h"

namespace {

constexpr uint32_t kBaud = 9600;
constexpr uint32_t kWifiRetryMs = 20000;
constexpr int kMaxMoveCm = 500;

ESP8266WebServer server(80);
char token[17] = "";
char rx[80] = {};
size_t rx_len = 0;
bool rx_overflow = false;
bool fs_ok = false;
bool wifi_up = false;
uint32_t wifi_attempt_ms = 0;
IPAddress last_ip;

int lidar_cm = 0;
bool have_lidar = false;
int compass_deg = 0;
bool compass_ok = false;
char color_name[32] = "black";
char enc_text[32] = "none";

bool same_text(const char* left, const char* right) {
  if (left == nullptr || right == nullptr) return false;
  return strcmp(left, right) == 0;
}

void make_token() {
  static const char* hex = "0123456789abcdef";
  for (int i = 0; i < 16; ++i) token[i] = hex[ESP.random() & 0x0f];
  token[16] = '\0';
}

bool authorized() {
  if (token[0] == '\0' || !server.hasHeader("Cookie")) return false;
  String cookie = server.header("Cookie");
  int at = cookie.indexOf("session=");
  if (at < 0) return false;
  String value = cookie.substring(at + 8, at + 8 + 16);
  return value.length() == 16 && value == token;
}

bool require_auth() {
  if (authorized()) return true;
  server.send(401, "text/plain", "Unauthorized");
  return false;
}

void send_file(const char* path, const char* type) {
  File file = LittleFS.open(path, "r");
  if (!file) {
    server.send(500, "text/plain", "Missing file");
    return;
  }
  server.streamFile(file, type);
  file.close();
}

bool send_command(const vehicle::Command& command) {
  char line[32];
  if (!vehicle::format_command(command, line, sizeof line)) return false;
  Serial.println(line);
  return true;
}

void handle_root() {
  if (authorized()) {
    server.sendHeader("Location", "/index");
    server.send(302, "text/plain", "Index");
    return;
  }
  send_file("/login.html", "text/html");
}

void handle_login() {
  if (!server.hasArg("username") || !server.hasArg("password")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  if (!same_text(server.arg("username").c_str(), AUTH_USER) ||
      !same_text(server.arg("password").c_str(), AUTH_PASSWORD)) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }
  make_token();
  char cookie[80];
  snprintf(cookie, sizeof cookie, "session=%s; Path=/; HttpOnly; SameSite=Lax", token);
  server.sendHeader("Set-Cookie", cookie);
  server.send(200, "text/plain", "ok");
}

void handle_logout() {
  token[0] = '\0';
  server.sendHeader("Set-Cookie", "session=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
  server.send(200, "text/plain", "ok");
}

void handle_index() {
  if (!authorized()) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Login");
    return;
  }
  send_file("/index.html", "text/html");
}

void handle_move() {
  if (!require_auth()) return;
  if (!server.hasArg("distance")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  int distance = server.arg("distance").toInt();
  if (distance <= 0 || distance > kMaxMoveCm) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  vehicle::Command command{vehicle::CommandKind::Move, distance, true};
  if (!send_command(command)) {
    server.send(500, "text/plain", "Format");
    return;
  }
  server.send(200, "text/plain", "ok");
}

void handle_turn() {
  if (!require_auth()) return;
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  int angle = server.arg("value").toInt();
  if (angle < 0 || angle > 360) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  vehicle::Command command{vehicle::CommandKind::Turn, angle, true};
  if (!send_command(command)) {
    server.send(500, "text/plain", "Format");
    return;
  }
  server.send(200, "text/plain", "ok");
}

void handle_simple(vehicle::CommandKind kind) {
  if (!require_auth()) return;
  vehicle::Command command{kind, 0, false};
  if (!send_command(command)) {
    server.send(500, "text/plain", "Format");
    return;
  }
  server.send(200, "text/plain", "ok");
}

void handle_maze() { handle_simple(vehicle::CommandKind::Maze); }
void handle_calibrate() { handle_simple(vehicle::CommandKind::Calibrate); }
void handle_stop() { handle_simple(vehicle::CommandKind::Stop); }

void handle_data() {
  if (!require_auth()) return;
  JsonDocument doc;
  if (have_lidar) doc["lidar"] = lidar_cm;
  doc["compass"] = compass_deg;
  doc["compassOk"] = compass_ok;
  doc["color"] = color_name;
  doc["enc"] = enc_text;
  String body;
  serializeJson(doc, body);
  server.send(200, "application/json", body);
}

void handle_not_found() { server.send(404, "text/plain", "Not found"); }

void copy_text(char* dest, size_t dest_len, const char* text) {
  if (dest_len == 0) return;
  snprintf(dest, dest_len, "%s", text == nullptr ? "" : text);
}

void apply_telemetry(const vehicle::Telemetry& message) {
  switch (message.kind) {
    case vehicle::TelemetryKind::Lidar:
      if (!message.has_number) return;
      lidar_cm = message.number;
      have_lidar = true;
      break;
    case vehicle::TelemetryKind::Compass:
      if (!message.has_number) return;
      compass_deg = message.number;
      compass_ok = true;
      break;
    case vehicle::TelemetryKind::Rgb:
      copy_text(color_name, sizeof color_name, message.text);
      break;
    case vehicle::TelemetryKind::Encoder:
      copy_text(enc_text, sizeof enc_text, message.text);
      break;
    case vehicle::TelemetryKind::Error:
      if (strcmp(message.text, "compass") == 0) compass_ok = false;
      break;
    case vehicle::TelemetryKind::Unknown:
      break;
  }
}

void read_mega() {
  while (Serial.available() > 0) {
    char next = static_cast<char>(Serial.read());
    if (next == '\r') continue;
    if (next != '\n') {
      if (rx_len + 1 < sizeof rx) rx[rx_len++] = next;
      else rx_overflow = true;
      continue;
    }
    rx[rx_len] = '\0';
    bool overflow = rx_overflow;
    rx_len = 0;
    rx_overflow = false;
    if (overflow || rx[0] == '\0') continue;
    vehicle::Telemetry message = vehicle::parse_telemetry(rx);
    if (message.kind == vehicle::TelemetryKind::Unknown) {
      Serial1.print("ignored ");
      Serial1.println(rx);
      continue;
    }
    apply_telemetry(message);
  }
}

void service_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    if (!wifi_up || ip != last_ip) {
      wifi_up = true;
      last_ip = ip;
      Serial1.print("IP ");
      Serial1.println(ip);
    }
    return;
  }

  wifi_up = false;
  uint32_t now = millis();
  if (wifi_attempt_ms != 0 && (now - wifi_attempt_ms) < kWifiRetryMs) return;
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifi_attempt_ms = now;
  Serial1.println("Wi-Fi retry");
}

void register_routes() {
  server.collectHeaders("Cookie");
  server.on("/", HTTP_GET, handle_root);
  server.on("/login", HTTP_POST, handle_login);
  server.on("/logout", HTTP_GET, handle_logout);
  server.on("/index", HTTP_GET, handle_index);
  server.on("/move", HTTP_POST, handle_move);
  server.on("/compass", HTTP_POST, handle_turn);
  server.on("/maze", HTTP_POST, handle_maze);
  server.on("/enc", HTTP_POST, handle_calibrate);
  server.on("/stop", HTTP_POST, handle_stop);
  server.on("/getData", HTTP_GET, handle_data);
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/script.js", LittleFS, "/script.js");
  server.serveStatic("/assets/logo.jpg", LittleFS, "/assets/logo.jpg");
  server.serveStatic("/assets/favicon.ico", LittleFS, "/assets/favicon.png");
  server.serveStatic("/assets/steering-wheel.svg", LittleFS, "/assets/steering-wheel.svg");
  server.serveStatic("/assets/road.svg", LittleFS, "/assets/road.svg");
  server.onNotFound(handle_not_found);
  server.begin();
}

}  // namespace

void setup() {
  Serial.begin(kBaud);
  Serial1.begin(kBaud);
  fs_ok = LittleFS.begin();
  if (!fs_ok) Serial1.println("LittleFS mount failed");

  WiFi.mode(WIFI_STA);
  WiFi.hostname("smart-vehicle");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifi_attempt_ms = millis();

  if (fs_ok) register_routes();
  else Serial1.println("Web server not started");
}

void loop() {
  service_wifi();
  if (fs_ok) server.handleClient();
  read_mega();
}
