#include "adaptive_integrator.hpp"
#include <algorithm>
#include <cmath>

namespace Q {
  namespace sampling {

    AdaptiveIntegrator::AdaptiveIntegrator(std::unique_ptr<SamplePattern> pattern, int base_samples,
                                           int max_samples, float threshold, int levels)
        : sample_pattern(std::move(pattern)), base_samples_per_pixel(base_samples),
          max_samples_per_pixel(max_samples), variance_threshold(threshold),
          adaptation_levels(levels) {}

    float
    AdaptiveIntegrator::calculate_variance(const std::vector<Q::radiometry::Color> &colors) const {
      if (colors.size() < 2) {
        return 0.0f;
      }

      // Calculate mean color
      Q::radiometry::Color mean(0.0f, 0.0f, 0.0f);
      for (const auto &color : colors) {
        mean = mean + color;
      }
      mean = mean * (1.0f / colors.size());

      // Calculate variance using luminance
      float variance_sum = 0.0f;
      for (const auto &color : colors) {
        // Use luminance for variance calculation (perceptually weighted)
        float sample_luminance = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
        float mean_luminance = 0.299f * mean.r + 0.587f * mean.g + 0.114f * mean.b;
        float diff = sample_luminance - mean_luminance;
        variance_sum += diff * diff;
      }

      return variance_sum / (colors.size() - 1);
    }

    bool AdaptiveIntegrator::needs_more_samples(const std::vector<Q::radiometry::Color> &colors,
                                                int current_sample_count) const {
      if (current_sample_count >= max_samples_per_pixel) {
        return false;
      }

      float variance = calculate_variance(colors);
      return variance > variance_threshold;
    }

    Q::radiometry::Color
    AdaptiveIntegrator::integrate_samples(const std::vector<Sample2D> &samples,
                                          const std::vector<Q::radiometry::Color> &colors) const {
      if (colors.empty()) {
        return Q::radiometry::Color(0.0f, 0.0f, 0.0f);
      }

      // Simple average integration (can be enhanced with weighted integration)
      Q::radiometry::Color result(0.0f, 0.0f, 0.0f);
      for (const auto &color : colors) {
        result = result + color;
      }

      return result * (1.0f / colors.size());
    }

    Q::radiometry::Color AdaptiveIntegrator::integrate_adaptive(
        int pixel_x, int pixel_y,
        std::function<Q::radiometry::Color(const Sample2D &)> ray_tracer) const {
      std::vector<Q::radiometry::Color> colors;
      int current_samples = base_samples_per_pixel;

      // Generate initial samples
      auto samples = sample_pattern->generate_samples(current_samples);

      // Offset samples to pixel coordinates
      for (auto &sample : samples) {
        sample.x += pixel_x;
        sample.y += pixel_y;
      }

      // Trace initial samples
      colors.reserve(max_samples_per_pixel);
      for (const auto &sample : samples) {
        colors.push_back(ray_tracer(sample));
      }

      // Adaptive refinement
      for (int level = 0; level < adaptation_levels; ++level) {
        if (!needs_more_samples(colors, current_samples)) {
          break;
        }

        // Double the sample count for next level
        int additional_samples = current_samples;
        current_samples = std::min(current_samples * 2, max_samples_per_pixel);
        additional_samples = current_samples - colors.size();

        if (additional_samples <= 0) {
          break;
        }

        // Generate additional samples
        auto additional_sample_points = sample_pattern->generate_samples(additional_samples);

        // Offset additional samples to pixel coordinates
        for (auto &sample : additional_sample_points) {
          sample.x += pixel_x;
          sample.y += pixel_y;
        }

        // Trace additional samples
        for (const auto &sample : additional_sample_points) {
          colors.push_back(ray_tracer(sample));
        }
      }

      // Integrate all samples
      return integrate_samples({}, colors); // Empty samples vector since we're not using them
    }

  } // namespace sampling
} // namespace Q