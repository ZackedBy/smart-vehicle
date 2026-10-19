#include "vehicle_core/encoder.hpp"

#include <math.h>
#include <stdio.h>

namespace vehicle {

bool scale_is_plausible(float mm_per_pulse) {
  return isfinite(mm_per_pulse) && mm_per_pulse > 0.01f && mm_per_pulse < 50.0f;
}

EncoderScale mm_per_pulse(int distance_cm, int pulses) {
  EncoderScale scale{false, 0.0f};
  if (distance_cm <= 0 || pulses <= 0) return scale;
  float value = (static_cast<float>(distance_cm) / static_cast<float>(pulses)) * 100.0f;
  if (!scale_is_plausible(value)) return scale;
  scale.ok = true;
  scale.mm_per_pulse = value;
  return scale;
}

int pulses_for_cm(float mm_per_pulse_value, int distance_cm) {
  if (!scale_is_plausible(mm_per_pulse_value) || distance_cm <= 0) return 0;
  float millimeters = static_cast<float>(distance_cm) * 10.0f;
  int pulses = static_cast<int>((millimeters / mm_per_pulse_value) + 0.5f);
  return pulses > 0 ? pulses : 0;
}

bool format_mm_per_pulse(float mm_per_pulse_value, char* out, size_t out_len) {
  if (out == nullptr || out_len == 0 || !scale_is_plausible(mm_per_pulse_value)) return false;
  int whole = static_cast<int>(mm_per_pulse_value);
  float fraction = mm_per_pulse_value - static_cast<float>(whole);
  int decimals = static_cast<int>(fraction * 10000.0f + 0.5f);
  if (decimals >= 10000) {
    whole += 1;
    decimals -= 10000;
  }
  int wrote = snprintf(out, out_len, "%d.%04d", whole, decimals);
  return wrote > 0 && static_cast<size_t>(wrote) < out_len;
}

}  // namespace vehicle
