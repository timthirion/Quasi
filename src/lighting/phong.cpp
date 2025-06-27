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
        if (light->is_area_light()) {
          // Handle area lights with multiple samples for soft shadows
          // Use the light's configured sample count for proper soft shadows
          auto area_light = std::dynamic_pointer_cast<RectangularAreaLight>(light);
          const int num_samples = area_light ? area_light->default_samples() : 16;
          auto samples = light->generate_samples(surface_point, num_samples);

          Color total_diffuse(0.0f, 0.0f, 0.0f);
          Color total_specular(0.0f, 0.0f, 0.0f);
          float shadow_factor = 0.0f; // Accumulate shadow coverage

          for (const auto &sample : samples) {
            // Check shadow for this sample
            bool sample_in_shadow = false;
            if (shadow_test) {
              sample_in_shadow = shadow_test(surface_point, sample.direction, sample.distance);
            }

            if (!sample_in_shadow) {
              shadow_factor += sample.weight;

              // Add weighted diffuse contribution
              Color diffuse =
                  calculate_diffuse(sample.direction, surface_normal, sample.intensity, material);
              total_diffuse = total_diffuse + diffuse * sample.weight;

              // Add weighted specular contribution
              Color specular = calculate_specular(sample.direction, surface_normal, view_direction,
                                                  sample.intensity, material);
              total_specular = total_specular + specular * sample.weight;
            }
          }

          // Add accumulated contributions (already weighted by shadow factor)
          final_color = final_color + total_diffuse + total_specular;

        } else {
          // Handle point lights with single sample (existing behavior)
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
      }

      // Only clamp negative values (allow HDR values > 1.0)
      final_color.r = std::max(0.0f, final_color.r);
      final_color.g = std::max(0.0f, final_color.g);
      final_color.b = std::max(0.0f, final_color.b);

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