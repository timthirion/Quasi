#pragma once

#include "../radiometry/color.hpp"

namespace Q {
  namespace materials {

    /**
     * Base material class defining surface properties for lighting calculations.
     */
    class Material {
    public:
      virtual ~Material() = default;

      // Get diffuse color at a surface point (u, v are texture coordinates)
      virtual Q::radiometry::Color get_diffuse_color(float u = 0.0f, float v = 0.0f) const = 0;

      // Material properties for Phong lighting
      virtual Q::radiometry::Color get_ambient_color() const = 0;
      virtual Q::radiometry::Color get_specular_color() const = 0;
      virtual float get_shininess() const = 0;

      // Reflection properties
      virtual float get_reflectance() const = 0; // [0,1] how reflective the surface is
    };

    /**
     * Solid color material - uniform color across the entire surface.
     */
    class SolidMaterial : public Material {
    private:
      Q::radiometry::Color diffuse_color;
      Q::radiometry::Color ambient_color;
      Q::radiometry::Color specular_color;
      float shininess;
      float reflectance;

    public:
      SolidMaterial(const Q::radiometry::Color &diffuse,
                    const Q::radiometry::Color &ambient = Q::radiometry::Color(0.1f, 0.1f, 0.1f),
                    const Q::radiometry::Color &specular = Q::radiometry::Color(0.3f, 0.3f, 0.3f),
                    float shine = 32.0f, float reflect = 0.0f)
          : diffuse_color(diffuse), ambient_color(ambient), specular_color(specular),
            shininess(shine), reflectance(reflect) {}

      // Convenience constructor for diffuse color and reflectance only
      SolidMaterial(const Q::radiometry::Color &diffuse, float reflect)
          : diffuse_color(diffuse), ambient_color(Q::radiometry::Color(0.1f, 0.1f, 0.1f)),
            specular_color(Q::radiometry::Color(0.3f, 0.3f, 0.3f)), shininess(32.0f),
            reflectance(reflect) {}

      Q::radiometry::Color get_diffuse_color(float u = 0.0f, float v = 0.0f) const override {
        return diffuse_color;
      }

      Q::radiometry::Color get_ambient_color() const override { return ambient_color; }

      Q::radiometry::Color get_specular_color() const override { return specular_color; }

      float get_shininess() const override { return shininess; }

      float get_reflectance() const override { return reflectance; }
    };

  } // namespace materials
} // namespace Q