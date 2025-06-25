#include "sample_pattern.hpp"
#include "blue_noise_pattern.hpp"
#include "poisson_disk_pattern.hpp"
#include "stratified_pattern.hpp"
#include <stdexcept>

namespace Q {
  namespace sampling {

    std::unique_ptr<SamplePattern> create_sample_pattern(const std::string &pattern_name) {
      if (pattern_name == "stratified") {
        return std::make_unique<StratifiedPattern>();
      } else if (pattern_name == "blue_noise") {
        return std::make_unique<BlueNoisePattern>();
      } else if (pattern_name == "poisson_disk") {
        return std::make_unique<PoissonDiskPattern>();
      }

      throw std::invalid_argument("Unknown sampling pattern: " + pattern_name);
    }

  } // namespace sampling
} // namespace Q