#include "../geometry/geometry.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

using namespace Q::geometry;

struct Color {
  float r, g, b;

  Color() : r(0), g(0), b(0) {}
  Color(float r, float g, float b) : r(r), g(g), b(b) {}

  // Convert to 0-255 range for PPM
  int r_int() const { return static_cast<int>(std::clamp(r * 255.0f, 0.0f, 255.0f)); }
  int g_int() const { return static_cast<int>(std::clamp(g * 255.0f, 0.0f, 255.0f)); }
  int b_int() const { return static_cast<int>(std::clamp(b * 255.0f, 0.0f, 255.0f)); }
};

class PPMWriter {
public:
  static void write_ppm(const std::string &filename, const std::vector<Color> &pixels, int width,
                        int height) {
    std::ofstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
      return;
    }

    // PPM header
    file << "P3\n";
    file << width << " " << height << "\n";
    file << "255\n";

    // Pixel data
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const Color &pixel = pixels[y * width + x];
        file << pixel.r_int() << " " << pixel.g_int() << " " << pixel.b_int() << " ";
      }
      file << "\n";
    }

    file.close();
    std::cout << "Image written to " << filename << std::endl;
  }
};

class Camera {
private:
  Vec3 origin;
  Vec3 lower_left_corner;
  Vec3 horizontal;
  Vec3 vertical;

public:
  Camera(Vec3 look_from, Vec3 look_at, Vec3 vup, float vfov, float aspect_ratio) {
    float theta = vfov * M_PI / 180.0f;
    float half_height = std::tan(theta / 2.0f);
    float half_width = aspect_ratio * half_height;

    origin = look_from;
    Vec3 w = (look_from - look_at).get_normalized();
    Vec3 u = vup.cross_product(w).get_normalized();
    Vec3 v = w.cross_product(u);

    lower_left_corner = origin - u * half_width - v * half_height - w;
    horizontal = u * (2.0f * half_width);
    vertical = v * (2.0f * half_height);
  }

  Ray get_ray(float u, float v) const {
    return Ray(origin, lower_left_corner + horizontal * u + vertical * v - origin);
  }
};

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