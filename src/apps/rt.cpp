#include <iostream>
#include <quasi/geometry/geometry.hpp>
#include <quasi/io/ppm_writer.hpp>
#include <quasi/io/scene_parser.hpp>
#include <quasi/radiometry/camera.hpp>
#include <quasi/radiometry/color.hpp>
#include <quasi/scene/scene.hpp>
#include <vector>

using namespace Q::geometry;
using namespace Q::radiometry;
using namespace Q::io;
using namespace Q::scene;

// Scene structures are now defined in quasi/scene/scene.hpp

int main(int argc, char *argv[]) {
  // Parse command line arguments
  std::string scene_filename = "scenes/default_scene.json";
  if (argc > 1) {
    scene_filename = argv[1];
  }

  try {
    // Load scene from JSON file
    std::cout << "Loading scene from: " << scene_filename << std::endl;
    auto scene_data = SceneParser::parse_scene_file(scene_filename);
    Scene scene(scene_data);

    // Remove triangle test for sphere test

    // TEST: Print first few rays to see what's happening
    Camera test_camera(scene_data.camera.position, scene_data.camera.look_at, scene_data.camera.up,
                       scene_data.camera.fov, 1.0f);
    for (int i = 0; i < 3; i++) {
      float u = 0.5f; // center
      float v = 0.5f; // center
      Ray test_ray = test_camera.get_ray(u, v);
      std::cout << "Center ray: origin(" << test_ray.origin.x << "," << test_ray.origin.y << ","
                << test_ray.origin.z << ") dir(" << test_ray.direction.x << ","
                << test_ray.direction.y << "," << test_ray.direction.z << ")" << std::endl;
      break;
    }

    // Create camera from scene data
    float aspect_ratio =
        static_cast<float>(scene_data.render.width) / static_cast<float>(scene_data.render.height);
    Camera camera(scene_data.camera.position, scene_data.camera.look_at, scene_data.camera.up,
                  scene_data.camera.fov, aspect_ratio);

    // Render the image
    std::vector<Color> pixels(scene_data.render.width * scene_data.render.height);

    std::cout << "Rendering " << scene_data.render.width << "x" << scene_data.render.height
              << " image..." << std::endl;

    for (int y = 0; y < scene_data.render.height; ++y) {
      for (int x = 0; x < scene_data.render.width; ++x) {
        float u = static_cast<float>(x) / static_cast<float>(scene_data.render.width - 1);
        float v = static_cast<float>(scene_data.render.height - 1 - y) /
                  static_cast<float>(scene_data.render.height - 1); // Flip Y

        Ray ray = camera.get_ray(u, v);
        Color pixel_color = scene.trace_ray(ray);

        pixels[y * scene_data.render.width + x] = pixel_color;
      }

      // Progress indicator
      if (y % 50 == 0 || y == scene_data.render.height - 1) {
        std::cout << "Progress: " << (y + 1) << "/" << scene_data.render.height << " lines"
                  << std::endl;
      }
    }

    // Write the image to file
    std::string output_filename = "raytraced_spheres.ppm";
    if (argc > 2) {
      output_filename = argv[2];
    }
    PPMWriter::write_ppm(output_filename, pixels, scene_data.render.width,
                         scene_data.render.height);

    std::cout << "Raytracing complete!" << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}