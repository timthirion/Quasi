#pragma once

#include "sample_integrator.hpp"

namespace Q {
  namespace sampling {

    // Simple average integrator - equal weight for all samples
    class AverageIntegrator : public SampleIntegrator {
    public:
      Q::radiometry::Color
      integrate_samples(const std::vector<Sample2D> &samples,
                        const std::vector<Q::radiometry::Color> &colors) const override;

      std::string get_name() const override { return "average"; }
    };

  } // namespace sampling
} // namespace Q