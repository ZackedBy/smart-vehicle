#include "vehicle_core/heading.hpp"

namespace vehicle {
namespace {

int magnitude(int value) { return value < 0 ? -value : value; }

}  // namespace

int normalize_heading(int degrees) {
  int wrapped = degrees % 360;
  if (wrapped < 0) wrapped += 360;
  return wrapped;
}

int heading_from_raw(uint8_t raw) {
  return normalize_heading((static_cast<int>(raw) * 360) / 255);
}

int apply_offset(int uncorrected_deg, int offset_deg) {
  return normalize_heading(uncorrected_deg - offset_deg);
}

int shortest_diff(int current_deg, int target_deg) {
  int diff = normalize_heading(target_deg) - normalize_heading(current_deg);
  if (diff > 180) diff -= 360;
  if (diff < -180) diff += 360;
  return diff;
}

bool within_tolerance(int diff_deg, int tolerance_deg) {
  return magnitude(diff_deg) <= tolerance_deg;
}

const char* cardinal(int heading_deg) {
  static const char* labels[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  int wrapped = normalize_heading(heading_deg);
  int index = ((wrapped + 23) % 360) / 45;
  return labels[index];
}

}  // namespace vehicle
