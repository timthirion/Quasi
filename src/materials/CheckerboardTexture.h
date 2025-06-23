#pragma once

#include "../radiometry/Color.h"
#include "Texture.h"
#include <cmath>

namespace Q {
  namespace materials {

    class CheckerboardTexture : public Texture {
    private:
      Q::radiometry::Color color1, color2;
      int checks_u, checks_v;

    public:
      CheckerboardTexture(const Q::radiometry::Color &c1, const Q::radiometry::Color &c2,
                          int u_checks, int v_checks)
          : color1(c1), color2(c2), checks_u(u_checks), checks_v(v_checks) {}

      Q::radiometry::Color sample(float u, float v) const override {
        // Clamp UV coordinates to [0, 1] to handle edge cases
        u = std::fmax(0.0f, std::fmin(1.0f, u));
        v = std::fmax(0.0f, std::fmin(1.0f, v));

        // Calculate which checker square we're in
        int u_checker = static_cast<int>(std::floor(u * checks_u));
        int v_checker = static_cast<int>(std::floor(v * checks_v));

        // Handle edge case where u or v equals 1.0
        if (u_checker >= checks_u)
          u_checker = checks_u - 1;
        if (v_checker >= checks_v)
          v_checker = checks_v - 1;

        // Checkerboard pattern: alternate colors based on sum of checker indices
        bool is_even = (u_checker + v_checker) % 2 == 0;
        return is_even ? color1 : color2;
      }
    };

  } // namespace materials
} // namespace Q