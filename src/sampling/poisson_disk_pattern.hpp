#pragma once

#include "sample_pattern.hpp"
#include <random>
#include <vector>

namespace Q {
  namespace sampling {

    /**
     * Poisson disk sampling pattern using Bridson's algorithm.
     * Generates samples with guaranteed minimum distance between points,
     * producing natural-looking distributions without clustering.
     */
    class PoissonDiskPattern : public SamplePattern {
    private:
      mutable std::mt19937 rng_;
      float min_distance_; // Minimum distance between samples
      int max_attempts_;   // Maximum attempts to place each sample

    public:
      PoissonDiskPattern(float min_distance = 0.1f, int max_attempts = 30,
                         unsigned int seed = 12345)
          : rng_(seed), min_distance_(min_distance), max_attempts_(max_attempts) {}

      std::vector<Sample2D> generate_samples(int samples_per_pixel) const override;
      std::string get_name() const override { return "poisson_disk"; }

    private:
      // Generate Poisson disk samples using Bridson's algorithm
      std::vector<Sample2D> generate_poisson_disk_samples(int target_count, float min_dist) const;

      // Check if a point is valid (far enough from existing samples)
      bool is_valid_sample(const Sample2D &candidate, const std::vector<Sample2D> &existing_samples,
                           float min_dist) const;
    };

  } // namespace sampling
} // namespace Q