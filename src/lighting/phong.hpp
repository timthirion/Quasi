#pragma once

#include "../geometry/vec3.hpp"
#include "../materials/material.hpp"
#include "../radiometry/color.hpp"
#include "light.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace Q {
  namespace lighting {

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
      static Q::radiometry::Color calculate_lighting(
          const Q::geometry::Vec3 &surface_point, const Q::geometry::Vec3 &surface_normal,
          const Q::geometry::Vec3 &view_direction, const Q::materials::Material &material,
          const std::vector<std::shared_ptr<Light>> &lights,
          std::function<bool(const Q::geometry::Vec3 &, const Q::geometry::Vec3 &, float)>
              shadow_test = nullptr);

    private:
      static Q::radiometry::Color calculate_ambient(const Q::materials::Material &material);

      static Q::radiometry::Color calculate_diffuse(const Q::geometry::Vec3 &light_direction,
                                                    const Q::geometry::Vec3 &surface_normal,
                                                    const Q::radiometry::Color &light_intensity,
                                                    const Q::materials::Material &material);

      static Q::radiometry::Color calculate_specular(const Q::geometry::Vec3 &light_direction,
                                                     const Q::geometry::Vec3 &surface_normal,
                                                     const Q::geometry::Vec3 &view_direction,
                                                     const Q::radiometry::Color &light_intensity,
                                                     const Q::materials::Material &material);
    };

  } // namespace lighting
} // namespace Q