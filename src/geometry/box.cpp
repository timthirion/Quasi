#include "box.hpp"

namespace Q {
  namespace geometry {

    Box::Box(const Vec3 &min, const Vec3 &max) : min_corner(min), max_corner(max) {
      build_triangles();
    }

    std::array<Vec3, 8> Box::get_vertices() const {
      // Define the 8 vertices of the box
      return {{
          // Bottom face (y = min.y)
          Vec3(min_corner.x, min_corner.y, min_corner.z), // 0: min corner
          Vec3(max_corner.x, min_corner.y, min_corner.z), // 1
          Vec3(max_corner.x, min_corner.y, max_corner.z), // 2
          Vec3(min_corner.x, min_corner.y, max_corner.z), // 3

          // Top face (y = max.y)
          Vec3(min_corner.x, max_corner.y, min_corner.z), // 4
          Vec3(max_corner.x, max_corner.y, min_corner.z), // 5
          Vec3(max_corner.x, max_corner.y, max_corner.z), // 6: max corner
          Vec3(min_corner.x, max_corner.y, max_corner.z)  // 7
      }};
    }

    void Box::build_triangles() {
      auto vertices = get_vertices();

      // Build 12 triangles (2 per face)
      // Each face has consistent winding order (counter-clockwise when viewed from outside)

      // Bottom face (y = min.y) - looking up at it
      triangles[0] = Triangle(vertices[0], vertices[1], vertices[2]); // 0-1-2
      triangles[1] = Triangle(vertices[0], vertices[2], vertices[3]); // 0-2-3

      // Top face (y = max.y) - looking down at it
      triangles[2] = Triangle(vertices[4], vertices[6], vertices[5]); // 4-6-5
      triangles[3] = Triangle(vertices[4], vertices[7], vertices[6]); // 4-7-6

      // Front face (z = max.z)
      triangles[4] = Triangle(vertices[3], vertices[2], vertices[6]); // 3-2-6
      triangles[5] = Triangle(vertices[3], vertices[6], vertices[7]); // 3-6-7

      // Back face (z = min.z)
      triangles[6] = Triangle(vertices[0], vertices[4], vertices[5]); // 0-4-5
      triangles[7] = Triangle(vertices[0], vertices[5], vertices[1]); // 0-5-1

      // Right face (x = max.x)
      triangles[8] = Triangle(vertices[1], vertices[5], vertices[6]); // 1-5-6
      triangles[9] = Triangle(vertices[1], vertices[6], vertices[2]); // 1-6-2

      // Left face (x = min.x)
      triangles[10] = Triangle(vertices[0], vertices[3], vertices[7]); // 0-3-7
      triangles[11] = Triangle(vertices[0], vertices[7], vertices[4]); // 0-7-4
    }

  } // namespace geometry
} // namespace Q