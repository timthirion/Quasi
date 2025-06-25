#include "../rendering/tile_renderer.hpp"
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

    std::cout << "Rendering " << scene_data.render.width << "x" << scene_data.render.height
              << " image using tile-based parallelism (" << std::thread::hardware_concurrency()
              << " threads)..." << std::endl;

    // Create and configure the tile renderer
    Q::rendering::TileRenderer tile_renderer;

    // Render the image asynchronously using tile-based parallelism
    auto render_future = tile_renderer.render_async(scene_data, scene, ray_tracer,
                                                    pinhole_camera.get(), dof_camera.get());

    // Wait for rendering to complete and get the result
    std::vector<Color> pixels = render_future.get();

    // Write the image to file
    std::string output_filename;
    if (argc > 2) {
      output_filename = argv[2];
    } else {
      // Generate filename based on scene name
      std::string scene_path = argv[1];
      size_t last_slash = scene_path.find_last_of('/');
      size_t last_dot = scene_path.find_last_of('.');
      if (last_slash != std::string::npos && last_dot != std::string::npos &&
          last_dot > last_slash) {
        std::string scene_name = scene_path.substr(last_slash + 1, last_dot - last_slash - 1);
        output_filename = "rendered_" + scene_name + ".ppm";
      } else {
        output_filename = "raytraced_output.ppm";
      }
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