#include "scene.hpp"
#include <cmath>
#include <limits>

namespace Q {
  namespace scene {

    Scene::Scene() {
      // Create default empty scene
    }

    Scene::Scene(const Q::io::SceneData &scene_data) {
      // Create background texture
      background_texture = std::make_unique<Q::materials::CheckerboardTexture>(
          scene_data.background.color1, scene_data.background.color2, scene_data.background.rows,
          scene_data.background.columns);

      // Setup background geometry
      setup_background(scene_data.background, scene_data.camera, scene_data.render);

      // Add spheres
      for (const auto &sphere_data : scene_data.spheres) {
        Q::geometry::Sphere sphere(sphere_data.center, sphere_data.radius);
        add_sphere(sphere, sphere_data.color);
      }
    }

    Scene Scene::from_file(const std::string &filename) {
      auto scene_data = Q::io::SceneParser::parse_scene_file(filename);
      return Scene(scene_data);
    }

    void Scene::add_sphere(const Q::geometry::Sphere &sphere, const Q::radiometry::Color &color) {
      spheres.emplace_back(sphere, color);
    }

    Q::radiometry::Color Scene::trace_ray(const Q::geometry::Ray &ray) const {
      float closest_t = std::numeric_limits<float>::max();
      Q::radiometry::Color hit_color;
      bool hit_anything = false;

      // Check sphere intersections
      for (const auto &colored_sphere : spheres) {
        auto result = ray_sphere_intersection(ray, colored_sphere.sphere);
        if (result.has_value()) {
          // Use the near intersection point (first hit)
          float t = result->t_near;
          if (t > 0.001f && t < closest_t) { // Small epsilon to avoid self-intersection
            closest_t = t;
            hit_color = colored_sphere.color;
            hit_anything = true;
          }
        }
      }

      // Check background triangle intersections (only if no sphere hit)
      if (!hit_anything) {
        for (const auto &textured_triangle : background_triangles) {
          auto result = ray_triangle_intersection(ray, textured_triangle.triangle);
          if (result.has_value() && result->hit) {
            float t = result->t;
            if (t > 0.001f && t < closest_t) {
              closest_t = t;

              // Interpolate UV coordinates using barycentric coordinates
              Q::geometry::Vec3 bary = result->barycentric;
              float u = bary.x * textured_triangle.uv0.x + bary.y * textured_triangle.uv1.x +
                        bary.z * textured_triangle.uv2.x;
              float v = bary.x * textured_triangle.uv0.y + bary.y * textured_triangle.uv1.y +
                        bary.z * textured_triangle.uv2.y;

              hit_color = textured_triangle.texture->sample(u, v);
              hit_anything = true;
            }
          }
        }
      }

      if (hit_anything) {
        return hit_color;
      }

      // Fallback background color (should rarely be reached)
      return Q::radiometry::Color(0.2f, 0.2f, 0.2f); // Dark gray
    }

    void Scene::setup_background(const Q::io::BackgroundSettings &bg_settings,
                                 const Q::io::SceneCamera &camera_settings,
                                 const Q::io::RenderSettings &render_settings) {
      // Create background quad at the specified distance
      float far_distance = bg_settings.distance;
      float fov_radians = camera_settings.fov * M_PI / 180.0f;
      float aspect_ratio =
          static_cast<float>(render_settings.width) / static_cast<float>(render_settings.height);

      // Calculate quad dimensions slightly larger than camera frustum to avoid edge precision
      // issues
      float half_height = std::tan(fov_radians / 2.0f) * far_distance;
      float half_width = half_height * aspect_ratio;

      // Make quad slightly larger to ensure all edge rays hit
      float margin = 0.01f; // Small margin to avoid edge precision issues
      half_width += margin;
      half_height += margin;

      float quad_z = -far_distance;

      // Define quad vertices slightly larger than camera frustum
      Q::geometry::Vec3 bottom_left(-half_width, -half_height, quad_z);
      Q::geometry::Vec3 bottom_right(half_width, -half_height, quad_z);
      Q::geometry::Vec3 top_left(-half_width, half_height, quad_z);
      Q::geometry::Vec3 top_right(half_width, half_height, quad_z);

      // UV coordinates [0,1] × [0,1] for the quad
      // Use small negative/positive margins to ensure texture coverage
      float uv_margin = -0.005f; // Negative margin to slightly extend UV coverage
      Q::geometry::Vec3 uv_bl(uv_margin, uv_margin, 0.0f);
      Q::geometry::Vec3 uv_br(1.0f - uv_margin, uv_margin, 0.0f);
      Q::geometry::Vec3 uv_tl(uv_margin, 1.0f - uv_margin, 0.0f);
      Q::geometry::Vec3 uv_tr(1.0f - uv_margin, 1.0f - uv_margin, 0.0f);

      // First triangle: bottom-left, bottom-right, top-left
      background_triangles.emplace_back(Q::geometry::Triangle(bottom_left, bottom_right, top_left),
                                        uv_bl, uv_br, uv_tl, background_texture.get());

      // Second triangle: bottom-right, top-right, top-left
      background_triangles.emplace_back(Q::geometry::Triangle(bottom_right, top_right, top_left),
                                        uv_br, uv_tr, uv_tl, background_texture.get());
    }

  } // namespace scene
} // namespace Q