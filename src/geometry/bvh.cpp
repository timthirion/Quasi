#include "bvh.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>

namespace Q {
  namespace geometry {

    // AABB-Ray intersection using efficient slab method
    AABBIntersection intersect_aabb(const Ray &ray, const AABB &aabb) {
      // Precompute inverse direction to avoid divisions in the loop
      Vec3 inv_dir(1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z);

      // Calculate t values for each slab
      float t1 = (aabb.min.x - ray.origin.x) * inv_dir.x;
      float t2 = (aabb.max.x - ray.origin.x) * inv_dir.x;
      float t3 = (aabb.min.y - ray.origin.y) * inv_dir.y;
      float t4 = (aabb.max.y - ray.origin.y) * inv_dir.y;
      float t5 = (aabb.min.z - ray.origin.z) * inv_dir.z;
      float t6 = (aabb.max.z - ray.origin.z) * inv_dir.z;

      // Find the largest entering t and smallest exiting t
      float t_min = std::max({std::min(t1, t2), std::min(t3, t4), std::min(t5, t6)});
      float t_max = std::min({std::max(t1, t2), std::max(t3, t4), std::max(t5, t6)});

      // Check if ray intersects AABB
      bool hit = t_max >= 0 && t_min <= t_max;

      return AABBIntersection(hit, t_min, t_max);
    }

    void MeshBVH::build(const std::vector<Triangle> &triangle_array) {
      triangles = &triangle_array;

      if (triangle_array.empty()) {
        nodes.clear();
        triangle_indices.clear();
        return;
      }

      // Clear previous data
      nodes.clear();
      triangle_indices.clear();

      // Create primitive array for construction
      std::vector<BVHPrimitive> primitives;
      primitives.reserve(triangle_array.size());

      for (uint32_t i = 0; i < triangle_array.size(); ++i) {
        primitives.emplace_back(i, triangle_array[i]);
      }

      // Initialize triangle indices array
      triangle_indices.reserve(triangle_array.size());

      // Build BVH tree recursively
      root_node_idx = build_recursive(primitives, 0, static_cast<uint32_t>(primitives.size()));

      std::cout << "BVH built: " << nodes.size() << " nodes, " << triangle_array.size()
                << " triangles (max depth: " << MAX_BVH_DEPTH << ")" << std::endl;
    }

    std::optional<IntersectionResult> MeshBVH::intersect(const Ray &ray) const {
      if (!triangles || triangles->empty() || nodes.empty()) {
        return std::nullopt;
      }

      // Stack-based traversal
      std::array<TraversalState, MAX_TRAVERSAL_DEPTH> stack;
      int stack_ptr = 0;

      // Initialize with root node
      auto root_intersection = intersect_aabb(ray, nodes[root_node_idx].bounds);
      if (!root_intersection.hit) {
        return std::nullopt;
      }

      stack[0] = TraversalState(root_node_idx, root_intersection.t_min, root_intersection.t_max);

      float closest_t = std::numeric_limits<float>::max();
      IntersectionResult best_hit;
      bool found_hit = false;

      while (stack_ptr >= 0) {
        TraversalState current = stack[stack_ptr--];

        // Skip if this node is farther than our closest hit
        if (current.t_min > closest_t) {
          continue;
        }

        const BVHNode &node = nodes[current.node_index];

        if (node.is_leaf()) {
          // Leaf node: test triangles
          for (uint32_t i = 0; i < node.primitive_count; ++i) {
            uint32_t triangle_idx = triangle_indices[node.first_primitive + i];
            const Triangle &triangle = (*triangles)[triangle_idx];

            auto result = Q::geometry::intersect(ray, triangle);
            if (result.has_value() && result->t > 0.001f && result->t < closest_t) {
              closest_t = result->t;
              best_hit = *result;
              found_hit = true;
            }
          }
        } else {
          // Internal node: traverse children
          uint32_t left_child = node.left_child();
          uint32_t right_child = node.right_child();

          // Test left child
          auto left_intersection = intersect_aabb(ray, nodes[left_child].bounds);
          if (left_intersection.hit && left_intersection.t_min < closest_t) {
            stack_ptr++;
            if (stack_ptr >= MAX_TRAVERSAL_DEPTH) {
              std::cerr << "Warning: BVH traversal stack overflow" << std::endl;
              break;
            }
            stack[stack_ptr] =
                TraversalState(left_child, left_intersection.t_min, left_intersection.t_max);
          }

          // Test right child
          auto right_intersection = intersect_aabb(ray, nodes[right_child].bounds);
          if (right_intersection.hit && right_intersection.t_min < closest_t) {
            stack_ptr++;
            if (stack_ptr >= MAX_TRAVERSAL_DEPTH) {
              std::cerr << "Warning: BVH traversal stack overflow" << std::endl;
              break;
            }
            stack[stack_ptr] =
                TraversalState(right_child, right_intersection.t_min, right_intersection.t_max);
          }
        }
      }

      return found_hit ? std::optional{best_hit} : std::nullopt;
    }

