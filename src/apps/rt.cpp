#include "../geometry/geometry.h"
#include "../io/PPMWriter.h"
#include "../radiometry/Camera.h"
#include "../radiometry/Color.h"
#include <iostream>
#include <limits>
#include <vector>

using namespace Q::geometry;
using namespace Q::radiometry;
using namespace Q::io;

struct ColoredSphere {
  Sphere sphere;
  Color color;

  ColoredSphere(const Sphere &s, const Color &c) : sphere(s), color(c) {}
};

class Scene {
private:
  std::vector<ColoredSphere> spheres;

public:
  void add_sphere(const Sphere &sphere, const Color &color) { spheres.emplace_back(sphere, color); }

  Color trace_ray(const Ray &ray) const {
    float closest_t = std::numeric_limits<float>::max();
    Color hit_color;
    bool hit_anything = false;

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

    if (hit_anything) {
      return hit_color;
    }

    // Background color (light blue gradient)
    Vec3 unit_direction = ray.direction.get_normalized();
    float t = 0.5f * (unit_direction.y + 1.0f);
    return Color((1.0f - t) * 1.0f + t * 0.5f, // White to light blue
                 (1.0f - t) * 1.0f + t * 0.7f, (1.0f - t) * 1.0f + t * 1.0f);
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