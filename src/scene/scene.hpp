#pragma once

#include "../geometry/box.hpp"
#include "../geometry/geometry.hpp"
#include "../io/scene_parser.hpp"
#include "../lighting/light.hpp"
#include "../lighting/phong.hpp"
#include "../materials/checkerboard_texture.hpp"
#include "../materials/material.hpp"
#include "../radiometry/camera.hpp"
#include "../radiometry/color.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace Q {
  namespace scene {

    // Using aliases for commonly used types
    using Vec3 = Q::geometry::Vec3;
    using Ray = Q::geometry::Ray;
    using Color = Q::radiometry::Color;

    // Intersection information for ray tracing
    struct Intersection {
      Vec3 point;
      Vec3 normal;
      float distance;
      std::shared_ptr<Q::materials::Material> material;

      Intersection(const Vec3 &p, const Vec3 &n, float d,
                   std::shared_ptr<Q::materials::Material> mat)
          : point(p), normal(n), distance(d), material(mat) {}
    };

    struct ColoredSphere {
      Q::geometry::Sphere sphere;
      std::shared_ptr<Q::materials::Material> material;

      ColoredSphere(const Q::geometry::Sphere &s, std::shared_ptr<Q::materials::Material> mat)
          : sphere(s), material(mat) {}
    };

    struct ColoredBox {
      Q::geometry::Box box;
      std::shared_ptr<Q::materials::Material> material;

      ColoredBox(const Q::geometry::Box &b, std::shared_ptr<Q::materials::Material> mat)
          : box(b), material(mat) {}
    };

    struct ColoredTriangle {
      Q::geometry::Triangle triangle;
      std::shared_ptr<Q::materials::Material> material;

      ColoredTriangle(const Q::geometry::Triangle &t, std::shared_ptr<Q::materials::Material> mat)
          : triangle(t), material(mat) {}
    };

    struct TexturedTriangle {
      Q::geometry::Triangle triangle;
      Vec3 uv0, uv1, uv2; // UV coordinates for each vertex (z component unused)
      Q::materials::CheckerboardTexture *texture;

      TexturedTriangle(const Q::geometry::Triangle &tri, const Vec3 &uv0, const Vec3 &uv1,
                       const Vec3 &uv2, Q::materials::CheckerboardTexture *tex)
          : triangle(tri), uv0(uv0), uv1(uv1), uv2(uv2), texture(tex) {}
    };

    class Scene {
    private:
      std::vector<ColoredSphere> spheres;
      std::vector<ColoredTriangle> triangles;
      std::vector<ColoredBox> boxes;
      std::vector<TexturedTriangle> background_triangles;
      std::vector<std::shared_ptr<Q::lighting::Light>> lights;
      std::unique_ptr<Q::materials::CheckerboardTexture> background_texture;

    public:
      // Default constructor (creates empty scene)
      Scene();

      // Constructor from scene data
      Scene(const Q::io::SceneData &scene_data);

      // Constructor from JSON file
      static Scene from_file(const std::string &filename);

      void add_sphere(const Q::geometry::Sphere &sphere,
                      std::shared_ptr<Q::materials::Material> material);
      void add_triangle(const Q::geometry::Triangle &triangle,
                        std::shared_ptr<Q::materials::Material> material);
      void add_box(const Q::geometry::Box &box, std::shared_ptr<Q::materials::Material> material);
      void add_light(std::shared_ptr<Q::lighting::Light> light);
      Color trace_ray(const Ray &ray) const;

      // Find closest intersection for reflection ray tracing
      std::optional<Intersection> find_closest_intersection(const Ray &ray) const;

    private:
      void setup_background(const Q::io::BackgroundSettings &bg_settings,
                            const Q::io::SceneCamera &camera_settings,
                            const Q::io::RenderSettings &render_settings);
    };

  } // namespace scene
} // namespace Q