#pragma once

#include "vec3.hpp"

namespace Q {
  namespace geometry {

    struct Ray {
      Vec3 origin;
      Vec3 direction;

      Ray(const Vec3 &origin, const Vec3 &direction);
      Vec3 point_at(float t) const;
    };

  } // namespace geometry
} // namespace Q