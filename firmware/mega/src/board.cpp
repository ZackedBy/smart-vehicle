#include "board.hpp"

#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>

#include <stdio.h>

#include <vehicle_core/control.hpp>
#include <vehicle_core/encoder.hpp>
#include <vehicle_core/heading.hpp>

namespace {

volatile uint32_t pulses_left = 0;
volatile uint32_t pulses_right = 0;
volatile bool button_pending = false;
volatile uint32_t last_button_ms = 0;

void encoder_left_isr() { pulses_left++; }

void encoder_right_isr() { pulses_right++; }

void button_isr() {
  uint32_t now = millis();
  if (now - last_button_ms < cfg::kButtonDebounceMs) return;
  last_button_ms = now;
  button_pending = true;
}

int clamp_pwm(int pwm) {
  if (pwm < 0) return 0;
  if (pwm > cfg::kMotorMaxPwm) return cfg::kMotorMaxPwm;
  return pwm;
}

struct StoredScale {
  uint32_t magic;
  float mm_per_pulse;
};

}  // namespace

Board::Board()
    : lcd_(cfg::kLcdRs, cfg::kLcdEn, cfg::kLcdD4, cfg::kLcdD5, cfg::kLcdD6, cfg::kLcdD7),
      rgb_(TCS34725_INTEGRATIONTIME_2_4MS, TCS34725_GAIN_4X) {}

void Board::begin() {
  Serial.begin(cfg::kBaud);
  Serial2.begin(cfg::kBaud);

  pinMode(cfg::kJoyButton, INPUT_PULLUP);
  pinMode(cfg::kMotorLDir, OUTPUT);
  pinMode(cfg::kMotorRDir, OUTPUT);
  pinMode(cfg::kEncALeft, INPUT);
  pinMode(cfg::kEncARight, INPUT);
  pinMode(cfg::kEncBLeft, INPUT);
  pinMode(cfg::kEncBRight, INPUT);
  stop_motors();

  lcd_.begin(20, 4);
  lcd_.clear();
  lcd_.print(F("Smart Vehicle"));
  uint32_t now = millis();
  last_lcd_ms_ = now - cfg::kLcdPeriodMs;
  last_tx_ms_ = now - cfg::kTelemetryPeriodMs;

  Wire.begin();
  rgb_ok_ = rgb_.begin();
  if (!rgb_ok_) {
    set_status("RGB missing");
    Serial.println(F("RGB sensor missing"));
    lcd_.setCursor(0, 1);
    lcd_.print(F("RGB missing"));
    return;
  }

  lidar_hw_.begin(1, true);
  lidar_.begin(lidar_hw_);

  int initial = 0;
  if (sample_compass(&initial)) {
    offset_ = initial;
    offset_valid_ = true;
    heading_ = 0;
    compass_ok_ = true;
    misses_ = 0;
  } else {
    compass_ok_ = false;
    Serial.println(F("Compass missing"));
  }

  load_scale();
  update_color();
  set_status(compass_ok_ ? "Color 0/3" : "No compass");

  attachInterrupt(digitalPinToInterrupt(cfg::kJoyButton), button_isr, FALLING);
  attachInterrupt(digitalPinToInterrupt(cfg::kEncARight), encoder_right_isr, RISING);
  attachInterrupt(digitalPinToInterrupt(cfg::kEncALeft), encoder_left_isr, RISING);
}

bool Board::sample_compass(int* uncorrected) {
  Wire.beginTransmission(cfg::kCompassAddress);
  Wire.write(0x01);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(static_cast<int>(cfg::kCompassAddress), 1) != 1) return false;
  if (Wire.available() < 1) return false;
  uint8_t raw = static_cast<uint8_t>(Wire.read());
  *uncorrected = vehicle::heading_from_raw(raw);
  return true;
}

void Board::poll_compass() {
  int raw = 0;
  if (!sample_compass(&raw)) {
    if (misses_ < 1000) ++misses_;
    if (misses_ >= cfg::kCompassMissLimit) compass_ok_ = false;
    return;
  }
  misses_ = 0;
  if (!offset_valid_) {
    compass_ok_ = false;
    return;
  }
  heading_ = vehicle::apply_offset(raw, offset_);
  compass_ok_ = true;
}

