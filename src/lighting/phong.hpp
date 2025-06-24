#pragma once

#include "../geometry/vec.hpp"
#include "../materials/material.hpp"
#include "../radiometry/color.hpp"
#include "light.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace Q {
  namespace lighting {

    // Using aliases for commonly used types
    using Vec3 = Q::geometry::Vec3;
    using Color = Q::radiometry::Color;

    /**
     * Phong lighting model implementation.
     * Combines ambient, diffuse, and specular lighting components.
     */
    class PhongLighting {
    public:
      /**
       * Calculate lighting at a surface point using the Phong model.
       *
       * @param surface_point The point on the surface being lit
       * @param surface_normal The surface normal at the point (should be normalized)
       * @param view_direction Direction from surface to viewer (should be normalized)
       * @param material The material properties of the surface
       * @param lights List of lights affecting the surface
       * @param shadow_test Function to test if a point is in shadow from a light
       * @return The final color after applying Phong lighting
       */
      static Color calculate_lighting(
          const Vec3 &surface_point, const Vec3 &surface_normal, const Vec3 &view_direction,
          const Q::materials::Material &material, const std::vector<std::shared_ptr<Light>> &lights,
          std::function<bool(const Vec3 &, const Vec3 &, float)> shadow_test = nullptr);

    private:
      static Color calculate_ambient(const Q::materials::Material &material);

      static Color calculate_diffuse(const Vec3 &light_direction, const Vec3 &surface_normal,
                                     const Color &light_intensity,
                                     const Q::materials::Material &material);

      static Color calculate_specular(const Vec3 &light_direction, const Vec3 &surface_normal,
                                      const Vec3 &view_direction, const Color &light_intensity,
                                      const Q::materials::Material &material);
    };

  } // namespace lighting
} // namespace Q