    std::optional<MeshIntersectionResult> MeshBVH::intersect_enhanced(const Ray &ray) const {
      if (!triangles || triangles->empty() || nodes.empty()) {
        return std::nullopt;
      }

      // Stack-based traversal
      std::array<TraversalState, MAX_TRAVERSAL_DEPTH> stack;
      int stack_ptr = 0;

      // Initialize with root node
      auto root_intersection = intersect_aabb(ray, nodes[root_node_idx].bounds);
      if (!root_intersection.hit) {
        return std::nullopt;
      }

      stack[0] = TraversalState(root_node_idx, root_intersection.t_min, root_intersection.t_max);

      float closest_t = std::numeric_limits<float>::max();
      MeshIntersectionResult best_hit;
      bool found_hit = false;

      while (stack_ptr >= 0) {
        TraversalState current = stack[stack_ptr--];

        // Skip if this node is farther than our closest hit
        if (current.t_min > closest_t) {
          continue;
        }

        const BVHNode &node = nodes[current.node_index];

        if (node.is_leaf()) {
          // Leaf node: test triangles
          for (uint32_t i = 0; i < node.primitive_count; ++i) {
            uint32_t triangle_idx = triangle_indices[node.first_primitive + i];
            const Triangle &triangle = (*triangles)[triangle_idx];

            auto result = Q::geometry::intersect(ray, triangle);
            if (result.has_value() && result->t > 0.001f && result->t < closest_t) {
              closest_t = result->t;

              // Calculate proper normal from triangle vertices
              Vec3 edge1 = triangle.v1 - triangle.v0;
              Vec3 edge2 = triangle.v2 - triangle.v0;
              Vec3 normal = edge1.cross(edge2).get_normalized();

              // Ensure normal points toward camera
              Vec3 to_camera = ray.origin - result->point;
              if (normal.dot(to_camera) < 0) {
                normal = normal * -1.0f;
              }

              best_hit = MeshIntersectionResult(result->t, result->point, normal,
                                                result->barycentric, triangle_idx);
              found_hit = true;
            }
          }
        } else {
          // Internal node: traverse children
          uint32_t left_child = node.left_child();
          uint32_t right_child = node.right_child();

          // Test left child
          auto left_intersection = intersect_aabb(ray, nodes[left_child].bounds);
          if (left_intersection.hit && left_intersection.t_min < closest_t) {
            stack_ptr++;
            if (stack_ptr >= MAX_TRAVERSAL_DEPTH) {
              std::cerr << "Warning: BVH traversal stack overflow" << std::endl;
              break;
            }
            stack[stack_ptr] =
                TraversalState(left_child, left_intersection.t_min, left_intersection.t_max);
          }

          // Test right child
          auto right_intersection = intersect_aabb(ray, nodes[right_child].bounds);
          if (right_intersection.hit && right_intersection.t_min < closest_t) {
            stack_ptr++;
            if (stack_ptr >= MAX_TRAVERSAL_DEPTH) {
              std::cerr << "Warning: BVH traversal stack overflow" << std::endl;
              break;
            }
            stack[stack_ptr] =
                TraversalState(right_child, right_intersection.t_min, right_intersection.t_max);
          }
        }
      }

      return found_hit ? std::optional{best_hit} : std::nullopt;
    }