void Board::sample_rgb() {
  if (!rgb_ok_) return;
  float red = 0.0f;
  float green = 0.0f;
  float blue = 0.0f;
  rgb_.getRGB(&red, &green, &blue);
  sample_.r = static_cast<int>(red);
  sample_.g = static_cast<int>(green);
  sample_.b = static_cast<int>(blue);
  update_color();
}

void Board::update_color() {
  vehicle::ColorCalibration calibration{};
  calibration.ready = captured_ >= cfg::kColorCount;
  if (calibration.ready) {
    calibration.red = slots_[0];
    calibration.green = slots_[1];
    calibration.blue = slots_[2];
  }
  color_ = vehicle::classify_color(sample_, calibration, cfg::kColorThreshold);
  snprintf(color_text_, sizeof color_text_, "%s", vehicle::color_name(color_));
}

void Board::poll_lidar() {
  lidar_.update();
  if (lidar_.busy()) return;
  if (lidar_.has_sample()) {
    lidar_cm_ = lidar_.centimeters();
    have_lidar_ = true;
  }
  lidar_.request(cfg::kLidarSamples);
}

void Board::poll(bool continuous_lidar) {
  poll_compass();
  uint32_t now = millis();
  if (vehicle::time_elapsed(last_rgb_ms_, now, cfg::kRgbPeriodMs)) {
    last_rgb_ms_ = now;
    sample_rgb();
  }
  if (!continuous_lidar) return;
  shot_started_ = false;
  poll_lidar();
}

bool Board::capture_lidar(int* centimeters) {
  if (centimeters == nullptr) return false;
  if (!shot_started_) {
    lidar_.request(cfg::kLidarSamples);
    shot_started_ = true;
    return false;
  }
  lidar_.update();
  if (lidar_.busy()) return false;
  if (!lidar_.has_sample()) {
    lidar_.request(cfg::kLidarSamples);
    return false;
  }
  *centimeters = lidar_.centimeters();
  lidar_cm_ = *centimeters;
  have_lidar_ = true;
  shot_started_ = false;
  return true;
}

void Board::cancel_lidar_capture() { shot_started_ = false; }

void Board::capture_color() {
  if (!rgb_ok_ || captured_ >= cfg::kColorCount) return;
  slots_[captured_] = sample_;
  ++captured_;
  update_color();
}

void Board::set_motors(uint8_t left_dir, uint8_t right_dir, int left_pwm, int right_pwm) {
  digitalWrite(cfg::kMotorLDir, left_dir);
  digitalWrite(cfg::kMotorRDir, right_dir);
  analogWrite(cfg::kMotorLPwm, clamp_pwm(left_pwm));
  analogWrite(cfg::kMotorRPwm, clamp_pwm(right_pwm));
}

void Board::stop_motors() { set_motors(cfg::kForward, cfg::kForward, 0, 0); }

void Board::drive(uint8_t direction, int pwm) { set_motors(direction, direction, pwm, pwm); }

void Board::rotate(bool clockwise, int pwm) {
  if (clockwise) set_motors(cfg::kForward, cfg::kBackward, pwm, pwm);
  else set_motors(cfg::kBackward, cfg::kForward, pwm, pwm);
}

void Board::reset_encoders() {
  noInterrupts();
  pulses_left = 0;
  pulses_right = 0;
  interrupts();
}

uint32_t Board::average_pulses() const {
  noInterrupts();
  uint32_t left = pulses_left;
  uint32_t right = pulses_right;
  interrupts();
  return (left + right) / 2;
}

void Board::load_scale() {
  StoredScale stored{};
  EEPROM.get(cfg::kEepromAddress, stored);
  if (stored.magic != cfg::kEncoderMagic || !vehicle::scale_is_plausible(stored.mm_per_pulse)) {
    scale_ok_ = false;
    snprintf(scale_text_, sizeof scale_text_, "none");
    return;
  }
  scale_ = stored.mm_per_pulse;
  scale_ok_ = true;
  if (!vehicle::format_mm_per_pulse(scale_, scale_text_, sizeof scale_text_)) {
    snprintf(scale_text_, sizeof scale_text_, "none");
    scale_ok_ = false;
  }
}

