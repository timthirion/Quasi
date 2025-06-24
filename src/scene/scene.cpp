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

      // Skip background geometry setup for clean testing

      // Add spheres
      for (const auto &sphere_data : scene_data.spheres) {
        Q::geometry::Sphere sphere(sphere_data.center, sphere_data.radius);
        auto material = std::make_shared<Q::materials::SolidMaterial>(sphere_data.color,
                                                                      sphere_data.reflectance);
        add_sphere(sphere, material);
      }

      // Add triangles
      for (const auto &triangle_data : scene_data.triangles) {
        Q::geometry::Triangle triangle(triangle_data.vertex1, triangle_data.vertex2,
                                       triangle_data.vertex3);
        auto material = std::make_shared<Q::materials::SolidMaterial>(triangle_data.color,
                                                                      triangle_data.reflectance);
        add_triangle(triangle, material);
      }

      // Add boxes
      for (const auto &box_data : scene_data.boxes) {
        Q::geometry::Box box(box_data.min_corner, box_data.max_corner);
        auto material =
            std::make_shared<Q::materials::SolidMaterial>(box_data.color, box_data.reflectance);
        add_box(box, material);
      }

      // Add lights
      for (const auto &light_data : scene_data.lights) {
        auto light = std::make_shared<Q::lighting::PointLight>(
            light_data.position, light_data.color * light_data.intensity);
        add_light(light);
      }
    }

    Scene Scene::from_file(const std::string &filename) {
      auto scene_data = Q::io::SceneParser::parse_scene_file(filename);
      return Scene(scene_data);
    }

    void Scene::add_sphere(const Q::geometry::Sphere &sphere,
                           std::shared_ptr<Q::materials::Material> material) {
      spheres.emplace_back(sphere, material);
    }

    void Scene::add_triangle(const Q::geometry::Triangle &triangle,
                             std::shared_ptr<Q::materials::Material> material) {
      triangles.emplace_back(triangle, material);
    }

    void Scene::add_box(const Q::geometry::Box &box,
                        std::shared_ptr<Q::materials::Material> material) {
      boxes.emplace_back(box, material);
    }

    void Scene::add_light(std::shared_ptr<Q::lighting::Light> light) {
      lights.push_back(light);
    }

    Q::radiometry::Color Scene::trace_ray(const Q::geometry::Ray &ray) const {

      float closest_t = std::numeric_limits<float>::max();
      Q::radiometry::Color hit_color;
      Q::geometry::Vec3 hit_point;
      Q::geometry::Vec3 hit_normal;
      std::shared_ptr<Q::materials::Material> hit_material = nullptr;
      bool hit_anything = false;

      // Check sphere intersections
      for (const auto &colored_sphere : spheres) {
        auto result = ray_sphere_intersection(ray, colored_sphere.sphere);
        if (result.has_value()) {
          // Use the near intersection point (first hit)
          float t = result->t_near;
          if (t > 0.001f && t < closest_t) {
            closest_t = t;
            hit_point = ray.origin + ray.direction * t;
            hit_normal = (hit_point - colored_sphere.sphere.center).get_normalized();
            hit_material = colored_sphere.material;
            hit_anything = true;
          }
        }
      }

      // Check triangle intersections
      for (const auto &colored_triangle : triangles) {
        auto result = ray_triangle_intersection(ray, colored_triangle.triangle);
        if (result.has_value() && result->hit) {

          float t = result->t;
          if (t > 0.001f && t < closest_t) {
            closest_t = t;
            hit_point = ray.origin + ray.direction * t;
            // Calculate triangle normal (ensure it points outward)
            auto edge1 = colored_triangle.triangle.v1 - colored_triangle.triangle.v0;
            auto edge2 = colored_triangle.triangle.v2 - colored_triangle.triangle.v0;
            hit_normal = edge1.cross_product(edge2).get_normalized();

            // Ensure normal points toward camera (for proper lighting)
            Q::geometry::Vec3 to_camera = ray.origin - hit_point;
            if (hit_normal.dot_product(to_camera) < 0) {
              hit_normal = hit_normal * -1.0f; // Flip if pointing away
            }
            hit_material = colored_triangle.material;
            hit_anything = true;
          }
        }
      }

      // Check box intersections
      for (const auto &colored_box : boxes) {
        const auto &triangles = colored_box.box.get_triangles();
        for (int i = 0; i < triangles.size(); ++i) {
          const auto &triangle = triangles[i];
          auto result = ray_triangle_intersection(ray, triangle);
          if (result.has_value() && result->hit) {
            float t = result->t;
            if (t > 0.001f && t < closest_t) {
              closest_t = t;
              hit_point = ray.origin + ray.direction * t;
              // Calculate triangle normal (ensure it points outward)
              auto edge1 = triangle.v1 - triangle.v0;
              auto edge2 = triangle.v2 - triangle.v0;
              hit_normal = edge1.cross_product(edge2).get_normalized();

              // Ensure normal points toward camera (for proper lighting)
              Q::geometry::Vec3 to_camera = ray.origin - hit_point;
              if (hit_normal.dot_product(to_camera) < 0) {
                hit_normal = hit_normal * -1.0f; // Flip if pointing away
              }
              hit_material = colored_box.material;
              hit_anything = true;
            }
          }
        }
      }

      // If we hit something, apply Phong lighting
      if (hit_anything && hit_material) {
        // Calculate view direction (from hit point to camera)
        Q::geometry::Vec3 view_direction = (ray.origin - hit_point).get_normalized();

        // Apply Phong lighting
        hit_color = Q::lighting::PhongLighting::calculate_lighting(
            hit_point, hit_normal, view_direction, *hit_material, lights);

        return hit_color;
      }

      // Return solid black background
      return Q::radiometry::Color(0.0f, 0.0f, 0.0f); // Black background
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

    std::optional<Intersection>
    Scene::find_closest_intersection(const Q::geometry::Ray &ray) const {
      float closest_t = std::numeric_limits<float>::infinity();
      Q::geometry::Vec3 hit_point;
      Q::geometry::Vec3 hit_normal;
      std::shared_ptr<Q::materials::Material> hit_material = nullptr;
      bool hit_anything = false;

      // Check sphere intersections
      for (const auto &colored_sphere : spheres) {
        auto result = ray_sphere_intersection(ray, colored_sphere.sphere);
        if (result.has_value()) {
          float t = result->t_near;
          if (t > 0.001f && t < closest_t) {
            closest_t = t;
            hit_point = ray.origin + ray.direction * t;
            hit_normal = (hit_point - colored_sphere.sphere.center).get_normalized();
            hit_material = colored_sphere.material;
            hit_anything = true;
          }
        }
      }

      // Check triangle intersections
      for (const auto &colored_triangle : triangles) {
        auto result = ray_triangle_intersection(ray, colored_triangle.triangle);
        if (result.has_value()) {
          float t = result->t;
          if (t > 0.001f && t < closest_t) {
            closest_t = t;
            hit_point = ray.origin + ray.direction * t;
            hit_normal = colored_triangle.triangle.get_normal();
            hit_material = colored_triangle.material;
            hit_anything = true;
          }
        }
      }

      // Check box intersections (simplified - we'll use the first hit triangle)
      for (const auto &colored_box : boxes) {
        auto triangles = colored_box.box.get_triangles();
        for (const auto &triangle : triangles) {
          auto result = ray_triangle_intersection(ray, triangle);
          if (result.has_value()) {
            float t = result->t;
            if (t > 0.001f && t < closest_t) {
              closest_t = t;
              hit_point = ray.origin + ray.direction * t;
              hit_normal = triangle.get_normal();
              hit_material = colored_box.material;
              hit_anything = true;
            }
          }
        }
      }

      if (hit_anything) {
        return Intersection(hit_point, hit_normal, closest_t, hit_material);
      }

      return std::nullopt;
    }

  } // namespace scene
} // namespace Q