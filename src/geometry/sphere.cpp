#include "sphere.hpp"
#include <cmath>

namespace Q {
  namespace geometry {

    Sphere::Sphere(const Vec3 &center, float radius) : center(center), radius(radius) {}

    bool Sphere::contains_point(const Vec3 &point) const {
      Vec3 to_point = point - center;
      return to_point.get_length() <= radius;
    }

    Vec3 Sphere::get_normal_at(const Vec3 &point) const {
      return (point - center).get_normalized();
    }

    float Sphere::get_surface_area() const {
      return 4.0f * M_PI * radius * radius;
    }

    float Sphere::get_volume() const {
      return (4.0f / 3.0f) * M_PI * radius * radius * radius;
    }

    SphereIntersectionResult::SphereIntersectionResult()
        : hit(false), t_near(0), t_far(0), point_near(), point_far(), normal_near(), normal_far() {}

    SphereIntersectionResult::SphereIntersectionResult(float t_near, float t_far,
                                                       const Vec3 &point_near,
                                                       const Vec3 &point_far,
                                                       const Vec3 &normal_near,
                                                       const Vec3 &normal_far)
        : hit(true), t_near(t_near), t_far(t_far), point_near(point_near), point_far(point_far),
          normal_near(normal_near), normal_far(normal_far) {}

    std::optional<SphereIntersectionResult> intersect(const Ray &ray, const Sphere &sphere) {
      // Vector from ray origin to sphere center
      Vec3 oc = ray.origin - sphere.center;

      // Quadratic equation coefficients: at^2 + bt + c = 0
      float a = ray.direction.dot(ray.direction);
      float b = 2.0f * oc.dot(ray.direction);
      float c = oc.dot(oc) - sphere.radius * sphere.radius;

      // Calculate discriminant
      float discriminant = b * b - 4 * a * c;

      // No intersection if discriminant is negative
      if (discriminant < 0) {
        return std::nullopt;
      }

      // Calculate the two possible intersection points
      float sqrt_discriminant = std::sqrt(discriminant);
      float t1 = (-b - sqrt_discriminant) / (2.0f * a);
      float t2 = (-b + sqrt_discriminant) / (2.0f * a);

      // Ensure t1 <= t2
      if (t1 > t2) {
        std::swap(t1, t2);
      }

      // Both intersections behind ray origin
      if (t2 < 0) {
        return std::nullopt;
      }

      // Calculate intersection points and normals
      Vec3 point1 = ray.point_at(t1);
      Vec3 point2 = ray.point_at(t2);
      Vec3 normal1 = sphere.get_normal_at(point1);
      Vec3 normal2 = sphere.get_normal_at(point2);

      return SphereIntersectionResult(t1, t2, point1, point2, normal1, normal2);
    }

  } // namespace geometry
} // namespace Q