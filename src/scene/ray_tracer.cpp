#include "ray_tracer.hpp"
#include <stack>

namespace Q {
  namespace scene {

    Q::radiometry::Color
    RayTracer::trace_ray_with_reflections(const Q::geometry::Ray &initial_ray) {
      Q::radiometry::Color final_color(0.0f, 0.0f, 0.0f);
      std::stack<RayBounce> ray_stack;

      // Start with the initial ray
      ray_stack.push(RayBounce(initial_ray, Q::radiometry::Color(1.0f, 1.0f, 1.0f), 0));

      while (!ray_stack.empty()) {
        RayBounce current = ray_stack.top();
        ray_stack.pop();

        // Stop if we've reached maximum depth
        if (current.depth >= max_depth) {
          continue;
        }

        // Get the direct lighting contribution for this ray
        Q::radiometry::Color direct_color = scene.trace_ray(current.ray);

        // Find intersection to check for reflections
        auto intersection = scene.find_closest_intersection(current.ray);
        if (!intersection.has_value()) {
          // Ray hit background, add attenuated background color
          final_color = final_color + direct_color * current.attenuation;
          continue;
        }

        const auto &hit = intersection.value();

        // Get material properties
        float reflectance = 0.0f;
        if (hit.material) {
          reflectance = hit.material->get_reflectance();
        }

        // Add direct lighting contribution (attenuated by material reflectance)
        float direct_contribution = 1.0f - reflectance;
        final_color = final_color + direct_color * current.attenuation * direct_contribution;

        // Add reflection ray if material is reflective
        if (reflectance > 0.0001f && current.depth < max_depth - 1) {
          Q::geometry::Ray reflection_ray =
              Q::geometry::compute_reflection_ray(current.ray, hit.point, hit.normal);

          // Attenuate reflection by reflectance and current attenuation
          Q::radiometry::Color reflection_attenuation = current.attenuation * reflectance;

          ray_stack.push(RayBounce(reflection_ray, reflection_attenuation, current.depth + 1));
        }
      }

      return final_color;
    }

  } // namespace scene
} // namespace Q