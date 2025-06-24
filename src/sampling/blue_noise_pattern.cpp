#include "blue_noise_pattern.hpp"
#include <algorithm>
#include <limits>

namespace Q {
  namespace sampling {

    float BlueNoisePattern::toroidal_distance(const Sample2D &a, const Sample2D &b) const {
      // Calculate distance with wrapping at domain boundaries
      float dx = std::abs(a.x - b.x);
      float dy = std::abs(a.y - b.y);

      // Handle wrapping for toroidal topology
      dx = std::min(dx, 1.0f - dx);
      dy = std::min(dy, 1.0f - dy);

      return std::sqrt(dx * dx + dy * dy);
    }

    float
    BlueNoisePattern::min_distance_to_samples(const Sample2D &candidate,
                                              const std::vector<Sample2D> &existing_samples) const {
      if (existing_samples.empty()) {
        return std::numeric_limits<float>::max();
      }

      float min_dist = std::numeric_limits<float>::max();
      for (const auto &sample : existing_samples) {
        float dist = toroidal_distance(candidate, sample);
        min_dist = std::min(min_dist, dist);
      }

      return min_dist;
    }

    std::vector<Sample2D> BlueNoisePattern::generate_blue_noise_samples(int sample_count) const {
      std::vector<Sample2D> samples;
      samples.reserve(sample_count);

      if (sample_count <= 0) {
        return samples;
      }

      // Calculate ideal minimum distance for uniform distribution
      float ideal_min_distance = std::sqrt(1.0f / sample_count) * MIN_DISTANCE_FACTOR;

      // Place first sample randomly
      samples.emplace_back(dist(gen), dist(gen));

      // Generate remaining samples using best-candidate algorithm
      for (int i = 1; i < sample_count; ++i) {
        Sample2D best_candidate(0.0f, 0.0f);
        float best_distance = -1.0f;

        // Generate multiple candidates and pick the best one
        for (int j = 0; j < CANDIDATE_COUNT; ++j) {
          Sample2D candidate(dist(gen), dist(gen));
          float candidate_distance = min_distance_to_samples(candidate, samples);

          if (candidate_distance > best_distance) {
            best_candidate = candidate;
            best_distance = candidate_distance;
          }
        }

        samples.push_back(best_candidate);
      }

      return samples;
    }

    std::vector<Sample2D> BlueNoisePattern::generate_samples(int samples_per_pixel) const {
      return generate_blue_noise_samples(samples_per_pixel);
    }

    std::vector<Sample2D> BlueNoisePattern::generate_blue_noise_points(int count,
                                                                       float domain_width,
                                                                       float domain_height) const {
      auto unit_samples = generate_blue_noise_samples(count);

      // Scale samples to desired domain
      std::vector<Sample2D> scaled_samples;
      scaled_samples.reserve(count);

      for (const auto &sample : unit_samples) {
        scaled_samples.emplace_back(sample.x * domain_width, sample.y * domain_height);
      }

      return scaled_samples;
    }

  } // namespace sampling
} // namespace Q