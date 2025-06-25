#include "tile_renderer.hpp"
#include <algorithm>
#include <chrono>
#include <execution>
#include <iomanip>
#include <iostream>
#include <quasi/sampling/adaptive_integrator.hpp>
#include <thread>

namespace Q {
  namespace rendering {

    std::future<std::vector<Q::radiometry::Color>>
    TileRenderer::render_async(const Q::io::SceneData &scene_data, const Q::scene::Scene &scene,
                               Q::scene::RayTracer &ray_tracer,
                               Q::radiometry::Camera *pinhole_camera,
                               Q::radiometry::DepthOfFieldCamera *dof_camera) {

      return std::async(std::launch::async, [=, &scene_data, &scene, &ray_tracer, this]() {
        const int image_width = scene_data.render.width;
        const int image_height = scene_data.render.height;
        const int samples_per_pixel = scene_data.render.multisampling.samples_per_pixel;

        // Pre-allocate pixel buffer
        std::vector<Q::radiometry::Color> pixels(image_width * image_height);

        // Generate tiles for parallel processing
        auto tiles = generate_tiles(image_width, image_height);

        // Calculate total work for progress tracking
        const long long total_rays =
            static_cast<long long>(image_width) * image_height * samples_per_pixel;
        auto progress =
            std::make_shared<RenderProgress>(total_rays, static_cast<int>(tiles.size()));

        // Create sampling pattern and integrator
        auto sample_pattern =
            Q::sampling::create_sample_pattern(scene_data.render.multisampling.sampling_pattern);
        auto sample_integrator = Q::sampling::create_sample_integrator(
            scene_data.render.multisampling.sample_integrator,
            scene_data.render.multisampling.samples_per_pixel,
            scene_data.render.multisampling.max_samples_per_pixel,
            scene_data.render.multisampling.variance_threshold,
            scene_data.render.multisampling.adaptation_levels);

        // Initialize progress tracking
        progress->rendering.store(true);

        // Start timing
        auto start_time = std::chrono::high_resolution_clock::now();

        // Start progress monitoring in a separate thread
        const int width = std::to_string(progress->total_rays).length();

        // Show initial progress
        std::cout << "\rRay " << std::setw(width) << 0 << "/" << progress->total_rays << "   0% (0/"
                  << progress->total_tiles << " tiles)" << std::flush;

        std::thread progress_thread([&]() {
          auto last_update = std::chrono::steady_clock::now();

          while (progress->rendering.load()) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);

            if (elapsed.count() >= 500) { // Update every 500ms for more responsiveness
              long long current_rays = progress->completed_rays.load();
              int completed_tiles = progress->completed_tiles.load();
              int percentage = static_cast<int>(progress->progress_percentage());

              std::cout << "\rRay " << std::setw(width) << current_rays << "/"
                        << progress->total_rays << " " << std::setw(3) << percentage << "% ("
                        << completed_tiles << "/" << progress->total_tiles << " tiles)"
                        << std::flush;
              last_update = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
          }
        });

        // Process tiles in parallel using std::async
        std::vector<std::future<void>> tile_futures;
        tile_futures.reserve(tiles.size());

        for (const auto &tile : tiles) {
          tile_futures.emplace_back(std::async(std::launch::async, [&, tile]() {
            render_tile(tile, pixels, scene_data, scene, ray_tracer, pinhole_camera, dof_camera,
                        sample_pattern.get(), sample_integrator.get(), progress);
          }));
        }

        // Wait for all tiles to complete
        for (auto &future : tile_futures) {
          future.get();
        }

        // Stop progress monitoring
        progress->rendering.store(false);
        progress_thread.join();

        // Calculate final timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        long long ms = duration.count();
        double seconds = ms / 1000.0;
        long long rays_per_second = static_cast<long long>(total_rays / seconds);

        // Clear progress line and show final results
        std::cout << "\r" << std::string(60, ' ') << "\r";

        if (ms < 1000) {
          std::cout << total_rays << " rays traced in " << ms << " ms at " << rays_per_second
                    << " rays/s" << std::endl;
        } else if (ms < 60000) {
          std::cout << total_rays << " rays traced in " << std::fixed << std::setprecision(1)
                    << seconds << " s at " << rays_per_second << " rays/s" << std::endl;
        } else {
          int minutes = ms / 60000;
          double remaining_seconds = (ms % 60000) / 1000.0;
          std::cout << total_rays << " rays traced in " << minutes << " min " << std::fixed
                    << std::setprecision(1) << remaining_seconds << " s at " << rays_per_second
                    << " rays/s" << std::endl;
        }

        return pixels;
      });
    }

    std::vector<Tile> TileRenderer::generate_tiles(int width, int height) const {
      std::vector<Tile> tiles;

      for (int y = 0; y < height; y += tile_size_) {
        for (int x = 0; x < width; x += tile_size_) {
          int x_end = std::min(x + tile_size_, width);
          int y_end = std::min(y + tile_size_, height);
          tiles.emplace_back(x, y, x_end, y_end);
        }
      }

      return tiles;
    }