bool Board::save_scale(float mm_per_pulse) {
  if (!vehicle::format_mm_per_pulse(mm_per_pulse, scale_text_, sizeof scale_text_)) return false;
  StoredScale stored{cfg::kEncoderMagic, mm_per_pulse};
  EEPROM.put(cfg::kEepromAddress, stored);
  scale_ = mm_per_pulse;
  scale_ok_ = true;
  return true;
}

void Board::set_status(const char* text) {
  if (text == nullptr) text = "";
  snprintf(status_, sizeof status_, "%s", text);
}

void Board::render() {
  uint32_t now = millis();
  if (!vehicle::time_elapsed(last_lcd_ms_, now, cfg::kLcdPeriodMs)) return;
  last_lcd_ms_ = now;

  lcd_.clear();
  lcd_.setCursor(0, 0);
  if (!compass_ok_) {
    lcd_.print(F("--"));
  } else {
    lcd_.print(vehicle::cardinal(heading_));
    lcd_.print(' ');
    lcd_.print(heading_);
  }
  lcd_.print(F(" L:"));
  if (have_lidar_) lcd_.print(lidar_cm_);
  else lcd_.print(F("--"));

  lcd_.setCursor(0, 1);
  lcd_.print(color_text_);
  lcd_.print(' ');
  lcd_.print(sample_.r);
  lcd_.print(' ');
  lcd_.print(sample_.g);
  lcd_.print(' ');
  lcd_.print(sample_.b);

  lcd_.setCursor(0, 2);
  lcd_.print(status_);

  lcd_.setCursor(0, 3);
  lcd_.print(F("mm/p "));
  lcd_.print(scale_text_);
}

bool Board::consume_button() {
  noInterrupts();
  bool pressed = button_pending;
  button_pending = false;
  interrupts();
  return pressed;
}

bool Board::read_command(vehicle::Command* command) {
  if (command == nullptr) return false;
  while (Serial2.available() > 0) {
    char next = static_cast<char>(Serial2.read());
    if (next == '\r') continue;
    if (next != '\n') {
      if (rx_len_ + 1 < sizeof rx_) rx_[rx_len_++] = next;
      else rx_overflow_ = true;
      continue;
    }

    rx_[rx_len_] = '\0';
    bool overflow = rx_overflow_;
    rx_len_ = 0;
    rx_overflow_ = false;
    if (overflow) {
      *command = {vehicle::CommandKind::Unknown, 0, false};
      return true;
    }
    Serial.println(rx_);
    *command = vehicle::parse_command(rx_);
    if (command->kind == vehicle::CommandKind::None) continue;
    return true;
  }
  return false;
}

void Board::publish() {
  uint32_t now = millis();
  if (!vehicle::time_elapsed(last_tx_ms_, now, cfg::kTelemetryPeriodMs)) return;
  last_tx_ms_ = now;

  char line[48];
  if (have_lidar_ &&
      vehicle::format_telemetry_int(vehicle::TelemetryKind::Lidar, lidar_cm_, line, sizeof line)) {
    Serial2.println(line);
  }
  if (compass_ok_ &&
      vehicle::format_telemetry_int(vehicle::TelemetryKind::Compass, heading_, line, sizeof line)) {
    Serial2.println(line);
  } else if (vehicle::format_telemetry_text(vehicle::TelemetryKind::Error, "compass", line,
                                            sizeof line)) {
    Serial2.println(line);
  }
  if (vehicle::format_telemetry_text(vehicle::TelemetryKind::Rgb, color_text_, line, sizeof line)) {
    Serial2.println(line);
  }
  if (vehicle::format_telemetry_text(vehicle::TelemetryKind::Encoder, scale_text_, line,
                                     sizeof line)) {
    Serial2.println(line);
  }
}
