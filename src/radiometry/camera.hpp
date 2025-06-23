#pragma once

#include "../geometry/geometry.hpp"
#include <cmath>

namespace Q::radiometry {

  class Camera {
  private:
    Q::geometry::Vec3 origin;
    Q::geometry::Vec3 lower_left_corner;
    Q::geometry::Vec3 horizontal;
    Q::geometry::Vec3 vertical;

  public:
    Camera(Q::geometry::Vec3 look_from, Q::geometry::Vec3 look_at, Q::geometry::Vec3 vup,
           float vfov, float aspect_ratio) {
      float theta = vfov * M_PI / 180.0f;
      float half_height = std::tan(theta / 2.0f);
      float half_width = aspect_ratio * half_height;

      origin = look_from;
      Q::geometry::Vec3 w = (look_from - look_at).get_normalized();
      Q::geometry::Vec3 u = vup.cross_product(w).get_normalized();
      Q::geometry::Vec3 v = w.cross_product(u);

      lower_left_corner = origin - u * half_width - v * half_height - w;
      horizontal = u * (2.0f * half_width);
      vertical = v * (2.0f * half_height);
    }

    Q::geometry::Ray get_ray(float u, float v) const {
      return Q::geometry::Ray(origin, lower_left_corner + horizontal * u + vertical * v - origin);
    }
  };

} // namespace Q::radiometry