    uint32_t MeshBVH::build_recursive(std::vector<BVHPrimitive> &primitives, uint32_t start,
                                      uint32_t end, uint32_t depth) {
      assert(start < end);

      uint32_t node_idx = allocate_node();
      BVHNode &node = nodes[node_idx];

      // Calculate bounding box for this node
      node.bounds = calculate_bounds(primitives, start, end);

      uint32_t primitive_count = end - start;

      // Check termination criteria: either reached max leaf size or max depth
      if (primitive_count <= MAX_LEAF_TRIANGLES || depth >= MAX_BVH_DEPTH) {
        // Create leaf node
        node.first_primitive = static_cast<uint32_t>(triangle_indices.size());
        node.primitive_count = primitive_count;

        // Add triangle indices to the array
        for (uint32_t i = start; i < end; ++i) {
          triangle_indices.push_back(primitives[i].triangle_index);
        }

        return node_idx;
      }

      // Find best split axis (use longest axis for median split)
      Vec3 extent = node.bounds.size();
      uint8_t split_axis = 0;
      if (extent.y > extent.x) {
        split_axis = 1;
      }
      if (extent.z > extent[split_axis]) {
        split_axis = 2;
      }
      node.split_axis = split_axis;

      // Find split position using median
      uint32_t mid = find_split_median(primitives, start, end, split_axis);

      // Ensure we actually split the primitives
      if (mid == start || mid == end) {
        // Fallback: split in the middle
        mid = start + primitive_count / 2;
      }

      // Set up for child creation - the left child will be the next node
      uint32_t left_child_idx = static_cast<uint32_t>(nodes.size());

      // Recursively build child nodes
      uint32_t left_child = build_recursive(primitives, start, mid, depth + 1);
      uint32_t right_child = build_recursive(primitives, mid, end, depth + 1);

      // Set child pointers
      nodes[node_idx].first_primitive = left_child;  // Left child
      nodes[node_idx].right_child_idx = right_child; // Right child
      nodes[node_idx].primitive_count = 0;           // Internal node

      return node_idx;
    }

    uint32_t MeshBVH::find_split_median(std::vector<BVHPrimitive> &primitives, uint32_t start,
                                        uint32_t end, uint8_t axis) {
      // Sort primitives by their centroid along the split axis
      std::sort(primitives.begin() + start, primitives.begin() + end,
                [axis](const BVHPrimitive &a, const BVHPrimitive &b) {
                  return a.centroid[axis] < b.centroid[axis];
                });

      // Return median position
      return start + (end - start) / 2;
    }

    AABB MeshBVH::calculate_bounds(const std::vector<BVHPrimitive> &primitives, uint32_t start,
                                   uint32_t end) {
      AABB bounds;
      for (uint32_t i = start; i < end; ++i) {
        bounds.expand(primitives[i].bounds);
      }
      return bounds;
    }

    uint32_t MeshBVH::allocate_node() {
      nodes.emplace_back();
      return static_cast<uint32_t>(nodes.size() - 1);
    }

    MeshBVH::Stats MeshBVH::get_stats() const {
      Stats stats = {0, 0, 0, 0, 0.0f};

      if (nodes.empty()) {
        return stats;
      }

      // Simple traversal to count nodes and leaves
      stats.node_count = static_cast<uint32_t>(nodes.size());
      stats.total_triangles = static_cast<uint32_t>(triangle_indices.size());

      for (const auto &node : nodes) {
        if (node.is_leaf()) {
          stats.leaf_count++;
        }
      }

      if (stats.leaf_count > 0) {
        stats.avg_leaf_triangles = static_cast<float>(stats.total_triangles) / stats.leaf_count;
      }

      // For max depth, we'd need a more complex traversal - for now use the build limit
      stats.max_depth = MAX_BVH_DEPTH;

      return stats;
    }

  } // namespace geometry
} // namespace Q