#pragma once

#include <stdint.h>

namespace vehicle {

// CMPS14 heading byte (0-255) mapped the same way as Arduino map(raw, 0, 255, 0, 360),
// then wrapped into [0, 360).
int heading_from_raw(uint8_t raw);

// Wrap any integer angle into [0, 360).
int normalize_heading(int degrees);

// Subtract the boot offset and wrap. Offset is the uncorrected heading captured at startup.
int apply_offset(int uncorrected_deg, int offset_deg);

// Shortest signed turn from current to target, in [-180, 180]. +180 stays positive.
int shortest_diff(int current_deg, int target_deg);

// True when the signed difference is inside +/- tolerance, inclusive.
bool within_tolerance(int diff_deg, int tolerance_deg);

// Eight-point compass label. heading is wrapped before the lookup.
const char* cardinal(int heading_deg);

}  // namespace vehicle
