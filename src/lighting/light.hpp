#pragma once

#include "../geometry/vec3.hpp"
#include "../radiometry/color.hpp"

namespace Q {
  namespace lighting {

    /**
     * Base class for light sources.
     */
    class Light {
    public:
      virtual ~Light() = default;

      // Get light direction from a surface point to the light
      virtual Q::geometry::Vec3
      get_direction_to_light(const Q::geometry::Vec3 &surface_point) const = 0;

      // Get light intensity at a surface point
      virtual Q::radiometry::Color get_intensity(const Q::geometry::Vec3 &surface_point) const = 0;

      // Get distance to light (for shadow testing)
      virtual float get_distance(const Q::geometry::Vec3 &surface_point) const = 0;
    };

    /**
     * Point light source with position and intensity.
     */
    class PointLight : public Light {
    private:
      Q::geometry::Vec3 position;
      Q::radiometry::Color intensity;
      float attenuation_constant;
      float attenuation_linear;
      float attenuation_quadratic;

    public:
      PointLight(const Q::geometry::Vec3 &pos, const Q::radiometry::Color &color,
                 float constant = 1.0f, float linear = 0.0f, float quadratic = 0.0f)
          : position(pos), intensity(color), attenuation_constant(constant),
            attenuation_linear(linear), attenuation_quadratic(quadratic) {}

      Q::geometry::Vec3
      get_direction_to_light(const Q::geometry::Vec3 &surface_point) const override {
        return (position - surface_point).get_normalized();
      }

      Q::radiometry::Color get_intensity(const Q::geometry::Vec3 &surface_point) const override {
        float distance = get_distance(surface_point);
        float attenuation = attenuation_constant + attenuation_linear * distance +
                            attenuation_quadratic * distance * distance;

        return intensity * (1.0f / attenuation);
      }

      float get_distance(const Q::geometry::Vec3 &surface_point) const override {
        return (position - surface_point).get_length();
      }

      const Q::geometry::Vec3 &get_position() const { return position; }
    };

  } // namespace lighting
} // namespace Q