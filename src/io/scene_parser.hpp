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

    struct MultisamplingSettings {
      int samples_per_pixel;
      std::string sampling_pattern;
      std::string sample_integrator;

      // Default settings
      MultisamplingSettings()
          : samples_per_pixel(1), sampling_pattern("stratified"), sample_integrator("average") {}
    };

    struct RenderSettings {
      int width;
      int height;
      MultisamplingSettings multisampling;
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
      float reflectance; // [0,1] how reflective the surface is

      SceneSphere() : reflectance(0.0f) {}
    };

    struct SceneBox {
      Q::geometry::Vec3 min_corner;
      Q::geometry::Vec3 max_corner;
      Q::radiometry::Color color;
      float reflectance;

      SceneBox() : reflectance(0.0f) {}
    };

    struct SceneTriangle {
      Q::geometry::Vec3 vertex1;
      Q::geometry::Vec3 vertex2;
      Q::geometry::Vec3 vertex3;
      Q::radiometry::Color color;
      float reflectance;

      SceneTriangle() : reflectance(0.0f) {}
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