#pragma once

#include "../radiometry/color.hpp"
#include "sample_integrator.hpp"
#include "sample_pattern.hpp"
#include <functional>
#include <memory>

namespace Q {
  namespace sampling {

    // Using alias for commonly used type
    using Color = Q::radiometry::Color;

    /**
     * Adaptive sample integrator that automatically increases sampling density
     * in high-variance regions to improve image quality efficiently.
     */
    class AdaptiveIntegrator : public SampleIntegrator {
    private:
      std::unique_ptr<SamplePattern> sample_pattern;

      // Adaptive sampling parameters
      int base_samples_per_pixel;
      int max_samples_per_pixel;
      float variance_threshold;
      int adaptation_levels;

      /**
       * Calculate color variance from a set of samples
       */
      float calculate_variance(const std::vector<Color> &colors) const;

      /**
       * Determine if more samples are needed based on variance
       */
      bool needs_more_samples(const std::vector<Color> &colors, int current_sample_count) const;

    public:
      /**
       * Constructor for adaptive integrator
       * @param pattern Sample pattern to use for generating additional samples
       * @param base_samples Initial number of samples per pixel
       * @param max_samples Maximum number of samples per pixel
       * @param threshold Variance threshold for triggering more samples
       * @param levels Number of adaptation levels (doublings)
       */
      AdaptiveIntegrator(std::unique_ptr<SamplePattern> pattern, int base_samples = 4,
                         int max_samples = 64, float threshold = 0.01f, int levels = 3);

      Color integrate_samples(const std::vector<Sample2D> &samples,
                              const std::vector<Color> &colors) const override;

      /**
       * Adaptive sampling integration with callback for generating more samples
       * @param pixel_x Pixel x coordinate for additional sampling
       * @param pixel_y Pixel y coordinate for additional sampling
       * @param ray_tracer Function to trace rays and get colors
       * @return Final integrated color
       */
      Color integrate_adaptive(int pixel_x, int pixel_y,
                               std::function<Color(const Sample2D &)> ray_tracer) const;

      std::string get_name() const override { return "adaptive"; }

      // Getters for configuration
      int get_base_samples() const { return base_samples_per_pixel; }
      int get_max_samples() const { return max_samples_per_pixel; }
      float get_variance_threshold() const { return variance_threshold; }
    };

  } // namespace sampling
} // namespace Q