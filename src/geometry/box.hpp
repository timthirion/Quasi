#pragma once

#include "triangle.hpp"
#include "vec3.hpp"
#include <array>
#include <vector>

namespace Q {
  namespace geometry {

    /**
     * Axis-aligned box primitive defined by minimum and maximum corners.
     * The box is constructed as 6 faces, each made of 2 triangles (12 triangles total).
     */
    class Box {
    private:
      Vec3 min_corner;
      Vec3 max_corner;
      std::array<Triangle, 12> triangles; // 6 faces * 2 triangles per face

    public:
      Box(const Vec3 &min, const Vec3 &max);

      // Getters
      const Vec3 &get_min() const { return min_corner; }
      const Vec3 &get_max() const { return max_corner; }
      const std::array<Triangle, 12> &get_triangles() const { return triangles; }

      // Get the 8 vertices of the box
      std::array<Vec3, 8> get_vertices() const;

    private:
      void build_triangles();
    };

  } // namespace geometry
} // namespace Q