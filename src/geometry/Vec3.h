#pragma once

#include <cmath>

namespace Q {
  namespace geometry {

    struct Vec3 {
      float x, y, z;

      Vec3() : x(0), y(0), z(0) {}
      Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

      Vec3 operator+(const Vec3 &other) const;
      Vec3 operator-(const Vec3 &other) const;
      Vec3 operator*(float scalar) const;
      bool operator==(const Vec3 &other) const;

      float dot_product(const Vec3 &other) const;
      Vec3 cross_product(const Vec3 &other) const;
      float get_length() const;
      Vec3 get_normalized() const;
    };

  } // namespace geometry
} // namespace Q