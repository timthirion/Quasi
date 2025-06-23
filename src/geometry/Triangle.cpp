#include "Triangle.h"

namespace Q {
  namespace geometry {

    Triangle::Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2) : v0(v0), v1(v1), v2(v2) {}

    Vec3 Triangle::get_normal() const {
      Vec3 edge1 = v1 - v0;
      Vec3 edge2 = v2 - v0;
      return edge1.cross_product(edge2).get_normalized();
    }

    Vec3 Triangle::get_center() const {
      return (v0 + v1 + v2) * (1.0f / 3.0f);
    }

    IntersectionResult::IntersectionResult() : hit(false), t(0), point(), barycentric() {}

    IntersectionResult::IntersectionResult(float t, const Vec3 &point, const Vec3 &barycentric)
        : hit(true), t(t), point(point), barycentric(barycentric) {}

    std::optional<IntersectionResult> ray_triangle_intersection(const Ray &ray,
                                                                const Triangle &triangle) {
      const float EPSILON = 1e-8f;

      Vec3 edge1 = triangle.v1 - triangle.v0;
      Vec3 edge2 = triangle.v2 - triangle.v0;

      Vec3 h = ray.direction.cross_product(edge2);
      float a = edge1.dot_product(h);

      if (a > -EPSILON && a < EPSILON) {
        return std::nullopt;
      }

      float f = 1.0f / a;
      Vec3 s = ray.origin - triangle.v0;
      float u = f * s.dot_product(h);

      if (u < 0.0f || u > 1.0f) {
        return std::nullopt;
      }

      Vec3 q = s.cross_product(edge1);
      float v = f * ray.direction.dot_product(q);

      if (v < 0.0f || u + v > 1.0f) {
        return std::nullopt;
      }

      float t = f * edge2.dot_product(q);

      if (t > EPSILON) {
        Vec3 intersectionPoint = ray.point_at(t);
        Vec3 barycentric(1.0f - u - v, u, v);
        return IntersectionResult(t, intersectionPoint, barycentric);
      }

      return std::nullopt;
    }

  } // namespace geometry
} // namespace Q