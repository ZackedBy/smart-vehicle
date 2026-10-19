#pragma once

#include <stddef.h>

namespace vehicle {

struct EncoderScale {
  bool ok;
  float mm_per_pulse;
};

// mm/pulse = (distance_cm / pulses) * 100. pulses == 0 is not a scale.
EncoderScale mm_per_pulse(int distance_cm, int pulses);

bool scale_is_plausible(float mm_per_pulse);

// Rounded pulse target for a forward distance. 0 when the scale or distance is unusable.
int pulses_for_cm(float mm_per_pulse, int distance_cm);

// Fixed 4 decimal places, no printf float support required.
bool format_mm_per_pulse(float mm_per_pulse, char* out, size_t out_len);

}  // namespace vehicle
