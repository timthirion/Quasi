#include "sample_integrator.hpp"
#include "adaptive_integrator.hpp"
#include "average_integrator.hpp"
#include "blue_noise_pattern.hpp"
#include <stdexcept>

namespace Q {
  namespace sampling {

    std::unique_ptr<SampleIntegrator> create_sample_integrator(const std::string &integrator_name) {
      return create_sample_integrator(integrator_name, 4, 64, 0.01f, 3);
    }

    std::unique_ptr<SampleIntegrator> create_sample_integrator(const std::string &integrator_name,
                                                               int base_samples, int max_samples,
                                                               float variance_threshold,
                                                               int adaptation_levels) {
      if (integrator_name == "average") {
        return std::make_unique<AverageIntegrator>();
      } else if (integrator_name == "adaptive") {
        // Create adaptive integrator with blue noise pattern by default
        auto blue_noise_pattern = std::make_unique<BlueNoisePattern>();
        return std::make_unique<AdaptiveIntegrator>(std::move(blue_noise_pattern), base_samples,
                                                    max_samples, variance_threshold,
                                                    adaptation_levels);
      }

      throw std::invalid_argument("Unknown sample integrator: " + integrator_name);
    }

  } // namespace sampling
} // namespace Q