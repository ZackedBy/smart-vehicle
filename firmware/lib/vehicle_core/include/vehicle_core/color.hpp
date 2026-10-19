#pragma once

namespace vehicle {

struct Rgb {
  int r;
  int g;
  int b;
};

enum class ColorName { Black, Red, Green, Blue };

struct ColorCalibration {
  Rgb red;
  Rgb green;
  Rgb blue;
  bool ready;
};

bool color_matches(Rgb sample, Rgb target, int threshold);

// Red, then green, then blue. Anything else, including an incomplete calibration, is black.
ColorName classify_color(Rgb sample, const ColorCalibration& calibration, int threshold);

const char* color_name(ColorName color);

}  // namespace vehicle
