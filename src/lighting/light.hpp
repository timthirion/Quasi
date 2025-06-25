#pragma once

#include "../geometry/vec.hpp"
#include "../radiometry/color.hpp"
#include "../sampling/sample_pattern.hpp"
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace Q {
  namespace lighting {

    // Light sample structure for area lights
    struct LightSample {
      Q::geometry::Vec3 position;     // Sample position on light surface
      Q::geometry::Vec3 direction;    // Direction from surface to light sample
      Q::radiometry::Color intensity; // Light intensity at this sample
      float distance;                 // Distance to light sample
      float weight;                   // Sample weight (for weighted sampling)

      LightSample(const Q::geometry::Vec3 &pos, const Q::geometry::Vec3 &dir,
                  const Q::radiometry::Color &intens, float dist, float w = 1.0f)
          : position(pos), direction(dir), intensity(intens), distance(dist), weight(w) {}
    };

    /**
     * Base class for light sources.
     */
    class Light {
    public:
      virtual ~Light() = default;

      // Get light direction from a surface point to the light
      virtual Q::geometry::Vec3
      get_direction_to_light(const Q::geometry::Vec3 &surface_point) const = 0;

      // Get light intensity at a surface point
      virtual Q::radiometry::Color get_intensity(const Q::geometry::Vec3 &surface_point) const = 0;

      // Get distance to light (for shadow testing)
      virtual float get_distance(const Q::geometry::Vec3 &surface_point) const = 0;

      // Check if this is an area light (default: false for point lights)
      virtual bool is_area_light() const { return false; }

      // Generate samples for area lights (default: single sample for point lights)
      virtual std::vector<LightSample> generate_samples(const Q::geometry::Vec3 &surface_point,
                                                        int num_samples) const {
        // Default implementation for point lights: single sample
        return {LightSample(Q::geometry::Vec3(0, 0, 0), // position not used for point lights
                            get_direction_to_light(surface_point), get_intensity(surface_point),
                            get_distance(surface_point), 1.0f)};
      }
    };

    /**
     * Point light source with position and intensity.
     */
    class PointLight : public Light {
    private:
      Q::geometry::Vec3 position_;
      Q::radiometry::Color intensity_;
      float attenuation_constant_;
      float attenuation_linear_;
      float attenuation_quadratic_;

    public:
      PointLight(const Q::geometry::Vec3 &pos, const Q::radiometry::Color &color,
                 float constant = 1.0f, float linear = 0.0f, float quadratic = 0.0f)
          : position_(pos), intensity_(color), attenuation_constant_(constant),
            attenuation_linear_(linear), attenuation_quadratic_(quadratic) {}

      Q::geometry::Vec3
      get_direction_to_light(const Q::geometry::Vec3 &surface_point) const override {
        return (position_ - surface_point).get_normalized();
      }

      Q::radiometry::Color get_intensity(const Q::geometry::Vec3 &surface_point) const override {
        float distance = get_distance(surface_point);
        float attenuation = attenuation_constant_ + attenuation_linear_ * distance +
                            attenuation_quadratic_ * distance * distance;

        return intensity_ * (1.0f / attenuation);
      }

      float get_distance(const Q::geometry::Vec3 &surface_point) const override {
        return (position_ - surface_point).get_length();
      }

      const Q::geometry::Vec3 &position() const { return position_; }
    };

    /**
     * Rectangular area light source with position, size, and orientation.
     */
    class RectangularAreaLight : public Light {
    private:
      Q::geometry::Vec3 center_;       // Center position of the rectangle
      Q::geometry::Vec3 u_axis_;       // First edge vector (width direction)
      Q::geometry::Vec3 v_axis_;       // Second edge vector (height direction)
      Q::geometry::Vec3 normal_;       // Surface normal (computed from u x v)
      float width_, height_;           // Dimensions of the rectangle
      Q::radiometry::Color intensity_; // Light intensity
      int default_samples_;            // Default number of samples for this light
      std::string sampling_method_;    // Sampling method: "stratified" or "poisson_disk"
      mutable std::mt19937 rng_;       // Random number generator for Poisson disk sampling

    public:
      RectangularAreaLight(const Q::geometry::Vec3 &center_pos,
                           const Q::geometry::Vec3 &u_direction,
                           const Q::geometry::Vec3 &v_direction, float w, float h,
                           const Q::radiometry::Color &color, int samples = 16,
                           const std::string &sampling_method = "stratified")
          : center_(center_pos), width_(w), height_(h), intensity_(color),
            default_samples_(samples), sampling_method_(sampling_method), rng_(12345) {
        // Normalize and scale the direction vectors
        u_axis_ = u_direction.get_normalized() * (width_ * 0.5f);
        v_axis_ = v_direction.get_normalized() * (height_ * 0.5f);
        normal_ = u_axis_.cross_product(v_axis_).get_normalized();
      }

      // For point-based lighting queries (compatibility with existing code)
      Q::geometry::Vec3
      get_direction_to_light(const Q::geometry::Vec3 &surface_point) const override {
        return (center_ - surface_point).get_normalized();
      }

      Q::radiometry::Color get_intensity(const Q::geometry::Vec3 &surface_point) const override {
        // Simple distance-based falloff for area lights
        float distance = get_distance(surface_point);
        return intensity_ * (1.0f / (distance * distance + 1.0f));
      }

      float get_distance(const Q::geometry::Vec3 &surface_point) const override {
        return (center_ - surface_point).get_length();
      }

      bool is_area_light() const override { return true; }

      // Generate samples across the rectangular area
      std::vector<LightSample> generate_samples(const Q::geometry::Vec3 &surface_point,
                                                int num_samples) const override {
        if (sampling_method_ == "poisson_disk") {
          return generate_poisson_disk_samples(surface_point, num_samples);
        } else {
          return generate_stratified_samples(surface_point, num_samples);
        }
      }

      // Accessors for area light properties
      const Q::geometry::Vec3 &center() const { return center_; }
      const Q::geometry::Vec3 &normal() const { return normal_; }
      float width() const { return width_; }
      float height() const { return height_; }
      int default_samples() const { return default_samples_; }

    private:
      // Generate stratified samples with jitter
      std::vector<LightSample> generate_stratified_samples(const Q::geometry::Vec3 &surface_point,
                                                           int num_samples) const {
        std::vector<LightSample> samples;
        samples.reserve(num_samples);

        int grid_size = static_cast<int>(std::sqrt(num_samples));
        float cell_size = 1.0f / grid_size;

        std::uniform_real_distribution<float> jitter_dist(0.0f, 1.0f);

        for (int i = 0; i < grid_size; ++i) {
          for (int j = 0; j < grid_size; ++j) {
            // Stratified sample within grid cell with jitter
            float u = (i + jitter_dist(rng_)) * cell_size;
            float v = (j + jitter_dist(rng_)) * cell_size;

            samples.push_back(create_light_sample_from_uv(surface_point, u, v, num_samples));
          }
        }

        return samples;
      }

      // Generate Poisson disk samples
      std::vector<LightSample> generate_poisson_disk_samples(const Q::geometry::Vec3 &surface_point,
                                                             int num_samples) const {
        std::vector<LightSample> samples;

        // Calculate appropriate minimum distance based on target sample count
        float target_area_per_sample = 1.0f / static_cast<float>(num_samples);
        float min_distance = std::sqrt(target_area_per_sample) * 0.7f;

        // Generate Poisson disk samples in [0,1] x [0,1] space
        auto uv_samples = generate_poisson_disk_uv_samples(num_samples, min_distance);

        // Convert UV samples to light samples
        samples.reserve(uv_samples.size());
        for (const auto &uv : uv_samples) {
          samples.push_back(
              create_light_sample_from_uv(surface_point, uv.x, uv.y, uv_samples.size()));
        }

        return samples;
      }

      // Generate Poisson disk samples in UV space using simplified algorithm
      std::vector<Q::sampling::Sample2D> generate_poisson_disk_uv_samples(int target_count,
                                                                          float min_dist) const {
        std::vector<Q::sampling::Sample2D> samples;
        std::vector<Q::sampling::Sample2D> active_list;

        std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> radius_dist(min_dist, 2.0f * min_dist);

        // Generate first sample randomly
        Q::sampling::Sample2D first_sample(uniform_dist(rng_), uniform_dist(rng_));
        samples.push_back(first_sample);
        active_list.push_back(first_sample);

        int max_attempts = 30;

        // Generate subsequent samples
        while (!active_list.empty() && static_cast<int>(samples.size()) < target_count) {
          std::uniform_int_distribution<int> index_dist(0,
                                                        static_cast<int>(active_list.size()) - 1);
          int active_index = index_dist(rng_);
          Q::sampling::Sample2D base_sample = active_list[active_index];

          bool found_valid_sample = false;

          for (int attempt = 0; attempt < max_attempts; ++attempt) {
            float angle = angle_dist(rng_);
            float radius = radius_dist(rng_);

            Q::sampling::Sample2D candidate(base_sample.x + radius * std::cos(angle),
                                            base_sample.y + radius * std::sin(angle));

            if (candidate.x >= 0.0f && candidate.x <= 1.0f && candidate.y >= 0.0f &&
                candidate.y <= 1.0f && is_valid_poisson_sample(candidate, samples, min_dist)) {

              samples.push_back(candidate);
              active_list.push_back(candidate);
              found_valid_sample = true;
              break;
            }
          }

          if (!found_valid_sample) {
            active_list.erase(active_list.begin() + active_index);
          }
        }

        // Fill remaining with random samples if needed
        while (static_cast<int>(samples.size()) < target_count) {
          Q::sampling::Sample2D candidate(uniform_dist(rng_), uniform_dist(rng_));
          if (is_valid_poisson_sample(candidate, samples, min_dist * 0.5f)) {
            samples.push_back(candidate);
          } else {
            samples.push_back(candidate); // Add anyway to reach target count
            break;
          }
        }

        return samples;
      }

      // Check if a UV sample is valid for Poisson disk sampling
      bool is_valid_poisson_sample(const Q::sampling::Sample2D &candidate,
                                   const std::vector<Q::sampling::Sample2D> &existing_samples,
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

      // Create a light sample from UV coordinates
      LightSample create_light_sample_from_uv(const Q::geometry::Vec3 &surface_point, float u,
                                              float v, int total_samples) const {
        // Map [0,1] to [-1,1] for symmetric sampling
        u = u * 2.0f - 1.0f;
        v = v * 2.0f - 1.0f;

        // Calculate sample position on the rectangle
        Q::geometry::Vec3 sample_pos = center_ + u_axis_ * u + v_axis_ * v;

        // Calculate direction and distance to sample
        Q::geometry::Vec3 to_sample = sample_pos - surface_point;
        float sample_distance = to_sample.get_length();
        Q::geometry::Vec3 sample_direction = to_sample * (1.0f / sample_distance);

        // Calculate intensity with distance falloff
        Q::radiometry::Color sample_intensity =
            intensity_ * (1.0f / (sample_distance * sample_distance + 1.0f));

        // Weight by cosine of angle with surface normal (Lambert's law)
        float cos_theta = std::max(0.0f, normal_.dot_product(sample_direction * -1.0f));
        float weight = cos_theta / static_cast<float>(total_samples);

        return LightSample(sample_pos, sample_direction, sample_intensity, sample_distance, weight);
      }
    };

  } // namespace lighting
} // namespace Q