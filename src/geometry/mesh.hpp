#pragma once

#include "triangle.hpp"
#include "vec.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Q {
  namespace geometry {

    struct Mesh {
      std::vector<Triangle> triangles;
      std::string name;
      Vec3 center;
      float scale;

      Mesh() = default;
      Mesh(const std::vector<Triangle> &triangles, const std::string &name = "",
           const Vec3 &center = Vec3(0, 0, 0), float scale = 1.0f);

      // Transform the mesh by translating and scaling
      void transform(const Vec3 &translation, float scale_factor);

      // Get bounding box
      void get_bounds(Vec3 &min_bounds, Vec3 &max_bounds) const;

      // Get triangle count
      size_t triangle_count() const { return triangles.size(); }
    };

    // Mesh file reader
    class MeshReader {
    public:
      // Load mesh from JSON file
      static Mesh load_from_json(const std::string &filename);

    private:
      // Parse triangle from JSON object
      static Triangle parse_triangle(const nlohmann::json &triangle_json);

      // Parse Vec3 from JSON array
      static Vec3 parse_vec3(const nlohmann::json &vec_json);
    };

  } // namespace geometry
} // namespace Q