#pragma once

#include "ray.hpp"
#include "vec3.hpp"
#include <optional>

namespace Q {
  namespace geometry {

    struct Sphere {
      Vec3 center;
      float radius;

      Sphere(const Vec3 &center, float radius);
      bool contains_point(const Vec3 &point) const;
      Vec3 get_normal_at(const Vec3 &point) const;
      float get_surface_area() const;
      float get_volume() const;
    };

    struct SphereIntersectionResult {
      bool hit;
      float t_near;
      float t_far;
      Vec3 point_near;
      Vec3 point_far;
      Vec3 normal_near;
      Vec3 normal_far;

      SphereIntersectionResult();
      SphereIntersectionResult(float t_near, float t_far, const Vec3 &point_near,
                               const Vec3 &point_far, const Vec3 &normal_near,
                               const Vec3 &normal_far);
    };

    std::optional<SphereIntersectionResult> ray_sphere_intersection(const Ray &ray,
                                                                    const Sphere &sphere);

  } // namespace geometry
} // namespace Q