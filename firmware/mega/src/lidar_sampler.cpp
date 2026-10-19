#include "lidar_sampler.hpp"

#include <Arduino.h>

#include "config.hpp"

void LidarSampler::begin(LIDARLite& lidar) { lidar_ = &lidar; }

void LidarSampler::request(int samples) {
  target_ = samples > 0 ? samples : 1;
  sum_ = 0;
  count_ = 0;
  invalid_ = 0;
  last_ms_ = 0;
  active_ = lidar_ != nullptr;
  has_sample_ = false;
}

void LidarSampler::update() {
  if (!active_ || lidar_ == nullptr) return;

  unsigned long now = millis();
  if (last_ms_ != 0 && (now - last_ms_) < cfg::kLidarIntervalMs) return;

  int reading = (count_ == 0) ? lidar_->distance(true) : lidar_->distance(false);
  last_ms_ = now;
  if (reading < 0) {
    ++invalid_;
    if (invalid_ >= target_) active_ = false;
    return;
  }

  sum_ += reading;
  ++count_;
  if (count_ >= target_) {
    result_ = sum_ / target_;
    active_ = false;
    has_sample_ = true;
  }
}
