#pragma once

#include "sample_pattern.hpp"
#include <random>

namespace Q {
  namespace sampling {

    // Stratified (jittered grid) sampling pattern
    class StratifiedPattern : public SamplePattern {
    private:
      mutable std::random_device rd;
      mutable std::mt19937 gen;
      mutable std::uniform_real_distribution<float> dist;

    public:
      StratifiedPattern() : gen(rd()), dist(0.0f, 1.0f) {}

      std::vector<Sample2D> generate_samples(int samples_per_pixel) const override;
      std::string get_name() const override { return "stratified"; }
    };

  } // namespace sampling
} // namespace Q