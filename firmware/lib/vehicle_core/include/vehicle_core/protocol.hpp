#pragma once

#include <stddef.h>

namespace vehicle {

enum class CommandKind { None, Maze, Calibrate, Stop, Turn, Move, Unknown };

struct Command {
  CommandKind kind;
  int value;
  bool has_value;
};

enum class TelemetryKind { Unknown, Lidar, Compass, Rgb, Encoder, Error };

struct Telemetry {
  TelemetryKind kind;
  bool has_number;
  int number;
  char text[32];
};

// Line without the trailing newline. Empty and whitespace-only lines are None.
Command parse_command(const char* line);

// Canonical wire form: "maze", "calibrate", "stop", "turn:<deg>", "move:<cm>".
bool format_command(const Command& command, char* out, size_t out_len);

Telemetry parse_telemetry(const char* line);

bool format_telemetry_int(TelemetryKind kind, int value, char* out, size_t out_len);
bool format_telemetry_text(TelemetryKind kind, const char* text, char* out, size_t out_len);

}  // namespace vehicle
