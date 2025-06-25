#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <quasi/geometry/geometry.hpp>
#include <quasi/radiometry/camera.hpp>
#include <quasi/radiometry/color.hpp>
#include <quasi/radiometry/depth_of_field_camera.hpp>
#include <quasi/sampling/sample_integrator.hpp>
#include <quasi/sampling/sample_pattern.hpp>
#include <quasi/scene/ray_tracer.hpp>
#include <quasi/scene/scene.hpp>
#include <thread>
#include <vector>

namespace Q {
  namespace rendering {

    // Tile specification for parallel rendering
    struct Tile {
      int x_start, y_start;
      int x_end, y_end;
      int width, height;

      Tile(int x_start, int y_start, int x_end, int y_end)
          : x_start(x_start), y_start(y_start), x_end(x_end), y_end(y_end), width(x_end - x_start),
            height(y_end - y_start) {}

      int pixel_count() const { return width * height; }
    };

    // Progress tracking for threaded rendering
    struct RenderProgress {
      std::atomic<long long> completed_rays{0};
      std::atomic<int> completed_tiles{0};
      std::atomic<bool> rendering{false};
      long long total_rays;
      int total_tiles;

      RenderProgress(long long total_rays, int total_tiles)
          : total_rays(total_rays), total_tiles(total_tiles) {}

      void add_completed_rays(int rays) { completed_rays.fetch_add(rays); }
      void add_completed_tile() { completed_tiles.fetch_add(1); }

      float progress_percentage() const {
        return (static_cast<float>(completed_rays.load()) / total_rays) * 100.0f;
      }
    };

    // Async tile renderer with coroutines and thread pools
    class TileRenderer {
    private:
      static constexpr int DEFAULT_TILE_SIZE = 64;
      static constexpr int PROGRESS_UPDATE_INTERVAL_MS = 500;

    public:
      // Render scene using tile-based parallelism
      std::future<std::vector<Q::radiometry::Color>>
      render_async(const Q::io::SceneData &scene_data, const Q::scene::Scene &scene,
                   Q::scene::RayTracer &ray_tracer, Q::radiometry::Camera *pinhole_camera = nullptr,
                   Q::radiometry::DepthOfFieldCamera *dof_camera = nullptr);

      // Set custom tile size (default: 64x64)
      void set_tile_size(int tile_size) { tile_size_ = tile_size; }

      // Set number of worker threads (default: hardware concurrency)
      void set_thread_count(int thread_count) { thread_count_ = thread_count; }

    private:
      // Generate tile tasks for the given image dimensions
      std::vector<Tile> generate_tiles(int width, int height) const;

      // Function for rendering a single tile
      void render_tile(const Tile &tile, std::vector<Q::radiometry::Color> &pixels,
                       const Q::io::SceneData &scene_data, const Q::scene::Scene &scene,
                       Q::scene::RayTracer &ray_tracer, Q::radiometry::Camera *pinhole_camera,
                       Q::radiometry::DepthOfFieldCamera *dof_camera,
                       Q::sampling::SamplePattern *sample_pattern,
                       Q::sampling::SampleIntegrator *sample_integrator,
                       std::shared_ptr<RenderProgress> progress);

      // Render a single pixel within a tile
      Q::radiometry::Color render_pixel(int x, int y, const Q::io::SceneData &scene_data,
                                        const Q::scene::Scene &scene,
                                        Q::scene::RayTracer &ray_tracer,
                                        Q::radiometry::Camera *pinhole_camera,
                                        Q::radiometry::DepthOfFieldCamera *dof_camera,
                                        Q::sampling::SamplePattern *sample_pattern,
                                        Q::sampling::SampleIntegrator *sample_integrator) const;

      int tile_size_ = DEFAULT_TILE_SIZE;
      int thread_count_ = std::thread::hardware_concurrency();
    };

  } // namespace rendering
} // namespace Q