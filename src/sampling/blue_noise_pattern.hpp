#pragma once

#include "sample_pattern.hpp"
#include <cmath>
#include <random>
#include <vector>

namespace Q {
  namespace sampling {

    /**
     * Blue noise sampling pattern using Mitchell's best-candidate algorithm.
     * Blue noise has uniform distribution with minimal low-frequency energy,
     * reducing aliasing and improving visual quality.
     */
    class BlueNoisePattern : public SamplePattern {
    private:
      mutable std::random_device rd;
      mutable std::mt19937 gen;
      mutable std::uniform_real_distribution<float> dist;

      // Best-candidate algorithm parameters
      static constexpr int CANDIDATE_COUNT = 64;         // Number of candidates to test per sample
      static constexpr float MIN_DISTANCE_FACTOR = 0.7f; // Minimum distance factor for blue noise

      /**
       * Calculate distance between two 2D points with toroidal topology
       * (wrapping at boundaries for better distribution)
       */
      float toroidal_distance(const Sample2D &a, const Sample2D &b) const;

      /**
       * Find minimum distance from a point to existing samples
       */
      float min_distance_to_samples(const Sample2D &candidate,
                                    const std::vector<Sample2D> &existing_samples) const;

      /**
       * Generate blue noise samples using Mitchell's best-candidate algorithm
       */
      std::vector<Sample2D> generate_blue_noise_samples(int sample_count) const;

    public:
      BlueNoisePattern() : gen(rd()), dist(0.0f, 1.0f) {}

      std::vector<Sample2D> generate_samples(int samples_per_pixel) const override;
      std::string get_name() const override { return "blue_noise"; }

      /**
       * Generate generic blue noise points for any application
       * @param count Number of samples to generate
       * @param domain_width Width of sampling domain (default 1.0 for unit square)
       * @param domain_height Height of sampling domain (default 1.0 for unit square)
       * @return Vector of blue noise distributed points
       */
      std::vector<Sample2D> generate_blue_noise_points(int count, float domain_width = 1.0f,
                                                       float domain_height = 1.0f) const;
    };

  } // namespace sampling
} // namespace Q