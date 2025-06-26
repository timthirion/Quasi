#pragma once

#include "bvh.hpp"
#include "triangle.hpp"
#include "vec.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace Q {
  namespace geometry {

    struct Mesh {
      std::vector<Triangle> triangles;
      std::string name;
      Vec3 center;
      float scale;
      std::unique_ptr<MeshBVH> bvh; // BVH acceleration structure

      Mesh() = default;
      Mesh(const std::vector<Triangle> &triangles, const std::string &name = "",
           const Vec3 &center = Vec3(0, 0, 0), float scale = 1.0f);

      // Copy constructor and assignment operator
      Mesh(const Mesh &other);
      Mesh &operator=(const Mesh &other);

      // Move constructor and assignment operator
      Mesh(Mesh &&other) noexcept = default;
      Mesh &operator=(Mesh &&other) noexcept = default;

      // Transform the mesh by translating and scaling
      void transform(const Vec3 &translation, float scale_factor);

      // Get bounding box
      void get_bounds(Vec3 &min_bounds, Vec3 &max_bounds) const;

      // Get triangle count
      size_t triangle_count() const { return triangles.size(); }

      // Build BVH acceleration structure
      void build_bvh();

      // Accelerated ray intersection using BVH
      std::optional<IntersectionResult> intersect_ray(const Ray &ray) const;

      // Enhanced ray intersection with normal information
      std::optional<MeshIntersectionResult> intersect_ray_enhanced(const Ray &ray) const;

      // Get BVH statistics (returns nullopt if no BVH built)
      std::optional<MeshBVH::Stats> get_bvh_stats() const;
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