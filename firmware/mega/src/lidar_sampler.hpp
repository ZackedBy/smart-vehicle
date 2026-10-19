#pragma once

#include <LIDARLite.h>

// One averaged distance, spread across loop() so the sampler never owns the CPU.
class LidarSampler {
 public:
  void begin(LIDARLite& lidar);
  void request(int samples);
  void update();
  bool busy() const { return active_; }
  bool has_sample() const { return has_sample_; }
  int centimeters() const { return result_; }

 private:
  LIDARLite* lidar_ = nullptr;
  int sum_ = 0;
  int count_ = 0;
  int invalid_ = 0;
  int target_ = 0;
  unsigned long last_ms_ = 0;
  bool active_ = false;
  bool has_sample_ = false;
  int result_ = 0;
};
