#pragma once

#include "../radiometry/color.hpp"

namespace Q {
  namespace materials {

    class Texture {
    public:
      virtual ~Texture() = default;
      virtual Q::radiometry::Color sample(float u, float v) const = 0;
    };

  } // namespace materials
} // namespace Q