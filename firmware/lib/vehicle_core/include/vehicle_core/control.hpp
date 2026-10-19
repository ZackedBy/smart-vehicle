#pragma once

#include <stdint.h>

namespace vehicle {

// Unsigned millisecond difference, valid across the millis() wrap.
bool time_elapsed(uint32_t start_ms, uint32_t now_ms, uint32_t budget_ms);

// Integer percent of max PWM. percent is clamped to 0..100.
int pwm_percent(int max_pwm, int percent);

// Full PWM outside the slow band, quarter PWM inside it (inclusive).
int rotation_pwm(int diff_deg, int max_pwm, int slow_band_deg);

}  // namespace vehicle
