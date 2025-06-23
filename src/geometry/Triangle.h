#pragma once

#include "Ray.h"
#include "Vec3.h"
#include <optional>

namespace Q {
  namespace geometry {

    struct Triangle {
      Vec3 v0, v1, v2;

      Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2);
      Vec3 get_normal() const;
      Vec3 get_center() const;
    };

    struct IntersectionResult {
      bool hit;
      float t;
      Vec3 point;
      Vec3 barycentric;

      IntersectionResult();
      IntersectionResult(float t, const Vec3 &point, const Vec3 &barycentric);
    };

    std::optional<IntersectionResult> ray_triangle_intersection(const Ray &ray,
                                                                const Triangle &triangle);

  } // namespace geometry
} // namespace Q