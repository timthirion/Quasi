/// @file ray.hpp
/// @brief Ray type for raytracing.

#pragma once

#include <quasi/math/vec.hpp>

namespace Q::math {

struct ray {
    vec3 origin;
    vec3 direction;  // Should be normalized.

    constexpr ray() = default;
    constexpr ray(vec3 origin, vec3 direction)
        : origin{origin}, direction{direction} {}

    /// @brief Returns the point at parameter t along the ray.
    constexpr vec3 at(float t) const {
        return origin + direction * t;
    }
};

}  // namespace Q::math
