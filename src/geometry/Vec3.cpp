#include "Vec3.h"

namespace Q {
  namespace geometry {

    Vec3 Vec3::operator+(const Vec3 &other) const {
      return Vec3(x + other.x, y + other.y, z + other.z);
    }

    Vec3 Vec3::operator-(const Vec3 &other) const {
      return Vec3(x - other.x, y - other.y, z - other.z);
    }

    Vec3 Vec3::operator*(float scalar) const {
      return Vec3(x * scalar, y * scalar, z * scalar);
    }

    bool Vec3::operator==(const Vec3 &other) const {
      const float epsilon = 1e-6f;
      return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon &&
             std::abs(z - other.z) < epsilon;
    }

    float Vec3::dot_product(const Vec3 &other) const {
      return x * other.x + y * other.y + z * other.z;
    }

    Vec3 Vec3::cross_product(const Vec3 &other) const {
      return Vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }

    float Vec3::get_length() const {
      return std::sqrt(x * x + y * y + z * z);
    }

    Vec3 Vec3::get_normalized() const {
      float len = get_length();
      if (len == 0.0f)
        return Vec3(0, 0, 0);
      return Vec3(x / len, y / len, z / len);
    }

  } // namespace geometry
} // namespace Q