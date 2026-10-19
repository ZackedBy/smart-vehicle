#include "vehicle_core/color.hpp"
#include "vehicle_core/control.hpp"
#include "vehicle_core/encoder.hpp"
#include "vehicle_core/heading.hpp"
#include "vehicle_core/protocol.hpp"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {

int g_failures = 0;

void expect(bool condition, const char* name) {
  if (condition) return;
  fprintf(stderr, "FAIL %s\n", name);
  ++g_failures;
}

void test_heading() {
  expect(vehicle::heading_from_raw(0) == 0, "raw 0");
  expect(vehicle::heading_from_raw(255) == 0, "raw 255 wraps to 0");
  expect(vehicle::heading_from_raw(128) == 180, "raw 128");

  expect(vehicle::normalize_heading(360) == 0, "normalize 360");
  expect(vehicle::normalize_heading(-10) == 350, "normalize negative");
  expect(vehicle::apply_offset(10, 20) == 350, "offset across zero");
  expect(vehicle::apply_offset(90, 90) == 0, "offset cancels");

  expect(vehicle::shortest_diff(10, 350) == -20, "short turn westward");
  expect(vehicle::shortest_diff(350, 10) == 20, "short turn eastward");
  expect(vehicle::shortest_diff(0, 180) == 180, "half turn stays positive");
  expect(vehicle::shortest_diff(0, 181) == -179, "just past half turn");
  expect(vehicle::shortest_diff(20, 20) == 0, "already there");

  expect(vehicle::within_tolerance(2, 2), "tolerance inclusive");
  expect(!vehicle::within_tolerance(3, 2), "tolerance exclusive outside");

  expect(strcmp(vehicle::cardinal(0), "N") == 0, "cardinal 0");
  expect(strcmp(vehicle::cardinal(21), "N") == 0, "cardinal 21");
  expect(strcmp(vehicle::cardinal(22), "NE") == 0, "cardinal 22");
  expect(strcmp(vehicle::cardinal(337), "N") == 0, "cardinal 337");
  expect(strcmp(vehicle::cardinal(359), "N") == 0, "cardinal 359");
  expect(strcmp(vehicle::cardinal(-15), "N") == 0, "cardinal wrapped");
}

void test_control() {
  expect(vehicle::time_elapsed(0xfffffff0u, 0x20u, 48), "millis wrap elapsed");
  expect(!vehicle::time_elapsed(0xfffffff0u, 0x20u, 49), "millis wrap not yet");
  expect(vehicle::pwm_percent(255, 50) == 127, "half pwm");
  expect(vehicle::pwm_percent(255, 35) == 89, "blue pwm");
  expect(vehicle::pwm_percent(255, 75) == 191, "green pwm");
  expect(vehicle::pwm_percent(255, -5) == 0, "pwm clamp low");
  expect(vehicle::pwm_percent(255, 140) == 255, "pwm clamp high");
  expect(vehicle::rotation_pwm(11, 255, 10) == 255, "fast rotation");
  expect(vehicle::rotation_pwm(-11, 255, 10) == 255, "fast rotation either way");
  expect(vehicle::rotation_pwm(10, 255, 10) == 63, "slow band inclusive");
}

void test_color() {
  const vehicle::Rgb red{100, 10, 10};
  const vehicle::Rgb green{10, 100, 10};
  const vehicle::Rgb blue{10, 10, 100};
  expect(vehicle::color_matches(red, {110, 10, 10}, 10), "match on threshold");
  expect(!vehicle::color_matches(red, {112, 10, 10}, 10), "miss past threshold");

  vehicle::ColorCalibration calibration{red, green, blue, true};
  expect(vehicle::classify_color({100, 10, 10}, calibration, 10) == vehicle::ColorName::Red,
         "classify red");
  expect(vehicle::classify_color({10, 100, 10}, calibration, 10) == vehicle::ColorName::Green,
         "classify green");
  expect(vehicle::classify_color({10, 10, 100}, calibration, 10) == vehicle::ColorName::Blue,
         "classify blue");
  expect(vehicle::classify_color({0, 0, 0}, calibration, 10) == vehicle::ColorName::Black,
         "classify none");

  vehicle::ColorCalibration overlap{red, red, blue, true};
  expect(vehicle::classify_color(red, overlap, 10) == vehicle::ColorName::Red,
         "red wins a tie");

  vehicle::ColorCalibration incomplete{red, green, blue, false};
  expect(vehicle::classify_color(red, incomplete, 10) == vehicle::ColorName::Black,
         "uncalibrated stays black");
  expect(strcmp(vehicle::color_name(vehicle::ColorName::Green), "green") == 0, "color name");
}

void expect_command(const char* line, vehicle::CommandKind kind, int value, bool has_value,
                    const char* name) {
  vehicle::Command command = vehicle::parse_command(line);
  expect(command.kind == kind && command.value == value && command.has_value == has_value, name);
}

void expect_roundtrip(vehicle::CommandKind kind, int value, bool has_value, const char* name) {
  char wire[32];
  vehicle::Command command{kind, value, has_value};
  expect(vehicle::format_command(command, wire, sizeof wire), name);
  vehicle::Command parsed = vehicle::parse_command(wire);
  expect(parsed.kind == kind && parsed.value == value && parsed.has_value == has_value, name);
}

