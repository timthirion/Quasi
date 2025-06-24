#include "vec.hpp"

namespace Q {
  namespace geometry {

    // Vec2 implementations
    Vec2 Vec2::operator+(const Vec2 &other) const {
      return Vec2(x + other.x, y + other.y);
    }

    Vec2 Vec2::operator-(const Vec2 &other) const {
      return Vec2(x - other.x, y - other.y);
    }

    Vec2 Vec2::operator*(float scalar) const {
      return Vec2(x * scalar, y * scalar);
    }

    bool Vec2::operator==(const Vec2 &other) const {
      const float epsilon = 1e-6f;
      return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon;
    }

    float Vec2::dot(const Vec2 &other) const {
      return x * other.x + y * other.y;
    }

    float Vec2::get_length() const {
      return std::sqrt(x * x + y * y);
    }

    Vec2 Vec2::get_normalized() const {
      float len = get_length();
      if (len == 0.0f)
        return Vec2(0, 0);
      return Vec2(x / len, y / len);
    }

    // Vec3 implementations
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

    float Vec3::dot(const Vec3 &other) const {
      return x * other.x + y * other.y + z * other.z;
    }

    Vec3 Vec3::cross(const Vec3 &other) const {
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

    // Vec4 implementations
    Vec4 Vec4::operator+(const Vec4 &other) const {
      return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Vec4 Vec4::operator-(const Vec4 &other) const {
      return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    Vec4 Vec4::operator*(float scalar) const {
      return Vec4(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    bool Vec4::operator==(const Vec4 &other) const {
      const float epsilon = 1e-6f;
      return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon &&
             std::abs(z - other.z) < epsilon && std::abs(w - other.w) < epsilon;
    }

    float Vec4::dot(const Vec4 &other) const {
      return x * other.x + y * other.y + z * other.z + w * other.w;
    }

    float Vec4::get_length() const {
      return std::sqrt(x * x + y * y + z * z + w * w);
    }

    Vec4 Vec4::get_normalized() const {
      float len = get_length();
      if (len == 0.0f)
        return Vec4(0, 0, 0, 0);
      return Vec4(x / len, y / len, z / len, w / len);
    }

  } // namespace geometry
} // namespace Q