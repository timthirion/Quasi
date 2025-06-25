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

    // Using aliases for commonly used types
    using Vec3 = Q::geometry::Vec3;
    using Color = Q::radiometry::Color;

    struct SceneCamera {
      Vec3 position;
      Vec3 look_at;
      Vec3 up;
      float fov;

      // Depth of field parameters
      float aperture;       // Aperture size (0 = pinhole camera)
      float focus_distance; // Distance to focus plane

      // Default settings (pinhole camera)
      SceneCamera() : aperture(0.0f), focus_distance(1.0f) {}
    };

    struct MultisamplingSettings {
      int samples_per_pixel;
      std::string sampling_pattern;
      std::string sample_integrator;

      // Adaptive sampling parameters
      int max_samples_per_pixel;
      float variance_threshold;
      int adaptation_levels;

      // Default settings
      MultisamplingSettings()
          : samples_per_pixel(1), sampling_pattern("stratified"), sample_integrator("average"),
            max_samples_per_pixel(64), variance_threshold(0.01f), adaptation_levels(3) {}
    };

    struct RenderSettings {
      int width;
      int height;
      MultisamplingSettings multisampling;
    };

    struct BackgroundSettings {
      Color color1;
      Color color2;
      int rows;
      int columns;
      float distance;
    };

    struct SceneSphere {
      Vec3 center;
      float radius;
      Color color;
      float reflectance; // [0,1] how reflective the surface is

      SceneSphere() : reflectance(0.0f) {}
    };

    struct SceneBox {
      Vec3 min_corner;
      Vec3 max_corner;
      Color color;
      float reflectance;

      SceneBox() : reflectance(0.0f) {}
    };

    struct SceneTriangle {
      Vec3 vertex1;
      Vec3 vertex2;
      Vec3 vertex3;
      Color color;
      float reflectance;

      SceneTriangle() : reflectance(0.0f) {}
    };

    struct SceneMesh {
      std::string filename;
      Vec3 position;
      float scale;
      Color color;
      float reflectance;

      SceneMesh() : position(0, 0, 0), scale(1.0f), reflectance(0.0f) {}
    };

    struct SceneLight {
      Vec3 position;
      Color color;
      float intensity;
      std::string type;

      // Area light specific parameters
      Vec3 u_axis;
      Vec3 v_axis;
      float width;
      float height;
      int samples;
      std::string sampling_method;

      // Default constructor for point lights
      SceneLight()
          : type("point_light"), width(0.0f), height(0.0f), samples(16),
            sampling_method("stratified") {}
    };

    struct SceneData {
      SceneCamera camera;
      RenderSettings render;
      BackgroundSettings background;
      std::vector<SceneSphere> spheres;
      std::vector<SceneTriangle> triangles;
      std::vector<SceneBox> boxes;
      std::vector<SceneMesh> meshes;
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