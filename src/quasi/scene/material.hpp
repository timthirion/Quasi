/// @file material.hpp
/// @brief Material definitions for rendering.

#pragma once

#include <quasi/math/vec.hpp>

namespace Q::scene {

/// @brief Simple material for raytracing.
struct material {
    math::vec3 albedo   = {1.0f, 1.0f, 1.0f};  // Base color.
    float roughness     = 0.5f;                 // 0 = mirror, 1 = diffuse.
    math::vec3 emission = {0.0f, 0.0f, 0.0f};  // Emissive color.
    float metallic      = 0.0f;                 // 0 = dielectric, 1 = metal.
};

/// @brief Predefined materials.
namespace materials {

inline constexpr material red() {
    return {.albedo = {1.0f, 0.2f, 0.2f}, .roughness = 0.3f};
}

inline constexpr material white() {
    return {.albedo = {1.0f, 1.0f, 1.0f}, .roughness = 0.5f};
}

inline constexpr material metal() {
    return {.albedo = {0.8f, 0.8f, 0.9f}, .roughness = 0.1f, .emission = {}, .metallic = 1.0f};
}

inline constexpr material green() {
    return {.albedo = {0.12f, 0.45f, 0.15f}, .roughness = 1.0f};
}

inline constexpr material light(float intensity = 15.0f) {
    return {.albedo = {0.0f, 0.0f, 0.0f}, .roughness = 1.0f, .emission = {intensity, intensity, intensity}};
}

}  // namespace materials

}  // namespace Q::scene
