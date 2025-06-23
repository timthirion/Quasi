#pragma once

#include "../geometry/geometry.hpp"
#include "../io/scene_parser.hpp"
#include "../materials/checkerboard_texture.hpp"
#include "../radiometry/camera.hpp"
#include "../radiometry/color.hpp"
#include <memory>
#include <vector>

namespace Q {
  namespace scene {

    struct ColoredSphere {
      Q::geometry::Sphere sphere;
      Q::radiometry::Color color;

      ColoredSphere(const Q::geometry::Sphere &s, const Q::radiometry::Color &c)
          : sphere(s), color(c) {}
    };

    struct TexturedTriangle {
      Q::geometry::Triangle triangle;
      Q::geometry::Vec3 uv0, uv1, uv2; // UV coordinates for each vertex (z component unused)
      Q::materials::CheckerboardTexture *texture;

      TexturedTriangle(const Q::geometry::Triangle &tri, const Q::geometry::Vec3 &uv0,
                       const Q::geometry::Vec3 &uv1, const Q::geometry::Vec3 &uv2,
                       Q::materials::CheckerboardTexture *tex)
          : triangle(tri), uv0(uv0), uv1(uv1), uv2(uv2), texture(tex) {}
    };

    class Scene {
    private:
      std::vector<ColoredSphere> spheres;
      std::vector<TexturedTriangle> background_triangles;
      std::unique_ptr<Q::materials::CheckerboardTexture> background_texture;

    public:
      // Default constructor (creates empty scene)
      Scene();

      // Constructor from scene data
      Scene(const Q::io::SceneData &scene_data);

      // Constructor from JSON file
      static Scene from_file(const std::string &filename);

      void add_sphere(const Q::geometry::Sphere &sphere, const Q::radiometry::Color &color);
      Q::radiometry::Color trace_ray(const Q::geometry::Ray &ray) const;

    private:
      void setup_background(const Q::io::BackgroundSettings &bg_settings,
                            const Q::io::SceneCamera &camera_settings,
                            const Q::io::RenderSettings &render_settings);
    };

  } // namespace scene
} // namespace Q