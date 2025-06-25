#include "color.hpp"
#include <algorithm>
#include <cmath>

namespace Q::radiometry {

  float Color::luminance() const {
    // Standard luminance calculation (ITU-R BT.709)
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
  }

  Color Color::tone_map_reinhard() const {
    // Luminance-based Reinhard tone mapping with white point
    float lum = luminance();
    float white_point = 2.0f; // Adjust this to control brightness
    float mapped_lum = (lum * (1.0f + lum / (white_point * white_point))) / (1.0f + lum);

    // Preserve color ratios
    if (lum > 0.001f) {
      float scale = mapped_lum / lum;
      return Color(r * scale, g * scale, b * scale);
    }
    return *this;
  }

  Color Color::tone_map_exposure(float exposure) const {
    // Simple exposure adjustment
    float scale = std::pow(2.0f, exposure);
    Color exposed = Color(r * scale, g * scale, b * scale);
    // Apply simple tone mapping to prevent clipping
    return exposed.tone_map_reinhard();
  }

  Color Color::tone_map_aces() const {
    // ACES tone mapping approximation (Narkowicz 2015)
    // More conservative parameters for natural-looking results
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    // Pre-exposure adjustment to prevent over-brightening
    const float pre_exposure = 0.6f;

    auto aces_curve = [=](float x) {
      x *= pre_exposure;
      return std::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
    };

    return Color(aces_curve(r), aces_curve(g), aces_curve(b));
  }

  Color Color::apply_gamma(float gamma) const {
    // Apply gamma correction
    float inv_gamma = 1.0f / gamma;
    return Color(std::pow(std::max(0.0f, r), inv_gamma), std::pow(std::max(0.0f, g), inv_gamma),
                 std::pow(std::max(0.0f, b), inv_gamma));
  }

} // namespace Q::radiometry