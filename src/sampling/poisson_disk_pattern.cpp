#include "poisson_disk_pattern.hpp"
#include <algorithm>
#include <cmath>

namespace Q {
  namespace sampling {

    std::vector<Sample2D> PoissonDiskPattern::generate_samples(int samples_per_pixel) const {
      // Calculate appropriate minimum distance based on target sample count
      // For uniform distribution, each sample should occupy roughly 1/samples_per_pixel area
      float target_area_per_sample = 1.0f / static_cast<float>(samples_per_pixel);
      float adaptive_min_distance =
          std::sqrt(target_area_per_sample) * 0.8f; // 80% for some overlap

      // Use the smaller of adaptive distance or configured minimum distance
      float effective_min_distance = std::min(min_distance_, adaptive_min_distance);

      return generate_poisson_disk_samples(samples_per_pixel, effective_min_distance);
    }

    std::vector<Sample2D> PoissonDiskPattern::generate_poisson_disk_samples(int target_count,
                                                                            float min_dist) const {
      std::vector<Sample2D> samples;
      std::vector<Sample2D> active_list;

      std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
      std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
      std::uniform_real_distribution<float> radius_dist(min_dist, 2.0f * min_dist);

      // Generate first sample randomly
      Sample2D first_sample(uniform_dist(rng_), uniform_dist(rng_));
      samples.push_back(first_sample);
      active_list.push_back(first_sample);

      // Generate subsequent samples
      while (!active_list.empty() && static_cast<int>(samples.size()) < target_count) {
        // Pick random sample from active list
        std::uniform_int_distribution<int> index_dist(0, static_cast<int>(active_list.size()) - 1);
        int active_index = index_dist(rng_);
        Sample2D base_sample = active_list[active_index];

        bool found_valid_sample = false;

        // Try to generate new sample around the base sample
        for (int attempt = 0; attempt < max_attempts_; ++attempt) {
          // Generate random point in annulus around base sample
          float angle = angle_dist(rng_);
          float radius = radius_dist(rng_);

          Sample2D candidate(base_sample.x + radius * std::cos(angle),
                             base_sample.y + radius * std::sin(angle));

          // Check if candidate is within unit square and valid
          if (candidate.x >= 0.0f && candidate.x <= 1.0f && candidate.y >= 0.0f &&
              candidate.y <= 1.0f && is_valid_sample(candidate, samples, min_dist)) {

            samples.push_back(candidate);
            active_list.push_back(candidate);
            found_valid_sample = true;
            break;
          }
        }

        // If no valid sample found, remove base sample from active list
        if (!found_valid_sample) {
          active_list.erase(active_list.begin() + active_index);
        }
      }

      // If we don't have enough samples, fill remaining with random samples
      // that respect minimum distance constraint
      while (static_cast<int>(samples.size()) < target_count) {
        bool found_valid = false;

        for (int attempt = 0; attempt < max_attempts_ * 5; ++attempt) {
          Sample2D candidate(uniform_dist(rng_), uniform_dist(rng_));

          if (is_valid_sample(candidate, samples, min_dist)) {
            samples.push_back(candidate);
            found_valid = true;
            break;
          }
        }

        // If we still can't find valid samples, relax the constraint slightly
        if (!found_valid) {
          float relaxed_min_dist = min_dist * 0.8f;
          for (int attempt = 0; attempt < max_attempts_; ++attempt) {
            Sample2D candidate(uniform_dist(rng_), uniform_dist(rng_));

            if (is_valid_sample(candidate, samples, relaxed_min_dist)) {
              samples.push_back(candidate);
              break;
            }
          }

          // If still failing, just add a random sample to avoid infinite loop
          if (static_cast<int>(samples.size()) < target_count) {
            samples.push_back(Sample2D(uniform_dist(rng_), uniform_dist(rng_)));
          }
        }
      }

      return samples;
    }

    bool PoissonDiskPattern::is_valid_sample(const Sample2D &candidate,
                                             const std::vector<Sample2D> &existing_samples,
                                             float min_dist) const {
      for (const auto &existing : existing_samples) {
        float dx = candidate.x - existing.x;
        float dy = candidate.y - existing.y;
        float distance_squared = dx * dx + dy * dy;

        if (distance_squared < min_dist * min_dist) {
          return false;
        }
      }
      return true;
    }

  } // namespace sampling
} // namespace Q