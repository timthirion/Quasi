#pragma once

#include "../geometry/geometry.hpp"
#include "../materials/checkerboard_texture.hpp"
#include "../radiometry/camera.hpp"
#include "../radiometry/color.hpp"
#include <memory>
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
      static float parse_float(const std::string &str);
      static Q::geometry::Vec3 parse_vec3(const std::string &str);
      static Q::radiometry::Color parse_color(const std::string &str);
      static std::string trim(const std::string &str);
      static std::vector<std::string> split(const std::string &str, char delimiter);

      // JSON parsing helpers
      static float parse_float_from_json(const std::string &content, const std::string &key);
      static Q::geometry::Vec3 parse_vec3_from_json(const std::string &content,
                                                    const std::string &key);
      static Q::radiometry::Color parse_color_from_json(const std::string &content,
                                                        const std::string &key);
      static void parse_spheres_from_json(const std::string &content,
                                          std::vector<SceneSphere> &spheres);
      static void parse_triangles_from_json(const std::string &content,
                                            std::vector<SceneTriangle> &triangles);
      static void parse_boxes_from_json(const std::string &content, std::vector<SceneBox> &boxes);
      static void parse_lights_from_json(const std::string &content,
                                         std::vector<SceneLight> &lights);
    };

  } // namespace io
} // namespace Q