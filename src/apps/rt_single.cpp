#include <chrono>
#include <iomanip>
#include <iostream>
#include <quasi/geometry/geometry.hpp>
#include <quasi/io/ppm_writer.hpp>
#include <quasi/io/scene_parser.hpp>
#include <quasi/radiometry/camera.hpp>
#include <quasi/radiometry/color.hpp>
#include <quasi/radiometry/depth_of_field_camera.hpp>
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

    // Use depth of field camera if aperture is specified, otherwise use pinhole camera
    bool use_depth_of_field = scene_data.camera.aperture > 0.0f;
    std::unique_ptr<Camera> pinhole_camera;
    std::unique_ptr<DepthOfFieldCamera> dof_camera;

    if (use_depth_of_field) {
      dof_camera = std::make_unique<DepthOfFieldCamera>(
          scene_data.camera.position, scene_data.camera.look_at, scene_data.camera.up,
          scene_data.camera.fov, aspect_ratio, scene_data.camera.aperture,
          scene_data.camera.focus_distance);
      std::cout << "Using depth of field camera: aperture=" << scene_data.camera.aperture
                << ", focus_distance=" << scene_data.camera.focus_distance << std::endl;
    } else {
      pinhole_camera =
          std::make_unique<Camera>(scene_data.camera.position, scene_data.camera.look_at,
                                   scene_data.camera.up, scene_data.camera.fov, aspect_ratio);
      std::cout << "Using pinhole camera (no depth of field)" << std::endl;
    }

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

    // Calculate total number of rays to cast
    int total_pixels = scene_data.render.width * scene_data.render.height;
    int samples_per_pixel = scene_data.render.multisampling.samples_per_pixel;
    long long total_rays = (long long) total_pixels * samples_per_pixel;
    long long current_ray = 0;

    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int y = 0; y < scene_data.render.height; ++y) {
      for (int x = 0; x < scene_data.render.width; ++x) {
        Color pixel_color;

        // Handle adaptive sampling specially
        if (scene_data.render.multisampling.sample_integrator == "adaptive") {
          auto *adaptive_integrator = dynamic_cast<AdaptiveIntegrator *>(sample_integrator.get());
          if (adaptive_integrator) {
            pixel_color =
                adaptive_integrator->integrate_adaptive(x, y, [&](const Sample2D &sample) -> Color {
                  current_ray++;
                  // Convert pixel coordinates to UV coordinates with sample offset
                  float u = sample.x / static_cast<float>(scene_data.render.width);
                  float v = (static_cast<float>(scene_data.render.height) - sample.y) /
                            static_cast<float>(scene_data.render.height);

                  Ray ray = use_depth_of_field ? dof_camera->get_ray(u, v)
                                               : pinhole_camera->get_ray(u, v);
                  return ray_tracer.trace_ray_with_reflections(ray);
                });
          } else {
            // Fallback to regular sampling
            current_ray += samples_per_pixel;
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
            current_ray++;
            // Convert pixel coordinates to UV coordinates with sample offset
            float u =
                (static_cast<float>(x) + sample.x) / static_cast<float>(scene_data.render.width);
            float v = (static_cast<float>(scene_data.render.height - 1 - y) + sample.y) /
                      static_cast<float>(scene_data.render.height);

            Ray ray =
                use_depth_of_field ? dof_camera->get_ray(u, v) : pinhole_camera->get_ray(u, v);
            Color sample_color = ray_tracer.trace_ray_with_reflections(ray);
            sample_colors.push_back(sample_color);
          }

          // Integrate samples to get final pixel color
          pixel_color = sample_integrator->integrate_samples(samples, sample_colors);
        }

        pixels[y * scene_data.render.width + x] = pixel_color;

        // Update progress display with percentage (no newline, overwrite previous output)
        int percentage = (int) ((float) current_ray / (float) total_rays * 100.0f);
        std::cout << "\rRay " << current_ray << "/" << total_rays << " " << percentage << "%"
                  << std::flush;
      }
    }

    // Clear the progress line and move to next line
    std::cout << "\r" << std::string(60, ' ') << "\r";

    // Calculate and display rendering time with rays per second
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    long long ms = duration.count();
    double seconds = ms / 1000.0;
    long long rays_per_second = (long long) (total_rays / seconds);

    if (ms < 1000) {
      std::cout << "Rendering completed in " << ms << " ms at " << rays_per_second << " rays/s"
                << std::endl;
    } else if (ms < 60000) {
      std::cout << "Rendering completed in " << std::fixed << std::setprecision(1) << seconds
                << " s at " << rays_per_second << " rays/s" << std::endl;
    } else {
      int minutes = ms / 60000;
      double remaining_seconds = (ms % 60000) / 1000.0;
      std::cout << "Rendering completed in " << minutes << " min " << std::fixed
                << std::setprecision(1) << remaining_seconds << " s at " << rays_per_second
                << " rays/s" << std::endl;
    }

    // Write the image to file
    std::string output_filename = "raytraced_spheres_single.ppm";
    if (argc > 2) {
      output_filename = argv[2];
    }
    // Use Reinhard tone mapping with gamma correction for better image quality
    PPMWriter::write_ppm(output_filename, pixels, scene_data.render.width, scene_data.render.height,
                         ToneMapType::REINHARD, 0.0f, 2.2f);

    std::cout << "Raytracing complete!" << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}