#include "geometry.h"

namespace geometry {

  std::optional<IntersectionResult> rayTriangleIntersection(const Ray &ray,
                                                            const Triangle &triangle) {
    const float EPSILON = 1e-8f;

    Vec3 edge1 = triangle.v1 - triangle.v0;
    Vec3 edge2 = triangle.v2 - triangle.v0;

    Vec3 h = ray.direction.cross(edge2);
    float a = edge1.dot(h);

    if (a > -EPSILON && a < EPSILON) {
      return std::nullopt;
    }

    float f = 1.0f / a;
    Vec3 s = ray.origin - triangle.v0;
    float u = f * s.dot(h);

    if (u < 0.0f || u > 1.0f) {
      return std::nullopt;
    }

    Vec3 q = s.cross(edge1);
    float v = f * ray.direction.dot(q);

    if (v < 0.0f || u + v > 1.0f) {
      return std::nullopt;
    }

    float t = f * edge2.dot(q);

    if (t > EPSILON) {
      Vec3 intersectionPoint = ray.at(t);
      Vec3 barycentric(1.0f - u - v, u, v);
      return IntersectionResult(t, intersectionPoint, barycentric);
    }

    return std::nullopt;
  }

} // namespace geometry