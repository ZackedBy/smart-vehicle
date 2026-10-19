#include "vehicle_core/protocol.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace vehicle {
namespace {

bool is_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

char lower(char c) {
  if (c >= 'A' && c <= 'Z') return static_cast<char>(c - 'A' + 'a');
  return c;
}

bool equals_ci(const char* text, const char* literal) {
  while (*literal != '\0') {
    if (lower(*text) != *literal) return false;
    ++text;
    ++literal;
  }
  return *text == '\0';
}

// Copies a trimmed line into dest. Returns false when the line does not fit.
bool trim_copy(const char* line, char* dest, size_t dest_len) {
  if (line == nullptr || dest_len == 0) return false;
  while (*line != '\0' && is_space(*line)) ++line;
  size_t len = strlen(line);
  while (len > 0 && is_space(line[len - 1])) --len;
  if (len + 1 > dest_len) return false;
  memcpy(dest, line, len);
  dest[len] = '\0';
  return true;
}

bool parse_int(const char* text, int* value) {
  if (text == nullptr || *text == '\0') return false;
  char* end = nullptr;
  long parsed = strtol(text, &end, 10);
  if (end == text || *end != '\0') return false;
  if (parsed > 1000000L || parsed < -1000000L) return false;
  *value = static_cast<int>(parsed);
  return true;
}

const char* telemetry_prefix(TelemetryKind kind) {
  switch (kind) {
    case TelemetryKind::Lidar:
      return "LIDAR";
    case TelemetryKind::Compass:
      return "COMP";
    case TelemetryKind::Rgb:
      return "RGB";
    case TelemetryKind::Encoder:
      return "ENC";
    case TelemetryKind::Error:
      return "ERR";
    case TelemetryKind::Unknown:
      return nullptr;
  }
  return nullptr;
}

void set_text(Telemetry* message, const char* text) {
  message->text[0] = '\0';
  if (text == nullptr) return;
  snprintf(message->text, sizeof message->text, "%s", text);
}

}  // namespace

Command parse_command(const char* line) {
  Command command{CommandKind::None, 0, false};
  char buffer[64];
  if (!trim_copy(line, buffer, sizeof buffer)) {
    if (line != nullptr && line[0] != '\0') command.kind = CommandKind::Unknown;
    return command;
  }
  if (buffer[0] == '\0') return command;

  char* colon = strchr(buffer, ':');
  char* payload = nullptr;
  if (colon != nullptr) {
    *colon = '\0';
    payload = colon + 1;
  }

  if (equals_ci(buffer, "maze") && payload == nullptr) {
    command.kind = CommandKind::Maze;
    return command;
  }
  if (equals_ci(buffer, "calibrate") && payload == nullptr) {
    command.kind = CommandKind::Calibrate;
    return command;
  }
  if (equals_ci(buffer, "stop") && payload == nullptr) {
    command.kind = CommandKind::Stop;
    return command;
  }

  int value = 0;
  if (payload != nullptr && parse_int(payload, &value) &&
      (equals_ci(buffer, "turn") || equals_ci(buffer, "move"))) {
    command.kind = equals_ci(buffer, "turn") ? CommandKind::Turn : CommandKind::Move;
    command.value = value;
    command.has_value = true;
    return command;
  }

  command.kind = CommandKind::Unknown;
  return command;
}

bool format_command(const Command& command, char* out, size_t out_len) {
  if (out == nullptr || out_len == 0) return false;
  int wrote = -1;
  switch (command.kind) {
    case CommandKind::Maze:
      wrote = snprintf(out, out_len, "maze");
      break;
    case CommandKind::Calibrate:
      wrote = snprintf(out, out_len, "calibrate");
      break;
    case CommandKind::Stop:
      wrote = snprintf(out, out_len, "stop");
      break;
    case CommandKind::Turn:
      if (!command.has_value) return false;
      wrote = snprintf(out, out_len, "turn:%d", command.value);
      break;
    case CommandKind::Move:
      if (!command.has_value) return false;
      wrote = snprintf(out, out_len, "move:%d", command.value);
      break;
    case CommandKind::None:
    case CommandKind::Unknown:
      return false;
  }
  return wrote > 0 && static_cast<size_t>(wrote) < out_len;
}

Telemetry parse_telemetry(const char* line) {
  Telemetry message{TelemetryKind::Unknown, false, 0, {0}};
  char buffer[80];
  if (!trim_copy(line, buffer, sizeof buffer)) return message;

  char* colon = strchr(buffer, ':');
  if (colon == nullptr || colon == buffer) return message;
  *colon = '\0';
  const char* payload = colon + 1;

  if (equals_ci(buffer, "lidar")) message.kind = TelemetryKind::Lidar;
  else if (equals_ci(buffer, "comp")) message.kind = TelemetryKind::Compass;
  else if (equals_ci(buffer, "rgb")) message.kind = TelemetryKind::Rgb;
  else if (equals_ci(buffer, "enc")) message.kind = TelemetryKind::Encoder;
  else if (equals_ci(buffer, "err")) message.kind = TelemetryKind::Error;
  else return message;

  set_text(&message, payload);
  int value = 0;
  if (parse_int(payload, &value)) {
    message.has_number = true;
    message.number = value;
  }
  return message;
}

bool format_telemetry_int(TelemetryKind kind, int value, char* out, size_t out_len) {
  const char* prefix = telemetry_prefix(kind);
  if (prefix == nullptr || out == nullptr || out_len == 0) return false;
  int wrote = snprintf(out, out_len, "%s:%d", prefix, value);
  return wrote > 0 && static_cast<size_t>(wrote) < out_len;
}

bool format_telemetry_text(TelemetryKind kind, const char* text, char* out, size_t out_len) {
  const char* prefix = telemetry_prefix(kind);
  if (prefix == nullptr || text == nullptr || out == nullptr || out_len == 0) return false;
  if (strchr(text, '\n') != nullptr || strchr(text, '\r') != nullptr) return false;
  int wrote = snprintf(out, out_len, "%s:%s", prefix, text);
  return wrote > 0 && static_cast<size_t>(wrote) < out_len;
}

}  // namespace vehicle
