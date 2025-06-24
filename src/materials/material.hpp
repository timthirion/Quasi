#pragma once

#include "../radiometry/color.hpp"

namespace Q {
  namespace materials {

    // Using alias for commonly used type
    using Color = Q::radiometry::Color;

    /**
     * Base material class defining surface properties for lighting calculations.
     */
    class Material {
    public:
      virtual ~Material() = default;

      // Get diffuse color at a surface point (u, v are texture coordinates)
      virtual Color diffuse_color(float u = 0.0f, float v = 0.0f) const = 0;

      // Material properties for Phong lighting
      virtual Color ambient_color() const = 0;
      virtual Color specular_color() const = 0;
      virtual float shininess() const = 0;

      // Reflection properties
      virtual float reflectance() const = 0; // [0,1] how reflective the surface is

      // Backward compatibility aliases
      Color get_diffuse_color(float u = 0.0f, float v = 0.0f) const { return diffuse_color(u, v); }
      Color get_ambient_color() const { return ambient_color(); }
      Color get_specular_color() const { return specular_color(); }
      float get_shininess() const { return shininess(); }
      float get_reflectance() const { return reflectance(); }
    };

    /**
     * Solid color material - uniform color across the entire surface.
     */
    class SolidMaterial : public Material {
    private:
      Color diffuse_color_;
      Color ambient_color_;
      Color specular_color_;
      float shininess_;
      float reflectance_;

    public:
      SolidMaterial(const Color &diffuse, const Color &ambient = Color(0.1f, 0.1f, 0.1f),
                    const Color &specular = Color(0.3f, 0.3f, 0.3f), float shine = 32.0f,
                    float reflect = 0.0f)
          : diffuse_color_(diffuse), ambient_color_(ambient), specular_color_(specular),
            shininess_(shine), reflectance_(reflect) {}

      // Convenience constructor for diffuse color and reflectance only
      SolidMaterial(const Color &diffuse, float reflect)
          : diffuse_color_(diffuse), ambient_color_(Color(0.1f, 0.1f, 0.1f)),
            specular_color_(Color(0.3f, 0.3f, 0.3f)), shininess_(32.0f), reflectance_(reflect) {}

      Color diffuse_color(float u = 0.0f, float v = 0.0f) const override { return diffuse_color_; }

      Color ambient_color() const override { return ambient_color_; }

      Color specular_color() const override { return specular_color_; }

      float shininess() const override { return shininess_; }

      float reflectance() const override { return reflectance_; }
    };

  } // namespace materials
} // namespace Q