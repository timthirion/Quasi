#include "average_integrator.hpp"

namespace Q {
  namespace sampling {

    Q::radiometry::Color
    AverageIntegrator::integrate_samples(const std::vector<Sample2D> &samples,
                                         const std::vector<Q::radiometry::Color> &colors) const {

      if (colors.empty()) {
        return Q::radiometry::Color(0.0f, 0.0f, 0.0f);
      }

      // Simple average of all color samples
      float total_r = 0.0f;
      float total_g = 0.0f;
      float total_b = 0.0f;

      for (const auto &color : colors) {
        total_r += color.r;
        total_g += color.g;
        total_b += color.b;
      }

      float inv_count = 1.0f / static_cast<float>(colors.size());
      return Q::radiometry::Color(total_r * inv_count, total_g * inv_count, total_b * inv_count);
    }

  } // namespace sampling
} // namespace Q