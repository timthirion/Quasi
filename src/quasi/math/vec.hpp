/// @file vec.hpp
/// @brief Vector types for 3D math.
///
/// Plain structs suitable for GPU buffer layouts. These mirror
/// typical shader vec2/vec3/vec4 types.

#pragma once

#include <cmath>

namespace Q::math {

struct vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr vec2() = default;
    constexpr vec2(float x, float y) : x{x}, y{y} {}
    constexpr explicit vec2(float s) : x{s}, y{s} {}

    constexpr vec2 operator+(vec2 v) const { return {x + v.x, y + v.y}; }
    constexpr vec2 operator-(vec2 v) const { return {x - v.x, y - v.y}; }
    constexpr vec2 operator*(float s) const { return {x * s, y * s}; }
    constexpr vec2 operator/(float s) const { return {x / s, y / s}; }
    constexpr vec2 operator-() const { return {-x, -y}; }

    constexpr vec2& operator+=(vec2 v) { x += v.x; y += v.y; return *this; }
    constexpr vec2& operator-=(vec2 v) { x -= v.x; y -= v.y; return *this; }
    constexpr vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    constexpr vec2& operator/=(float s) { x /= s; y /= s; return *this; }
};

struct vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr vec3() = default;
    constexpr vec3(float x, float y, float z) : x{x}, y{y}, z{z} {}
    constexpr explicit vec3(float s) : x{s}, y{s}, z{s} {}

    constexpr vec3 operator+(vec3 v) const { return {x + v.x, y + v.y, z + v.z}; }
    constexpr vec3 operator-(vec3 v) const { return {x - v.x, y - v.y, z - v.z}; }
    constexpr vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    constexpr vec3 operator*(vec3 v) const { return {x * v.x, y * v.y, z * v.z}; }
    constexpr vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    constexpr vec3 operator-() const { return {-x, -y, -z}; }

    constexpr vec3& operator+=(vec3 v) { x += v.x; y += v.y; z += v.z; return *this; }
    constexpr vec3& operator-=(vec3 v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    constexpr vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    constexpr vec3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }
};

struct vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    constexpr vec4() = default;
    constexpr vec4(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} {}
    constexpr vec4(vec3 v, float w) : x{v.x}, y{v.y}, z{v.z}, w{w} {}
    constexpr explicit vec4(float s) : x{s}, y{s}, z{s}, w{s} {}

    constexpr vec3 xyz() const { return {x, y, z}; }

    constexpr vec4 operator+(vec4 v) const { return {x + v.x, y + v.y, z + v.z, w + v.w}; }
    constexpr vec4 operator-(vec4 v) const { return {x - v.x, y - v.y, z - v.z, w - v.w}; }
    constexpr vec4 operator*(float s) const { return {x * s, y * s, z * s, w * s}; }
    constexpr vec4 operator/(float s) const { return {x / s, y / s, z / s, w / s}; }
    constexpr vec4 operator-() const { return {-x, -y, -z, -w}; }
};

// Free functions

constexpr vec2 operator*(float s, vec2 v) { return v * s; }
constexpr vec3 operator*(float s, vec3 v) { return v * s; }
constexpr vec4 operator*(float s, vec4 v) { return v * s; }

constexpr float dot(vec2 a, vec2 b) {
    return a.x * b.x + a.y * b.y;
}

constexpr float dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr float dot(vec4 a, vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

constexpr vec3 cross(vec3 a, vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

constexpr float length_squared(vec2 v) { return dot(v, v); }
constexpr float length_squared(vec3 v) { return dot(v, v); }
constexpr float length_squared(vec4 v) { return dot(v, v); }

inline float length(vec2 v) { return std::sqrt(length_squared(v)); }
inline float length(vec3 v) { return std::sqrt(length_squared(v)); }
inline float length(vec4 v) { return std::sqrt(length_squared(v)); }

inline vec2 normalize(vec2 v) { return v / length(v); }
inline vec3 normalize(vec3 v) { return v / length(v); }
inline vec4 normalize(vec4 v) { return v / length(v); }

constexpr vec3 reflect(vec3 v, vec3 n) {
    return v - 2.0f * dot(v, n) * n;
}

template <typename T>
constexpr T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

template <typename T>
constexpr T clamp(T x, T min_val, T max_val) {
    if (x < min_val) return min_val;
    if (x > max_val) return max_val;
    return x;
}

}  // namespace Q::math
