#include "stratified_pattern.hpp"
#include <cmath>

namespace Q {
  namespace sampling {

    std::vector<Sample2D> StratifiedPattern::generate_samples(int samples_per_pixel) const {
      std::vector<Sample2D> samples;
      samples.reserve(samples_per_pixel);

      // Calculate grid dimensions (find closest square grid)
      int grid_size = static_cast<int>(std::ceil(std::sqrt(samples_per_pixel)));
      float cell_size = 1.0f / grid_size;

      // Generate stratified samples
      for (int i = 0; i < samples_per_pixel; ++i) {
        int row = i / grid_size;
        int col = i % grid_size;

        // Base cell position
        float base_x = col * cell_size;
        float base_y = row * cell_size;

        // Add random jitter within the cell
        float jitter_x = dist(gen) * cell_size;
        float jitter_y = dist(gen) * cell_size;

        samples.emplace_back(base_x + jitter_x, base_y + jitter_y);
      }

      return samples;
    }

  } // namespace sampling
} // namespace Q