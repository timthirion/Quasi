/// @file quad.hpp
/// @brief Quad primitive for raytracing (parallelogram defined by corner + two edges).

#pragma once

#include <quasi/math/vec.hpp>
#include <quasi/math/ray.hpp>

#include <cmath>
#include <optional>

namespace Q::scene {

/// @brief A quad (parallelogram) defined by a corner point and two edge vectors.
struct quad {
    math::vec3 origin = {0.0f, 0.0f, 0.0f};  // Corner position.
    math::vec3 u      = {1.0f, 0.0f, 0.0f};  // First edge vector.
    math::vec3 v      = {0.0f, 1.0f, 0.0f};  // Second edge vector.

    constexpr quad() = default;
    constexpr quad(math::vec3 origin, math::vec3 u, math::vec3 v)
        : origin{origin}, u{u}, v{v} {}

    /// @brief Computes the unit normal of the quad.
    [[nodiscard]] math::vec3 normal() const {
        return math::normalize(math::cross(u, v));
    }

    /// @brief Computes the area of the quad.
    [[nodiscard]] float area() const {
        return math::length(math::cross(u, v));
    }
};

/// @brief Result of a ray-quad intersection.
struct quad_hit_record {
    float t;              // Ray parameter at hit point.
    math::vec3 point;     // World-space hit point.
    math::vec3 normal;    // Surface normal at hit.
    bool front_face;      // True if ray hit from front.
    float u_coord;        // Parametric u coordinate [0,1].
    float v_coord;        // Parametric v coordinate [0,1].
};

/// @brief Tests ray-quad intersection.
/// @param r The ray to test.
/// @param q The quad to test against.
/// @param t_min Minimum valid t value.
/// @param t_max Maximum valid t value.
/// @return Hit record if intersection found, nullopt otherwise.
inline std::optional<quad_hit_record> intersect(
    const math::ray& r,
    const quad& q,
    float t_min = 0.001f,
    float t_max = 1e30f
) {
    // Compute plane normal and d coefficient.
    math::vec3 n = math::cross(q.u, q.v);
    float area_sq = math::length_squared(n);

    // Degenerate quad check.
    if (area_sq < 1e-8f) {
        return std::nullopt;
    }

    math::vec3 normal = n / std::sqrt(area_sq);
    float d = math::dot(normal, q.origin);

    // Check if ray is parallel to plane.
    float denom = math::dot(normal, r.direction);
    if (std::abs(denom) < 1e-8f) {
        return std::nullopt;
    }

    // Compute t parameter.
    float t = (d - math::dot(normal, r.origin)) / denom;
    if (t < t_min || t > t_max) {
        return std::nullopt;
    }

    // Compute hit point.
    math::vec3 p = r.at(t);
    math::vec3 planar = p - q.origin;

    // Project onto quad's local coordinates using the inverse of [u, v, n] matrix.
    // We use the formula: alpha = (n x v) . planar / (n . (u x v))
    //                     beta  = (u x n) . planar / (n . (u x v))
    // Since n = u x v, we have n . (u x v) = |u x v|^2 = area_sq

    math::vec3 w = n / area_sq;  // n / |n|^2

    float alpha = math::dot(math::cross(w, q.v), planar);
    float beta  = math::dot(math::cross(q.u, w), planar);

    // Check if point is inside quad.
    if (alpha < 0.0f || alpha > 1.0f || beta < 0.0f || beta > 1.0f) {
        return std::nullopt;
    }

    quad_hit_record rec;
    rec.t = t;
    rec.point = p;
    rec.u_coord = alpha;
    rec.v_coord = beta;
    rec.front_face = denom < 0.0f;
    rec.normal = rec.front_face ? normal : -normal;

    return rec;
}

}  // namespace Q::scene
