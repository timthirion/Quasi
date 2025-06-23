#include "phong.hpp"
#include <algorithm>
#include <cmath>

namespace Q {
  namespace lighting {

    Q::radiometry::Color PhongLighting::calculate_lighting(
        const Q::geometry::Vec3 &surface_point, const Q::geometry::Vec3 &surface_normal,
        const Q::geometry::Vec3 &view_direction, const Q::materials::Material &material,
        const std::vector<std::shared_ptr<Light>> &lights,
        std::function<bool(const Q::geometry::Vec3 &, const Q::geometry::Vec3 &, float)>
            shadow_test) {
      // Start with ambient lighting
      Q::radiometry::Color final_color = calculate_ambient(material);

      // Add contribution from each light
      for (const auto &light : lights) {
        Q::geometry::Vec3 light_direction = light->get_direction_to_light(surface_point);
        Q::radiometry::Color light_intensity = light->get_intensity(surface_point);

        // Check for shadows if shadow test function is provided
        bool in_shadow = false;
        if (shadow_test) {
          float light_distance = light->get_distance(surface_point);
          in_shadow = shadow_test(surface_point, light_direction, light_distance);
        }

        if (!in_shadow) {
          // Add diffuse contribution
          Q::radiometry::Color diffuse =
              calculate_diffuse(light_direction, surface_normal, light_intensity, material);
          final_color = final_color + diffuse;

          // Add specular contribution
          Q::radiometry::Color specular = calculate_specular(
              light_direction, surface_normal, view_direction, light_intensity, material);
          final_color = final_color + specular;
        }
      }

      // Clamp to valid color range [0, 1]
      final_color.r = std::max(0.0f, std::min(1.0f, final_color.r));
      final_color.g = std::max(0.0f, std::min(1.0f, final_color.g));
      final_color.b = std::max(0.0f, std::min(1.0f, final_color.b));

      return final_color;
    }

    Q::radiometry::Color PhongLighting::calculate_ambient(const Q::materials::Material &material) {
      return material.get_ambient_color();
    }

    Q::radiometry::Color PhongLighting::calculate_diffuse(
        const Q::geometry::Vec3 &light_direction, const Q::geometry::Vec3 &surface_normal,
        const Q::radiometry::Color &light_intensity, const Q::materials::Material &material) {
      // Lambert's law: diffuse intensity = dot(normal, light_direction)
      float diffuse_factor = std::max(0.0f, surface_normal.dot_product(light_direction));

      Q::radiometry::Color diffuse_color = material.get_diffuse_color();

      return Q::radiometry::Color(diffuse_color.r * light_intensity.r * diffuse_factor,
                                  diffuse_color.g * light_intensity.g * diffuse_factor,
                                  diffuse_color.b * light_intensity.b * diffuse_factor);
    }

    Q::radiometry::Color PhongLighting::calculate_specular(
        const Q::geometry::Vec3 &light_direction, const Q::geometry::Vec3 &surface_normal,
        const Q::geometry::Vec3 &view_direction, const Q::radiometry::Color &light_intensity,
        const Q::materials::Material &material) {
      // Calculate reflection vector: R = 2(N·L)N - L
      float nl_dot = surface_normal.dot_product(light_direction);
      Q::geometry::Vec3 reflection = surface_normal * (2.0f * nl_dot) - light_direction;

      // Specular factor = (R·V)^shininess
      float rv_dot = std::max(0.0f, reflection.dot_product(view_direction));
      float specular_factor = std::pow(rv_dot, material.get_shininess());

      Q::radiometry::Color specular_color = material.get_specular_color();

      return Q::radiometry::Color(specular_color.r * light_intensity.r * specular_factor,
                                  specular_color.g * light_intensity.g * specular_factor,
                                  specular_color.b * light_intensity.b * specular_factor);
    }

  } // namespace lighting
} // namespace Q