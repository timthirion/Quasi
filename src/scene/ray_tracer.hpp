#pragma once

#include "../geometry/ray.hpp"
#include "../geometry/reflection.hpp"
#include "../radiometry/color.hpp"
#include "scene.hpp"
#include <vector>

namespace Q {
  namespace scene {

    // Structure to hold ray bounce information
    struct RayBounce {
      Q::geometry::Ray ray;
      Q::radiometry::Color attenuation; // How much light this ray can contribute
      int depth;

      RayBounce(const Q::geometry::Ray &r, const Q::radiometry::Color &att, int d)
          : ray(r), attenuation(att), depth(d) {}
    };

    // Enhanced ray tracer with reflection support
    class RayTracer {
    private:
      const Scene &scene;
      int max_depth;

    public:
      RayTracer(const Scene &scene, int max_bounces = 3) : scene(scene), max_depth(max_bounces) {}

      // Trace ray with reflection bounces using iterative approach
      Q::radiometry::Color trace_ray_with_reflections(const Q::geometry::Ray &initial_ray);

      // Set maximum reflection depth
      void set_max_depth(int depth) { max_depth = depth; }
      int get_max_depth() const { return max_depth; }
    };

  } // namespace scene
} // namespace Q