void test_protocol() {
  expect_command("maze", vehicle::CommandKind::Maze, 0, false, "maze");
  expect_command("  MAZE\r", vehicle::CommandKind::Maze, 0, false, "maze case and space");
  expect_command("calibrate", vehicle::CommandKind::Calibrate, 0, false, "calibrate");
  expect_command("stop", vehicle::CommandKind::Stop, 0, false, "stop");
  expect_command("turn:90", vehicle::CommandKind::Turn, 90, true, "turn");
  expect_command("TURN:15", vehicle::CommandKind::Turn, 15, true, "turn case");
  expect_command("move:20", vehicle::CommandKind::Move, 20, true, "move");
  expect_command("move:0", vehicle::CommandKind::Move, 0, true, "move zero stays a command");
  expect_command("", vehicle::CommandKind::None, 0, false, "empty");
  expect_command("   ", vehicle::CommandKind::None, 0, false, "blank");
  expect_command("nope", vehicle::CommandKind::Unknown, 0, false, "unknown");
  expect_command("turn:", vehicle::CommandKind::Unknown, 0, false, "turn missing value");
  expect_command("turn:abc", vehicle::CommandKind::Unknown, 0, false, "turn junk");
  expect_command("turn:1.5", vehicle::CommandKind::Unknown, 0, false, "turn float");
  expect_command("maze:1", vehicle::CommandKind::Unknown, 0, false, "maze rejects a value");

  expect_roundtrip(vehicle::CommandKind::Maze, 0, false, "wire maze");
  expect_roundtrip(vehicle::CommandKind::Calibrate, 0, false, "wire calibrate");
  expect_roundtrip(vehicle::CommandKind::Stop, 0, false, "wire stop");
  expect_roundtrip(vehicle::CommandKind::Turn, 270, true, "wire turn");
  expect_roundtrip(vehicle::CommandKind::Move, 20, true, "wire move");

  vehicle::Telemetry lidar = vehicle::parse_telemetry("LIDAR:12");
  expect(lidar.kind == vehicle::TelemetryKind::Lidar && lidar.has_number && lidar.number == 12,
         "lidar telemetry");

  vehicle::Telemetry compass = vehicle::parse_telemetry("COMP:359");
  expect(compass.kind == vehicle::TelemetryKind::Compass && compass.number == 359,
         "compass telemetry");

  vehicle::Telemetry color = vehicle::parse_telemetry("RGB:red");
  expect(color.kind == vehicle::TelemetryKind::Rgb && !color.has_number &&
             strcmp(color.text, "red") == 0,
         "rgb telemetry");

  vehicle::Telemetry encoder = vehicle::parse_telemetry("ENC:0.5000");
  expect(encoder.kind == vehicle::TelemetryKind::Encoder && !encoder.has_number &&
             strcmp(encoder.text, "0.5000") == 0,
         "encoder telemetry");

  vehicle::Telemetry missing = vehicle::parse_telemetry("ENC:none");
  expect(missing.kind == vehicle::TelemetryKind::Encoder && strcmp(missing.text, "none") == 0,
         "encoder unset");

  vehicle::Telemetry error = vehicle::parse_telemetry("ERR:compass");
  expect(error.kind == vehicle::TelemetryKind::Error && strcmp(error.text, "compass") == 0,
         "compass fault");

  char wire[32];
  expect(vehicle::format_telemetry_int(vehicle::TelemetryKind::Lidar, 12, wire, sizeof wire),
         "format lidar");
  vehicle::Telemetry round = vehicle::parse_telemetry(wire);
  expect(round.kind == vehicle::TelemetryKind::Lidar && round.number == 12, "lidar roundtrip");
  expect(vehicle::format_telemetry_text(vehicle::TelemetryKind::Error, "compass", wire, sizeof wire),
         "format error");
  expect(strcmp(wire, "ERR:compass") == 0, "error prefix");
  expect(!vehicle::format_telemetry_text(vehicle::TelemetryKind::Error, "bad\nline", wire, sizeof wire),
         "reject newline in payload");
}

void test_encoder() {
  vehicle::EncoderScale zero = vehicle::mm_per_pulse(20, 0);
  expect(!zero.ok, "zero pulses");
  vehicle::EncoderScale negative = vehicle::mm_per_pulse(0, 10);
  expect(!negative.ok, "zero distance");

  vehicle::EncoderScale scale = vehicle::mm_per_pulse(20, 4000);
  expect(scale.ok, "scale ok");
  expect(scale.mm_per_pulse > 0.49f && scale.mm_per_pulse < 0.51f, "20cm / 4000 pulses");

  char text[16];
  expect(vehicle::format_mm_per_pulse(scale.mm_per_pulse, text, sizeof text), "format scale");
  expect(strcmp(text, "0.5000") == 0, "formatted 0.5000");
  expect(vehicle::pulses_for_cm(0.5f, 20) == 400, "pulses for 20cm");
  expect(vehicle::pulses_for_cm(0.0f, 20) == 0, "no pulses without a scale");
  expect(!vehicle::scale_is_plausible(0.0f / 0.0f), "nan rejected");
  expect(!vehicle::scale_is_plausible(1.0e20f), "huge scale rejected");
}

}  // namespace

int main() {
  test_heading();
  test_control();
  test_color();
  test_protocol();
  test_encoder();
  if (g_failures != 0) {
    fprintf(stderr, "%d failed\n", g_failures);
    return 1;
  }
  printf("ok\n");
  return 0;
}
