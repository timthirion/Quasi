/// @file camera.hpp
/// @brief Camera for raytracing.

#pragma once

#include <quasi/math/vec.hpp>
#include <quasi/math/ray.hpp>

#include <cmath>

namespace Q::scene {

/// @brief A simple perspective camera.
struct camera {
    math::vec3 position  = {0.0f, 0.0f, 0.0f};
    math::vec3 direction = {0.0f, 0.0f, -1.0f};  // Forward direction.
    math::vec3 up        = {0.0f, 1.0f, 0.0f};   // World up.
    float fov            = 60.0f;                 // Vertical FOV in degrees.
    float aspect         = 16.0f / 9.0f;          // Width / height.

    constexpr camera() = default;

    /// @brief Creates a camera looking at a target point.
    static camera look_at(math::vec3 from, math::vec3 at, math::vec3 world_up = {0, 1, 0}) {
        camera cam;
        cam.position = from;
        cam.direction = math::normalize(at - from);
        cam.up = world_up;
        return cam;
    }

    /// @brief Generates a ray for the given normalized screen coordinates.
    /// @param u Horizontal coordinate [0, 1], left to right.
    /// @param v Vertical coordinate [0, 1], bottom to top.
    /// @return Ray from camera through the screen point.
    [[nodiscard]] math::ray get_ray(float u, float v) const {
        float theta = fov * 3.14159265359f / 180.0f;
        float h = std::tan(theta / 2.0f);
        float viewport_height = 2.0f * h;
        float viewport_width = aspect * viewport_height;

        math::vec3 w = math::normalize(-direction);
        math::vec3 right = math::normalize(math::cross(up, w));
        math::vec3 cam_up = math::cross(w, right);

        math::vec3 horizontal = viewport_width * right;
        math::vec3 vertical = viewport_height * cam_up;
        math::vec3 lower_left = position - horizontal * 0.5f - vertical * 0.5f - w;

        math::vec3 target = lower_left + u * horizontal + v * vertical;
        return {position, math::normalize(target - position)};
    }
};

}  // namespace Q::scene
