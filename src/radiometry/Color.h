#pragma once

#include <algorithm>

namespace Q::radiometry {

  struct Color {
    float r, g, b;

    Color() : r(0), g(0), b(0) {}
    Color(float r, float g, float b) : r(r), g(g), b(b) {}

    // Convert to 0-255 range for PPM
    int r_int() const { return static_cast<int>(std::clamp(r * 255.0f, 0.0f, 255.0f)); }
    int g_int() const { return static_cast<int>(std::clamp(g * 255.0f, 0.0f, 255.0f)); }
    int b_int() const { return static_cast<int>(std::clamp(b * 255.0f, 0.0f, 255.0f)); }
  };

} // namespace Q::radiometry