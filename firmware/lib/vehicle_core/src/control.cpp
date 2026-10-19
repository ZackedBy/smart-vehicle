#include "vehicle_core/control.hpp"

namespace vehicle {
namespace {

int magnitude(int value) { return value < 0 ? -value : value; }

}  // namespace

bool time_elapsed(uint32_t start_ms, uint32_t now_ms, uint32_t budget_ms) {
  return (now_ms - start_ms) >= budget_ms;
}

int pwm_percent(int max_pwm, int percent) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  if (max_pwm < 0) max_pwm = 0;
  return (max_pwm * percent) / 100;
}

int rotation_pwm(int diff_deg, int max_pwm, int slow_band_deg) {
  if (max_pwm < 0) max_pwm = 0;
  if (magnitude(diff_deg) > slow_band_deg) return max_pwm;
  return max_pwm / 4;
}

}  // namespace vehicle
