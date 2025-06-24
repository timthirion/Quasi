#pragma once

#include <cmath>

namespace Q {
  namespace geometry {

    struct Vec2 {
      float x, y;

      Vec2() : x(0), y(0) {}
      Vec2(float x, float y) : x(x), y(y) {}

      Vec2 operator+(const Vec2 &other) const;
      Vec2 operator-(const Vec2 &other) const;
      Vec2 operator*(float scalar) const;
      bool operator==(const Vec2 &other) const;

      float dot(const Vec2 &other) const;
      float get_length() const;
      Vec2 get_normalized() const;
    };

    struct Vec3 {
      float x, y, z;

      Vec3() : x(0), y(0), z(0) {}
      Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

      Vec3 operator+(const Vec3 &other) const;
      Vec3 operator-(const Vec3 &other) const;
      Vec3 operator*(float scalar) const;
      bool operator==(const Vec3 &other) const;

      float dot(const Vec3 &other) const;
      Vec3 cross(const Vec3 &other) const;
      float get_length() const;
      Vec3 get_normalized() const;

      // Backward compatibility aliases
      float dot_product(const Vec3 &other) const { return dot(other); }
      Vec3 cross_product(const Vec3 &other) const { return cross(other); }
    };

    struct Vec4 {
      float x, y, z, w;

      Vec4() : x(0), y(0), z(0), w(0) {}
      Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

      Vec4 operator+(const Vec4 &other) const;
      Vec4 operator-(const Vec4 &other) const;
      Vec4 operator*(float scalar) const;
      bool operator==(const Vec4 &other) const;

      float dot(const Vec4 &other) const;
      float get_length() const;
      Vec4 get_normalized() const;
    };

  } // namespace geometry
} // namespace Q