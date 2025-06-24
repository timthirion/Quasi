#pragma once

#include "../geometry/geometry.hpp"
#include "../materials/checkerboard_texture.hpp"
#include "../radiometry/camera.hpp"
#include "../radiometry/color.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Q {
  namespace io {

    struct SceneCamera {
      Q::geometry::Vec3 position;
      Q::geometry::Vec3 look_at;
      Q::geometry::Vec3 up;
      float fov;
    };

    struct RenderSettings {
      int width;
      int height;
    };

    struct BackgroundSettings {
      Q::radiometry::Color color1;
      Q::radiometry::Color color2;
      int rows;
      int columns;
      float distance;
    };

    struct SceneSphere {
      Q::geometry::Vec3 center;
      float radius;
      Q::radiometry::Color color;
    };

    struct SceneBox {
      Q::geometry::Vec3 min_corner;
      Q::geometry::Vec3 max_corner;
      Q::radiometry::Color color;
    };

    struct SceneTriangle {
      Q::geometry::Vec3 vertex1;
      Q::geometry::Vec3 vertex2;
      Q::geometry::Vec3 vertex3;
      Q::radiometry::Color color;
    };

    struct SceneLight {
      Q::geometry::Vec3 position;
      Q::radiometry::Color color;
      float intensity;
    };

    struct SceneData {
      SceneCamera camera;
      RenderSettings render;
      BackgroundSettings background;
      std::vector<SceneSphere> spheres;
      std::vector<SceneTriangle> triangles;
      std::vector<SceneBox> boxes;
      std::vector<SceneLight> lights;
    };

    class SceneParser {
    public:
      static SceneData parse_scene_file(const std::string &filename);

    private:
      // Helper methods for parsing with nlohmann/json
      static Q::geometry::Vec3 parse_vec3(const nlohmann::json &vec_json);
      static Q::radiometry::Color parse_color(const nlohmann::json &color_json);
    };

  } // namespace io
} // namespace Q