/// @file scene.hpp
/// @brief Scene container and convenience header.

#pragma once

#include <quasi/scene/camera.hpp>
#include <quasi/scene/material.hpp>
#include <quasi/scene/sphere.hpp>

#include <vector>

namespace Q::scene {

/// @brief An object in the scene (sphere + material).
struct object {
    sphere geometry;
    material mat;
};

/// @brief A complete scene description.
struct scene {
    camera cam;
    std::vector<object> objects;
    math::vec3 background_color = {0.1f, 0.1f, 0.1f};

    /// @brief Adds a sphere with a material to the scene.
    void add(sphere s, material m = {}) {
        objects.push_back({s, m});
    }
};

/// @brief Creates a simple test scene with a red sphere.
inline scene make_test_scene(float aspect = 16.0f / 9.0f) {
    scene s;

    s.cam = camera::look_at(
        {0.0f, 0.0f, 3.0f},   // Camera position.
        {0.0f, 0.0f, 0.0f}    // Look at origin.
    );
    s.cam.aspect = aspect;

    s.add(
        sphere{{0.0f, 0.0f, 0.0f}, 1.0f},
        materials::red()
    );

    s.background_color = {0.2f, 0.3f, 0.5f};

    return s;
}

}  // namespace Q::scene
