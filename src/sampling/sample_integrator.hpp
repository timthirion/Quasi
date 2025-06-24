#pragma once

#include "../radiometry/color.hpp"
#include "sample_pattern.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Q {
  namespace sampling {

    // Abstract base class for sample integrators
    class SampleIntegrator {
    public:
      virtual ~SampleIntegrator() = default;

      // Integrate multiple color samples into a final pixel color
      // samples: the 2D sample positions used
      // colors: the corresponding color values for each sample
      virtual Q::radiometry::Color
      integrate_samples(const std::vector<Sample2D> &samples,
                        const std::vector<Q::radiometry::Color> &colors) const = 0;

      // Get the integrator name for debugging/logging
      virtual std::string get_name() const = 0;
    };

    // Factory function to create sample integrators
    std::unique_ptr<SampleIntegrator> create_sample_integrator(const std::string &integrator_name);

    // Factory function to create sample integrators with parameters
    std::unique_ptr<SampleIntegrator> create_sample_integrator(const std::string &integrator_name,
                                                               int base_samples = 4,
                                                               int max_samples = 64,
                                                               float variance_threshold = 0.01f,
                                                               int adaptation_levels = 3);

  } // namespace sampling
} // namespace Q