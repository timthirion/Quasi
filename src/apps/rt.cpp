#include "../geometry/geometry.h"
#include "../io/PPMWriter.h"
#include "../materials/CheckerboardTexture.h"
#include "../radiometry/Camera.h"
#include "../radiometry/Color.h"
#include <iostream>
#include <limits>
#include <vector>

using namespace Q::geometry;
using namespace Q::radiometry;
using namespace Q::io;
using namespace Q::materials;

struct ColoredSphere {
  Sphere sphere;
  Color color;

  ColoredSphere(const Sphere &s, const Color &c) : sphere(s), color(c) {}
};

struct TexturedTriangle {
  Triangle triangle;
  Vec3 uv0, uv1, uv2; // UV coordinates for each vertex (z component unused)
  CheckerboardTexture *texture;

  TexturedTriangle(const Triangle &tri, const Vec3 &uv0, const Vec3 &uv1, const Vec3 &uv2,
                   CheckerboardTexture *tex)
      : triangle(tri), uv0(uv0), uv1(uv1), uv2(uv2), texture(tex) {}
};

class Scene {
private:
  std::vector<ColoredSphere> spheres;
  std::vector<TexturedTriangle> background_triangles;
  CheckerboardTexture background_texture;

public:
  Scene() : background_texture(Color(1.0f, 1.0f, 1.0f), Color(0.0f, 0.0f, 0.0f), 6, 8) {
    // Create background quad at the camera frustum's far plane
    // Camera: 45° FOV, aspect ratio 4:3, positioned at origin looking down -Z
    float far_distance = 20.0f; // Far plane distance
    float fov_radians = 45.0f * M_PI / 180.0f;
    float aspect_ratio = 800.0f / 600.0f; // 4:3

    // Calculate quad dimensions slightly larger than camera frustum to avoid edge precision issues
    float half_height = std::tan(fov_radians / 2.0f) * far_distance;
    float half_width = half_height * aspect_ratio;

    // Make quad slightly larger to ensure all edge rays hit
    float margin = 0.01f; // Small margin to avoid edge precision issues
    half_width += margin;
    half_height += margin;

    float quad_z = -far_distance;

    // Define quad vertices slightly larger than camera frustum
    Vec3 bottom_left(-half_width, -half_height, quad_z);
    Vec3 bottom_right(half_width, -half_height, quad_z);
    Vec3 top_left(-half_width, half_height, quad_z);
    Vec3 top_right(half_width, half_height, quad_z);

    // UV coordinates [0,1] × [0,1] for the quad
    // Use small negative/positive margins to ensure texture coverage
    float uv_margin = -0.005f; // Negative margin to slightly extend UV coverage
    Vec3 uv_bl(uv_margin, uv_margin, 0.0f);
    Vec3 uv_br(1.0f - uv_margin, uv_margin, 0.0f);
    Vec3 uv_tl(uv_margin, 1.0f - uv_margin, 0.0f);
    Vec3 uv_tr(1.0f - uv_margin, 1.0f - uv_margin, 0.0f);

    // First triangle: bottom-left, bottom-right, top-left
    background_triangles.emplace_back(Triangle(bottom_left, bottom_right, top_left), uv_bl, uv_br,
                                      uv_tl, &background_texture);

    // Second triangle: bottom-right, top-right, top-left
    background_triangles.emplace_back(Triangle(bottom_right, top_right, top_left), uv_br, uv_tr,
                                      uv_tl, &background_texture);
  }

  void add_sphere(const Sphere &sphere, const Color &color) { spheres.emplace_back(sphere, color); }

  Color trace_ray(const Ray &ray) const {
    float closest_t = std::numeric_limits<float>::max();
    Color hit_color;
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
            Vec3 bary = result->barycentric;
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
    return Color(0.2f, 0.2f, 0.2f); // Dark gray
  }
};

int main() {
  // Image dimensions
  const int image_width = 800;
  const int image_height = 600;
  const float aspect_ratio = static_cast<float>(image_width) / static_cast<float>(image_height);

  // Camera setup
  Camera camera(Vec3(0, 0, 0),  // look from
                Vec3(0, 0, -1), // look at
                Vec3(0, 1, 0),  // up vector
                45.0f,          // vertical field of view
                aspect_ratio);

  // Scene setup with three spheres
  Scene scene;

  // Left sphere (red) - positioned at x = -1.5
  scene.add_sphere(Sphere(Vec3(-1.5f, 0.0f, -4.0f), 0.6f), Color(1.0f, 0.0f, 0.0f));

  // Middle sphere (green) - positioned at x = 0
  scene.add_sphere(Sphere(Vec3(0.0f, 0.0f, -4.0f), 0.6f), Color(0.0f, 1.0f, 0.0f));

  // Right sphere (blue) - positioned at x = 1.5
  scene.add_sphere(Sphere(Vec3(1.5f, 0.0f, -4.0f), 0.6f), Color(0.0f, 0.0f, 1.0f));

  // Render the image
  std::vector<Color> pixels(image_width * image_height);

  std::cout << "Rendering " << image_width << "x" << image_height << " image..." << std::endl;

  for (int y = 0; y < image_height; ++y) {
    for (int x = 0; x < image_width; ++x) {
      float u = static_cast<float>(x) / static_cast<float>(image_width - 1);
      float v =
          static_cast<float>(image_height - 1 - y) / static_cast<float>(image_height - 1); // Flip Y

      Ray ray = camera.get_ray(u, v);
      Color pixel_color = scene.trace_ray(ray);

      pixels[y * image_width + x] = pixel_color;
    }

    // Progress indicator
    if (y % 50 == 0 || y == image_height - 1) {
      std::cout << "Progress: " << (y + 1) << "/" << image_height << " lines" << std::endl;
    }
  }

  // Write the image to file
  PPMWriter::write_ppm("raytraced_spheres.ppm", pixels, image_width, image_height);

  std::cout << "Raytracing complete!" << std::endl;
  return 0;
}