    void TileRenderer::render_tile(const Tile &tile, std::vector<Q::radiometry::Color> &pixels,
                                   const Q::io::SceneData &scene_data, const Q::scene::Scene &scene,
                                   Q::scene::RayTracer &ray_tracer,
                                   Q::radiometry::Camera *pinhole_camera,
                                   Q::radiometry::DepthOfFieldCamera *dof_camera,
                                   Q::sampling::SamplePattern *sample_pattern,
                                   Q::sampling::SampleIntegrator *sample_integrator,
                                   std::shared_ptr<RenderProgress> progress) {

      int tile_rays = 0;

      // Process each pixel in the tile
      for (int y = tile.y_start; y < tile.y_end; ++y) {
        for (int x = tile.x_start; x < tile.x_end; ++x) {
          Q::radiometry::Color pixel_color =
              render_pixel(x, y, scene_data, scene, ray_tracer, pinhole_camera, dof_camera,
                           sample_pattern, sample_integrator);

          // Store pixel result
          pixels[y * scene_data.render.width + x] = pixel_color;

          // Track rays processed
          tile_rays += scene_data.render.multisampling.samples_per_pixel;
        }
      }

      // Update progress silently (display handled by main thread)
      progress->add_completed_rays(tile_rays);
      progress->add_completed_tile();
    }

    Q::radiometry::Color TileRenderer::render_pixel(
        int x, int y, const Q::io::SceneData &scene_data, const Q::scene::Scene &scene,
        Q::scene::RayTracer &ray_tracer, Q::radiometry::Camera *pinhole_camera,
        Q::radiometry::DepthOfFieldCamera *dof_camera, Q::sampling::SamplePattern *sample_pattern,
        Q::sampling::SampleIntegrator *sample_integrator) const {

      using Color = Q::radiometry::Color;
      using Ray = Q::geometry::Ray;
      using Sample2D = Q::sampling::Sample2D;

      const bool use_depth_of_field = dof_camera != nullptr;
      const int samples_per_pixel = scene_data.render.multisampling.samples_per_pixel;

      Color pixel_color;

      // Handle adaptive sampling specially
      if (scene_data.render.multisampling.sample_integrator == "adaptive") {
        auto *adaptive_integrator =
            dynamic_cast<Q::sampling::AdaptiveIntegrator *>(sample_integrator);
        if (adaptive_integrator) {
          pixel_color =
              adaptive_integrator->integrate_adaptive(x, y, [&](const Sample2D &sample) -> Color {
                // Convert pixel coordinates to UV coordinates with sample offset
                float u = sample.x / static_cast<float>(scene_data.render.width);
                float v = (static_cast<float>(scene_data.render.height) - sample.y) /
                          static_cast<float>(scene_data.render.height);

                Ray ray =
                    use_depth_of_field ? dof_camera->get_ray(u, v) : pinhole_camera->get_ray(u, v);
                return ray_tracer.trace_ray_with_reflections(ray);
              });
        } else {
          // Fallback to regular sampling
          pixel_color = Color(1.0f, 0.0f, 1.0f); // Magenta error color
        }
      } else {
        // Regular sampling
        auto samples = sample_pattern->generate_samples(samples_per_pixel);
        std::vector<Color> sample_colors;
        sample_colors.reserve(samples.size());

        // Trace rays for each sample - can be parallelized at sample level for high sample counts
        if (samples.size() > 32) {
          // Use sequential processing for debugging
          for (const auto &sample : samples) {
            float u =
                (static_cast<float>(x) + sample.x) / static_cast<float>(scene_data.render.width);
            float v = (static_cast<float>(scene_data.render.height - 1 - y) + sample.y) /
                      static_cast<float>(scene_data.render.height);

            Ray ray =
                use_depth_of_field ? dof_camera->get_ray(u, v) : pinhole_camera->get_ray(u, v);
            Color sample_color = ray_tracer.trace_ray_with_reflections(ray);
            sample_colors.push_back(sample_color);
          }
        } else {
          // Sequential processing for low sample counts
          for (const auto &sample : samples) {
            float u =
                (static_cast<float>(x) + sample.x) / static_cast<float>(scene_data.render.width);
            float v = (static_cast<float>(scene_data.render.height - 1 - y) + sample.y) /
                      static_cast<float>(scene_data.render.height);

            Ray ray =
                use_depth_of_field ? dof_camera->get_ray(u, v) : pinhole_camera->get_ray(u, v);
            Color sample_color = ray_tracer.trace_ray_with_reflections(ray);
            sample_colors.push_back(sample_color);
          }
        }

        // Integrate samples to get final pixel color
        pixel_color = sample_integrator->integrate_samples(samples, sample_colors);
      }

      return pixel_color;
    }

  } // namespace rendering
} // namespace Q