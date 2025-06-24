#include "color.hpp"
#include <algorithm>
#include <cmath>

namespace Q::radiometry {

  float Color::luminance() const {
    // Standard luminance calculation (ITU-R BT.709)
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
  }

  Color Color::tone_map_reinhard() const {
    // Simple Reinhard tone mapping: color / (1 + color)
    return Color(r / (1.0f + r), g / (1.0f + g), b / (1.0f + b));
  }

  Color Color::tone_map_exposure(float exposure) const {
    // Simple exposure adjustment
    float scale = std::pow(2.0f, exposure);
    Color exposed = Color(r * scale, g * scale, b * scale);
    // Apply simple tone mapping to prevent clipping
    return exposed.tone_map_reinhard();
  }

  Color Color::tone_map_aces() const {
    // Simplified ACES tone mapping approximation
    // This is a simplified version of the ACES RRT/ODT
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    auto aces_curve = [=](float x) { return (x * (a * x + b)) / (x * (c * x + d) + e); };

    return Color(aces_curve(r), aces_curve(g), aces_curve(b));
  }

  Color Color::apply_gamma(float gamma) const {
    // Apply gamma correction
    float inv_gamma = 1.0f / gamma;
    return Color(std::pow(std::max(0.0f, r), inv_gamma), std::pow(std::max(0.0f, g), inv_gamma),
                 std::pow(std::max(0.0f, b), inv_gamma));
  }

} // namespace Q::radiometry