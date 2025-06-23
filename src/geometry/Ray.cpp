#include "Ray.h"

namespace Q {
  namespace geometry {

    Ray::Ray(const Vec3 &origin, const Vec3 &direction)
        : origin(origin), direction(direction.get_normalized()) {}

    Vec3 Ray::point_at(float t) const {
      return origin + direction * t;
    }

  } // namespace geometry
} // namespace Q