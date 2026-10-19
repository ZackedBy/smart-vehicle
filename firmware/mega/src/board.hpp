#pragma once

#include <stddef.h>
#include <stdint.h>

#include <Adafruit_TCS34725.h>
#include <LIDARLite.h>
#include <LiquidCrystal.h>

#include <vehicle_core/color.hpp>
#include <vehicle_core/protocol.hpp>

#include "config.hpp"
#include "lidar_sampler.hpp"

// Hardware drivers. Behavior lives in Pilot; numbers the Pilot needs live in vehicle_core.
class Board {
 public:
  Board();

  void begin();
  bool rgb_ready() const { return rgb_ok_; }
  bool compass_ready() const { return compass_ok_; }
  bool encoder_ready() const { return scale_ok_; }
  bool lidar_ready() const { return have_lidar_; }
  int heading() const { return heading_; }
  int lidar_cm() const { return lidar_cm_; }
  float encoder_scale() const { return scale_; }
  vehicle::ColorName color() const { return color_; }
  int colors_captured() const { return captured_; }

  void poll(bool continuous_lidar);
  bool capture_lidar(int* centimeters);
  void cancel_lidar_capture();

  void capture_color();
  void stop_motors();
  void drive(uint8_t direction, int pwm);
  void rotate(bool clockwise, int pwm);
  void reset_encoders();
  uint32_t average_pulses() const;
  bool save_scale(float mm_per_pulse);

  void set_status(const char* text);
  void render();
  bool consume_button();
  bool read_command(vehicle::Command* command);
  void publish();

 private:
  bool sample_compass(int* uncorrected);
  void poll_compass();
  void sample_rgb();
  void update_color();
  void set_motors(uint8_t left_dir, uint8_t right_dir, int left_pwm, int right_pwm);
  void load_scale();
  void poll_lidar();

  LiquidCrystal lcd_;
  Adafruit_TCS34725 rgb_;
  LIDARLite lidar_hw_;
  LidarSampler lidar_;

  bool rgb_ok_ = false;
  bool compass_ok_ = false;
  bool offset_valid_ = false;
  bool scale_ok_ = false;
  bool have_lidar_ = false;
  bool shot_started_ = false;
  int offset_ = 0;
  int heading_ = 0;
  int misses_ = 0;
  int lidar_cm_ = 0;
  int captured_ = 0;
  float scale_ = 0.0f;
  vehicle::Rgb sample_{};
  vehicle::Rgb slots_[cfg::kColorCount]{};
  vehicle::ColorName color_ = vehicle::ColorName::Black;

  char status_[21] = "boot";
  char color_text_[8] = "black";
  char scale_text_[16] = "none";
  char rx_[64] = {};
  size_t rx_len_ = 0;
  bool rx_overflow_ = false;

  uint32_t last_lcd_ms_ = 0;
  uint32_t last_tx_ms_ = 0;
  uint32_t last_rgb_ms_ = 0;
};
