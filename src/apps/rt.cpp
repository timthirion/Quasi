#include <iostream>
#include <quasi/geometry/geometry.hpp>
#include <quasi/io/ppm_writer.hpp>
#include <quasi/io/scene_parser.hpp>
#include <quasi/radiometry/camera.hpp>
#include <quasi/radiometry/color.hpp>
#include <quasi/sampling/adaptive_integrator.hpp>
#include <quasi/sampling/sample_integrator.hpp>
#include <quasi/sampling/sample_pattern.hpp>
#include <quasi/scene/ray_tracer.hpp>
#include <quasi/scene/scene.hpp>
#include <vector>

using namespace Q::geometry;
using namespace Q::radiometry;
using namespace Q::io;
using namespace Q::sampling;
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

    // Create ray tracer with reflection support
    RayTracer ray_tracer(scene, 3); // 3 reflection bounces

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

    // Create sampling pattern and integrator
    std::unique_ptr<SamplePattern> sample_pattern;
    std::unique_ptr<SampleIntegrator> sample_integrator;

    // Handle adaptive sampling specially since it needs its own pattern
    if (scene_data.render.multisampling.sample_integrator == "adaptive") {
      sample_integrator =
          create_sample_integrator(scene_data.render.multisampling.sample_integrator,
                                   scene_data.render.multisampling.samples_per_pixel,
                                   scene_data.render.multisampling.max_samples_per_pixel,
                                   scene_data.render.multisampling.variance_threshold,
                                   scene_data.render.multisampling.adaptation_levels);
      // For adaptive sampling, create a separate pattern for regular use
      sample_pattern = create_sample_pattern(scene_data.render.multisampling.sampling_pattern);
    } else {
      sample_pattern = create_sample_pattern(scene_data.render.multisampling.sampling_pattern);
      sample_integrator =
          create_sample_integrator(scene_data.render.multisampling.sample_integrator,
                                   scene_data.render.multisampling.samples_per_pixel,
                                   scene_data.render.multisampling.max_samples_per_pixel,
                                   scene_data.render.multisampling.variance_threshold,
                                   scene_data.render.multisampling.adaptation_levels);
    }

    std::cout << "Using " << scene_data.render.multisampling.samples_per_pixel
              << " samples per pixel with " << sample_pattern->get_name() << " sampling and "
              << sample_integrator->get_name() << " integration" << std::endl;

    // Render the image
    std::vector<Color> pixels(scene_data.render.width * scene_data.render.height);

    std::cout << "Rendering " << scene_data.render.width << "x" << scene_data.render.height
              << " image..." << std::endl;

    for (int y = 0; y < scene_data.render.height; ++y) {
      for (int x = 0; x < scene_data.render.width; ++x) {
        Color pixel_color;

        // Handle adaptive sampling specially
        if (scene_data.render.multisampling.sample_integrator == "adaptive") {
          auto *adaptive_integrator = dynamic_cast<AdaptiveIntegrator *>(sample_integrator.get());
          if (adaptive_integrator) {
            pixel_color =
                adaptive_integrator->integrate_adaptive(x, y, [&](const Sample2D &sample) -> Color {
                  // Convert pixel coordinates to UV coordinates with sample offset
                  float u = sample.x / static_cast<float>(scene_data.render.width);
                  float v = (static_cast<float>(scene_data.render.height) - sample.y) /
                            static_cast<float>(scene_data.render.height);

                  Ray ray = camera.get_ray(u, v);
                  return ray_tracer.trace_ray_with_reflections(ray);
                });
          } else {
            // Fallback to regular sampling
            pixel_color = Color(1.0f, 0.0f, 1.0f); // Magenta error color
          }
        } else {
          // Regular sampling
          auto samples =
              sample_pattern->generate_samples(scene_data.render.multisampling.samples_per_pixel);
          std::vector<Color> sample_colors;
          sample_colors.reserve(samples.size());

          // Trace rays for each sample
          for (const auto &sample : samples) {
            // Convert pixel coordinates to UV coordinates with sample offset
            float u =
                (static_cast<float>(x) + sample.x) / static_cast<float>(scene_data.render.width);
            float v = (static_cast<float>(scene_data.render.height - 1 - y) + sample.y) /
                      static_cast<float>(scene_data.render.height);

            Ray ray = camera.get_ray(u, v);
            Color sample_color = ray_tracer.trace_ray_with_reflections(ray);
            sample_colors.push_back(sample_color);
          }

          // Integrate samples to get final pixel color
          pixel_color = sample_integrator->integrate_samples(samples, sample_colors);
        }

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