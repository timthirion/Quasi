#pragma once

#include "ray.hpp"
#include "triangle.hpp"
#include "vec.hpp"
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

namespace Q {
  namespace geometry {

    // Axis-aligned bounding box
    struct AABB {
      Vec3 min, max;

      AABB()
          : min(Vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max())),
            max(Vec3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                     std::numeric_limits<float>::lowest())) {}

      AABB(const Vec3 &min, const Vec3 &max) : min(min), max(max) {}

      // Expand AABB to include a point
      void expand(const Vec3 &point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
      }

      // Expand AABB to include another AABB
      void expand(const AABB &other) {
        expand(other.min);
        expand(other.max);
      }

      // Get center point
      Vec3 center() const { return (min + max) * 0.5f; }

      // Get size/extent
      Vec3 size() const { return max - min; }

      // Get surface area (for SAH calculation)
      float surface_area() const {
        Vec3 d = size();
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
      }

      // Check if AABB is valid (min <= max)
      bool is_valid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
    };

    // BVH traversal result for AABB intersection
    struct AABBIntersection {
      bool hit;
      float t_min, t_max;

      AABBIntersection() : hit(false), t_min(0.0f), t_max(0.0f) {}
      AABBIntersection(bool hit, float t_min, float t_max) : hit(hit), t_min(t_min), t_max(t_max) {}
    };

    // AABB-Ray intersection using efficient slab method
    AABBIntersection intersect_aabb(const Ray &ray, const AABB &aabb);

    // BVH node structure
    struct BVHNode {
      AABB bounds;              // Bounding box for this node
      uint32_t first_primitive; // Index of first triangle (leaf) or left child (internal)
      uint32_t primitive_count; // Number of triangles (leaf) or 0 (internal)
      uint32_t right_child_idx; // Right child index (for internal nodes)
      uint8_t split_axis;       // Split axis for internal nodes (0=X, 1=Y, 2=Z)

      BVHNode() : first_primitive(0), primitive_count(0), right_child_idx(0), split_axis(0) {}

      bool is_leaf() const { return primitive_count > 0; }
      uint32_t left_child() const { return first_primitive; }
      uint32_t right_child() const { return right_child_idx; }
    };

    // Triangle primitive with centroid for BVH construction
    struct BVHPrimitive {
      uint32_t triangle_index;
      Vec3 centroid;
      AABB bounds;

      BVHPrimitive(uint32_t idx, const Triangle &triangle) : triangle_index(idx) {
        // Calculate triangle bounds and centroid
        bounds.expand(triangle.v0);
        bounds.expand(triangle.v1);
        bounds.expand(triangle.v2);
        centroid = bounds.center();
      }
    };

    // Enhanced intersection result with normal information
    struct MeshIntersectionResult {
      bool hit;
      float t;
      Vec3 point;
      Vec3 normal;
      Vec3 barycentric;
      uint32_t triangle_index;

      MeshIntersectionResult() : hit(false), t(0.0f), triangle_index(0) {}
      MeshIntersectionResult(float t, const Vec3 &point, const Vec3 &normal,
                             const Vec3 &barycentric, uint32_t tri_idx)
          : hit(true), t(t), point(point), normal(normal), barycentric(barycentric),
            triangle_index(tri_idx) {}
    };

    // Traversal state for stack-based traversal
    struct TraversalState {
      uint32_t node_index;
      float t_min, t_max;

      TraversalState() : node_index(0), t_min(0.0f), t_max(0.0f) {}
      TraversalState(uint32_t idx, float tmin, float tmax)
          : node_index(idx), t_min(tmin), t_max(tmax) {}
    };

    // BVH acceleration structure for triangle meshes
    class MeshBVH {
    private:
      std::vector<BVHNode> nodes;
      std::vector<uint32_t> triangle_indices; // Reordered triangle references
      const std::vector<Triangle> *triangles; // Reference to original triangles
      uint32_t root_node_idx;
      static constexpr int MAX_LEAF_TRIANGLES = 4; // Maximum triangles per leaf
      static constexpr int MAX_BVH_DEPTH = 12;     // Maximum BVH tree depth during construction
      static constexpr int MAX_TRAVERSAL_DEPTH =
          16; // Maximum traversal stack depth (should be >= MAX_BVH_DEPTH + margin)

    public:
      MeshBVH() : triangles(nullptr), root_node_idx(0) {}

      // Build BVH from triangle array
      void build(const std::vector<Triangle> &triangle_array);

      // Ray intersection using BVH acceleration
      std::optional<IntersectionResult> intersect(const Ray &ray) const;

      // Enhanced ray intersection with normal information
      std::optional<MeshIntersectionResult> intersect_enhanced(const Ray &ray) const;

      // Get BVH statistics
      struct Stats {
        uint32_t node_count;
        uint32_t leaf_count;
        uint32_t max_depth;
        uint32_t total_triangles;
        float avg_leaf_triangles;
      };
      Stats get_stats() const;

    private:
      // Recursive BVH construction
      uint32_t build_recursive(std::vector<BVHPrimitive> &primitives, uint32_t start, uint32_t end,
                               uint32_t depth = 0);

      // Find best split position using median split
      uint32_t find_split_median(std::vector<BVHPrimitive> &primitives, uint32_t start,
                                 uint32_t end, uint8_t axis);

      // Calculate bounding box for primitive range
      AABB calculate_bounds(const std::vector<BVHPrimitive> &primitives, uint32_t start,
                            uint32_t end);

      // Get next available node index
      uint32_t allocate_node();
    };

  } // namespace geometry
} // namespace Q