/// @file cornell_box.hpp
/// @brief Cornell Box scene factory.

#pragma once

#include <quasi/scene/quad.hpp>
#include <quasi/scene/camera.hpp>
#include <quasi/scene/material.hpp>

#include <cmath>
#include <vector>

namespace Q::scene {

/// @brief A quad with its material.
struct quad_object {
    quad geometry;
    material mat;
};

/// @brief Cornell Box scene description.
struct cornell_box_scene {
    camera cam;
    std::vector<quad_object> quads;
    math::vec3 background_color = {0.0f, 0.0f, 0.0f};
    size_t light_index = 0;
};

namespace detail {

/// @brief Rotates a point around the Y axis.
inline math::vec3 rotate_y(math::vec3 p, float angle_deg) {
    float rad = angle_deg * 3.14159265359f / 180.0f;
    float c = std::cos(rad);
    float s = std::sin(rad);
    return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
}

/// @brief Adds a box to the scene as 6 quads.
/// @param scene The scene to add quads to.
/// @param center Center of the box base (y=0 of box).
/// @param size Dimensions (width, height, depth).
/// @param angle_y Rotation around Y axis in degrees.
/// @param mat Material for all faces.
inline void add_box(
    cornell_box_scene& scene,
    math::vec3 center,
    math::vec3 size,
    float angle_y,
    material mat
) {
    float hw = size.x * 0.5f;  // half width
    float h  = size.y;         // height
    float hd = size.z * 0.5f;  // half depth

    // Local corners of the box (before rotation, at origin).
    // Bottom: 0-3, Top: 4-7
    //   6---7
    //  /|  /|
    // 4---5 |
    // | 2-|-3
    // |/  |/
    // 0---1
    math::vec3 local[8] = {
        {-hw, 0, -hd}, { hw, 0, -hd}, {-hw, 0,  hd}, { hw, 0,  hd},  // bottom
        {-hw, h, -hd}, { hw, h, -hd}, {-hw, h,  hd}, { hw, h,  hd},  // top
    };

    // Rotate and translate.
    math::vec3 p[8];
    for (int i = 0; i < 8; i++) {
        p[i] = rotate_y(local[i], angle_y) + center;
    }

    // Add 5 visible faces (skip bottom, it sits on floor).
    // Top face (4, 5, 6) - u = 5-4, v = 6-4
    scene.quads.push_back({{p[4], p[5] - p[4], p[6] - p[4]}, mat});

    // Front face (0, 1, 4) - facing -Z after rotation
    scene.quads.push_back({{p[0], p[1] - p[0], p[4] - p[0]}, mat});

    // Back face (3, 2, 7) - facing +Z after rotation
    scene.quads.push_back({{p[3], p[2] - p[3], p[7] - p[3]}, mat});

    // Left face (2, 0, 6) - facing -X after rotation
    scene.quads.push_back({{p[2], p[0] - p[2], p[6] - p[2]}, mat});

    // Right face (1, 3, 5) - facing +X after rotation
    scene.quads.push_back({{p[1], p[3] - p[1], p[5] - p[1]}, mat});
}

}  // namespace detail

/// @brief Creates the standard Cornell Box scene.
inline cornell_box_scene make_cornell_box(float aspect = 1.0f) {
    cornell_box_scene scene;

    material white = {.albedo = {0.73f, 0.73f, 0.73f}, .roughness = 1.0f};
    material red   = {.albedo = {0.65f, 0.05f, 0.05f}, .roughness = 1.0f};
    material green = {.albedo = {0.12f, 0.45f, 0.15f}, .roughness = 1.0f};
    material light = {.albedo = {}, .roughness = 1.0f, .emission = {15.0f, 15.0f, 15.0f}};

    // Floor: Y=0 plane, corners at (-1,0,-1) to (1,0,1)
    scene.quads.push_back({{{-1, 0, -1}, {2, 0, 0}, {0, 0, 2}}, white});

    // Ceiling: Y=2 plane
    scene.quads.push_back({{{-1, 2, 1}, {2, 0, 0}, {0, 0, -2}}, white});

    // Back wall: Z=-1 plane
    scene.quads.push_back({{{-1, 0, -1}, {2, 0, 0}, {0, 2, 0}}, white});

    // Left wall: X=-1 plane (RED)
    scene.quads.push_back({{{-1, 0, 1}, {0, 0, -2}, {0, 2, 0}}, red});

    // Right wall: X=1 plane (GREEN)
    scene.quads.push_back({{{1, 0, -1}, {0, 0, 2}, {0, 2, 0}}, green});

    // Light on ceiling
    scene.light_index = scene.quads.size();
    scene.quads.push_back({{{-0.25f, 1.99f, -0.25f}, {0.5f, 0, 0}, {0, 0, 0.5f}}, light});

    // Tall box (left side, rotated 15 degrees)
    detail::add_box(scene, {-0.35f, 0.0f, 0.3f}, {0.5f, 1.2f, 0.5f}, 15.0f, white);

    // Short box (right side, rotated -18 degrees)
    detail::add_box(scene, {0.35f, 0.0f, -0.3f}, {0.55f, 0.55f, 0.55f}, -18.0f, white);

    // Camera
    scene.cam = camera::look_at({0, 1, 3.5f}, {0, 1, 0});
    scene.cam.fov = 40.0f;
    scene.cam.aspect = aspect;

    return scene;
}

}  // namespace Q::scene
