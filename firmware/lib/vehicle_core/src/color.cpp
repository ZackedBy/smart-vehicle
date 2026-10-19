#include "vehicle_core/color.hpp"

namespace vehicle {
namespace {

int magnitude(int value) { return value < 0 ? -value : value; }

}  // namespace

bool color_matches(Rgb sample, Rgb target, int threshold) {
  if (threshold < 0) return false;
  return magnitude(sample.r - target.r) <= threshold &&
         magnitude(sample.g - target.g) <= threshold &&
         magnitude(sample.b - target.b) <= threshold;
}

ColorName classify_color(Rgb sample, const ColorCalibration& calibration, int threshold) {
  if (!calibration.ready) return ColorName::Black;
  if (color_matches(sample, calibration.red, threshold)) return ColorName::Red;
  if (color_matches(sample, calibration.green, threshold)) return ColorName::Green;
  if (color_matches(sample, calibration.blue, threshold)) return ColorName::Blue;
  return ColorName::Black;
}

const char* color_name(ColorName color) {
  switch (color) {
    case ColorName::Red:
      return "red";
    case ColorName::Green:
      return "green";
    case ColorName::Blue:
      return "blue";
    case ColorName::Black:
      return "black";
  }
  return "black";
}

}  // namespace vehicle
