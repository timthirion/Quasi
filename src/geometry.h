#pragma once

#include <cmath>
#include <optional>

namespace Q {
  namespace geometry {

    struct Vec3 {
      float x, y, z;

      Vec3() : x(0), y(0), z(0) {}
      Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

      Vec3 operator+(const Vec3 &other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
      }

      Vec3 operator-(const Vec3 &other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
      }

      Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }

      float dot_product(const Vec3 &other) const { return x * other.x + y * other.y + z * other.z; }

      Vec3 cross_product(const Vec3 &other) const {
        return Vec3(y * other.z - z * other.y, z * other.x - x * other.z,
                    x * other.y - y * other.x);
      }

      float get_length() const { return std::sqrt(x * x + y * y + z * z); }

      Vec3 get_normalized() const {
        float len = get_length();
        if (len == 0.0f)
          return Vec3(0, 0, 0);
        return Vec3(x / len, y / len, z / len);
      }

      bool operator==(const Vec3 &other) const {
        const float epsilon = 1e-6f;
        return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon &&
               std::abs(z - other.z) < epsilon;
      }
    };

    struct Ray {
      Vec3 origin;
      Vec3 direction;

      Ray(const Vec3 &origin, const Vec3 &direction)
          : origin(origin), direction(direction.get_normalized()) {}

      Vec3 point_at(float t) const { return origin + direction * t; }
    };

    struct Triangle {
      Vec3 v0, v1, v2;

      Triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2) : v0(v0), v1(v1), v2(v2) {}

      Vec3 get_normal() const {
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        return edge1.cross_product(edge2).get_normalized();
      }

      Vec3 get_center() const { return (v0 + v1 + v2) * (1.0f / 3.0f); }
    };

    struct IntersectionResult {
      bool hit;
      float t;
      Vec3 point;
      Vec3 barycentric;

      IntersectionResult() : hit(false), t(0), point(), barycentric() {}
      IntersectionResult(float t, const Vec3 &point, const Vec3 &barycentric)
          : hit(true), t(t), point(point), barycentric(barycentric) {}
    };

    std::optional<IntersectionResult> ray_triangle_intersection(const Ray &ray,
                                                                const Triangle &triangle);

  } // namespace geometry
} // namespace Q