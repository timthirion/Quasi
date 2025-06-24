#include "phong.hpp"
#include <algorithm>
#include <cmath>

namespace Q {
  namespace lighting {

    // Using aliases for commonly used types
    using Vec3 = Q::geometry::Vec3;
    using Color = Q::radiometry::Color;

    Color PhongLighting::calculate_lighting(
        const Vec3 &surface_point, const Vec3 &surface_normal, const Vec3 &view_direction,
        const Q::materials::Material &material, const std::vector<std::shared_ptr<Light>> &lights,
        std::function<bool(const Vec3 &, const Vec3 &, float)> shadow_test) {
      // Start with ambient lighting
      Color final_color = calculate_ambient(material);

      // Add contribution from each light
      for (const auto &light : lights) {
        Vec3 light_direction = light->get_direction_to_light(surface_point);
        Color light_intensity = light->get_intensity(surface_point);

        // Check for shadows if shadow test function is provided
        bool in_shadow = false;
        if (shadow_test) {
          float light_distance = light->get_distance(surface_point);
          in_shadow = shadow_test(surface_point, light_direction, light_distance);
        }

        if (!in_shadow) {
          // Add diffuse contribution
          Color diffuse =
              calculate_diffuse(light_direction, surface_normal, light_intensity, material);
          final_color = final_color + diffuse;

          // Add specular contribution
          Color specular = calculate_specular(light_direction, surface_normal, view_direction,
                                              light_intensity, material);
          final_color = final_color + specular;
        }
      }

      // Clamp to valid color range [0, 1]
      final_color.r = std::max(0.0f, std::min(1.0f, final_color.r));
      final_color.g = std::max(0.0f, std::min(1.0f, final_color.g));
      final_color.b = std::max(0.0f, std::min(1.0f, final_color.b));

      return final_color;
    }

    Color PhongLighting::calculate_ambient(const Q::materials::Material &material) {
      return material.ambient_color();
    }

    Color PhongLighting::calculate_diffuse(const Vec3 &light_direction, const Vec3 &surface_normal,
                                           const Color &light_intensity,
                                           const Q::materials::Material &material) {
      // Lambert's law: diffuse intensity = dot(normal, light_direction)
      float diffuse_factor = std::max(0.0f, surface_normal.dot(light_direction));

      Color diffuse_color = material.diffuse_color();

      return Color(diffuse_color.r * light_intensity.r * diffuse_factor,
                   diffuse_color.g * light_intensity.g * diffuse_factor,
                   diffuse_color.b * light_intensity.b * diffuse_factor);
    }

    Color PhongLighting::calculate_specular(const Vec3 &light_direction, const Vec3 &surface_normal,
                                            const Vec3 &view_direction,
                                            const Color &light_intensity,
                                            const Q::materials::Material &material) {
      // Calculate reflection vector: R = 2(N·L)N - L
      float nl_dot = surface_normal.dot(light_direction);
      Vec3 reflection = surface_normal * (2.0f * nl_dot) - light_direction;

      // Specular factor = (R·V)^shininess
      float rv_dot = std::max(0.0f, reflection.dot(view_direction));
      float specular_factor = std::pow(rv_dot, material.shininess());

      Color specular_color = material.specular_color();

      return Color(specular_color.r * light_intensity.r * specular_factor,
                   specular_color.g * light_intensity.g * specular_factor,
                   specular_color.b * light_intensity.b * specular_factor);
    }

  } // namespace lighting
} // namespace Q