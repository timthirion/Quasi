#pragma once

#include "ray.hpp"
#include "vec3.hpp"

namespace Q {
  namespace geometry {

    // Compute reflection ray given incoming ray and surface normal
    inline Ray compute_reflection_ray(const Ray &incident_ray, const Vec3 &hit_point,
                                      const Vec3 &normal) {
      // Ensure normal is pointing towards the incident ray
      Vec3 n = normal;
      if (incident_ray.direction.dot_product(n) > 0) {
        n = Vec3(-n.x, -n.y, -n.z); // Flip normal
      }

      // Reflection formula: r = d - 2(d·n)n
      Vec3 reflected_direction =
          incident_ray.direction - n * (2.0f * incident_ray.direction.dot_product(n));

      // Start reflection ray slightly offset from hit point to avoid self-intersection
      const float epsilon = 1e-4f;
      Vec3 reflection_origin = hit_point + n * epsilon;

      return Ray(reflection_origin, reflected_direction.get_normalized());
    }

  } // namespace geometry
} // namespace Q