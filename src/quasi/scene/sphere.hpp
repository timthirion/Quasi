/// @file sphere.hpp
/// @brief Sphere primitive for raytracing.

#pragma once

#include <quasi/math/vec.hpp>
#include <quasi/math/ray.hpp>

#include <cmath>
#include <optional>

namespace Q::scene {

/// @brief A sphere defined by center and radius.
struct sphere {
    math::vec3 center = {0.0f, 0.0f, 0.0f};
    float radius      = 1.0f;

    constexpr sphere() = default;
    constexpr sphere(math::vec3 center, float radius)
        : center{center}, radius{radius} {}
};

/// @brief Result of a ray-sphere intersection.
struct hit_record {
    float t;              // Ray parameter at hit point.
    math::vec3 point;     // World-space hit point.
    math::vec3 normal;    // Surface normal at hit (facing ray origin).
    bool front_face;      // True if ray hit from outside.
};

/// @brief Tests ray-sphere intersection.
/// @param r The ray to test.
/// @param s The sphere to test against.
/// @param t_min Minimum valid t value.
/// @param t_max Maximum valid t value.
/// @return Hit record if intersection found, nullopt otherwise.
inline std::optional<hit_record> intersect(
    const math::ray& r,
    const sphere& s,
    float t_min = 0.001f,
    float t_max = 1e30f
) {
    math::vec3 oc = r.origin - s.center;

    float a = math::length_squared(r.direction);
    float half_b = math::dot(oc, r.direction);
    float c = math::length_squared(oc) - s.radius * s.radius;

    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0f) {
        return std::nullopt;
    }

    float sqrtd = std::sqrt(discriminant);

    // Find nearest root in acceptable range.
    float root = (-half_b - sqrtd) / a;
    if (root < t_min || root > t_max) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || root > t_max) {
            return std::nullopt;
        }
    }

    hit_record rec;
    rec.t = root;
    rec.point = r.at(root);

    math::vec3 outward_normal = (rec.point - s.center) / s.radius;
    rec.front_face = math::dot(r.direction, outward_normal) < 0.0f;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;

    return rec;
}

}  // namespace Q::scene
