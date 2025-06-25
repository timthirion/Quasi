#include "mesh.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace Q {
  namespace geometry {

    Mesh::Mesh(const std::vector<Triangle> &triangles, const std::string &name, const Vec3 &center,
               float scale)
        : triangles(triangles), name(name), center(center), scale(scale) {}

    void Mesh::transform(const Vec3 &translation, float scale_factor) {
      for (auto &triangle : triangles) {
        // Scale around origin first, then translate
        triangle.v0 = triangle.v0 * scale_factor + translation;
        triangle.v1 = triangle.v1 * scale_factor + translation;
        triangle.v2 = triangle.v2 * scale_factor + translation;
      }
      scale *= scale_factor;
      center = center + translation;
    }

    void Mesh::get_bounds(Vec3 &min_bounds, Vec3 &max_bounds) const {
      if (triangles.empty()) {
        min_bounds = max_bounds = Vec3(0, 0, 0);
        return;
      }

      min_bounds = Vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max());
      max_bounds = Vec3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::lowest());

      for (const auto &triangle : triangles) {
        // Check all three vertices
        for (const auto &vertex : {triangle.v0, triangle.v1, triangle.v2}) {
          min_bounds.x = std::min(min_bounds.x, vertex.x);
          min_bounds.y = std::min(min_bounds.y, vertex.y);
          min_bounds.z = std::min(min_bounds.z, vertex.z);

          max_bounds.x = std::max(max_bounds.x, vertex.x);
          max_bounds.y = std::max(max_bounds.y, vertex.y);
          max_bounds.z = std::max(max_bounds.z, vertex.z);
        }
      }
    }

    Mesh MeshReader::load_from_json(const std::string &filename) {
      std::ifstream file(filename);
      if (!file.is_open()) {
        throw std::runtime_error("Could not open mesh file: " + filename);
      }

      nlohmann::json json_data;
      file >> json_data;

      // Parse metadata
      std::string name = json_data.value("name", "Unknown Mesh");
      Vec3 center = parse_vec3(json_data.value("center", nlohmann::json::array({0.0, 0.0, 0.0})));
      float scale = json_data.value("scale", 1.0f);

      // Parse triangles - support both compact vertex/index format and explicit triangle format
      std::vector<Triangle> triangles;

      bool has_vertices = json_data.contains("vertices");
      bool has_indices = json_data.contains("indices");
      bool has_triangles = json_data.contains("triangles");

      // Try compact format first
      auto vertices_it = json_data.find("vertices");
      auto indices_it = json_data.find("indices");

      if (vertices_it != json_data.end() && indices_it != json_data.end()) {
        // New compact format: vertex buffer + index buffer
        const auto &vertices_array = *vertices_it;
        const auto &indices_array = *indices_it;

        // Parse vertices (array of floats: x1,y1,z1,x2,y2,z2,...)
        std::vector<Vec3> vertices;
        if (vertices_array.is_array() && vertices_array.size() % 3 == 0) {
          for (size_t i = 0; i < vertices_array.size(); i += 3) {
            vertices.emplace_back(vertices_array[i].get<float>(),
                                  vertices_array[i + 1].get<float>(),
                                  vertices_array[i + 2].get<float>());
          }
        }

        // Parse indices and build triangles
        if (indices_array.is_array() && indices_array.size() % 3 == 0) {
          for (size_t i = 0; i < indices_array.size(); i += 3) {
            int idx0 = indices_array[i].get<int>();
            int idx1 = indices_array[i + 1].get<int>();
            int idx2 = indices_array[i + 2].get<int>();

            if (idx0 >= 0 && idx1 >= 0 && idx2 >= 0 && idx0 < static_cast<int>(vertices.size()) &&
                idx1 < static_cast<int>(vertices.size()) &&
                idx2 < static_cast<int>(vertices.size())) {
              triangles.emplace_back(vertices[idx0], vertices[idx1], vertices[idx2]);
            } else {
              std::cerr << "Invalid triangle indices: " << idx0 << ", " << idx1 << ", " << idx2
                        << " (vertex count: " << vertices.size() << ")" << std::endl;
            }
          }
        }

      } else if (json_data.contains("triangles") && json_data["triangles"].is_array()) {
        // Old explicit triangle format (backward compatibility)
        for (const auto &triangle_json : json_data["triangles"]) {
          triangles.push_back(parse_triangle(triangle_json));
        }
      }

      return Mesh(triangles, name, center, scale);
    }

    Triangle MeshReader::parse_triangle(const nlohmann::json &triangle_json) {
      Vec3 v0 = parse_vec3(triangle_json["v0"]);
      Vec3 v1 = parse_vec3(triangle_json["v1"]);
      Vec3 v2 = parse_vec3(triangle_json["v2"]);

      return Triangle(v0, v1, v2);
    }

    Vec3 MeshReader::parse_vec3(const nlohmann::json &vec_json) {
      if (!vec_json.is_array() || vec_json.size() != 3) {
        throw std::runtime_error("Invalid Vec3 format in JSON");
      }

      return Vec3(vec_json[0].get<float>(), vec_json[1].get<float>(), vec_json[2].get<float>());
    }

  } // namespace geometry
} // namespace Q