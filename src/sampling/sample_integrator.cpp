#include "sample_integrator.hpp"
#include "average_integrator.hpp"
#include <stdexcept>

namespace Q {
  namespace sampling {

    std::unique_ptr<SampleIntegrator> create_sample_integrator(const std::string &integrator_name) {
      if (integrator_name == "average") {
        return std::make_unique<AverageIntegrator>();
      }

      throw std::invalid_argument("Unknown sample integrator: " + integrator_name);
    }

  } // namespace sampling
} // namespace Q