#pragma once

#include "../geometry/vec3.hpp"
#include <vector>

namespace Q {
  namespace sampling {

    // A 2D sample point within a pixel (coordinates in [0,1] x [0,1])
    struct Sample2D {
      float x, y;

      Sample2D(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
    };

    // Abstract base class for sampling patterns
    class SamplePattern {
    public:
      virtual ~SamplePattern() = default;

      // Generate sample points for a pixel
      // Returns samples in [0,1] x [0,1] coordinate space relative to pixel
      virtual std::vector<Sample2D> generate_samples(int samples_per_pixel) const = 0;

      // Get the pattern name for debugging/logging
      virtual std::string get_name() const = 0;
    };

    // Factory function to create sampling patterns
    std::unique_ptr<SamplePattern> create_sample_pattern(const std::string &pattern_name);

  } // namespace sampling
} // namespace Q