#include "scene_parser.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

namespace Q {
  namespace io {

    SceneData SceneParser::parse_scene_file(const std::string &filename) {
      std::ifstream file(filename);
      if (!file.is_open()) {
        throw std::runtime_error("Could not open scene file: " + filename);
      }

      json scene_json;
      try {
        file >> scene_json;
      } catch (const json::parse_error &e) {
        throw std::runtime_error("JSON parse error in file " + filename + ": " + e.what());
      }
      file.close();

      SceneData scene;

      try {
        // Parse camera
        if (scene_json.contains("camera")) {
          const auto &camera = scene_json["camera"];
          scene.camera.position = parse_vec3(camera["position"]);
          scene.camera.look_at = parse_vec3(camera["look_at"]);
          scene.camera.up = parse_vec3(camera["up"]);
          scene.camera.fov = camera["fov"].get<float>();

          // Parse depth of field parameters (optional)
          if (camera.contains("aperture")) {
            scene.camera.aperture = camera["aperture"].get<float>();
          }
          if (camera.contains("focus_distance")) {
            scene.camera.focus_distance = camera["focus_distance"].get<float>();
          }
          if (camera.contains("f_stop")) {
            // Convert f-stop to aperture (assuming focal length of 1.0)
            float f_stop = camera["f_stop"].get<float>();
            scene.camera.aperture = 1.0f / f_stop;
          }
        }

        // Parse render settings
        if (scene_json.contains("render")) {
          const auto &render = scene_json["render"];
          scene.render.width = render["width"].get<int>();
          scene.render.height = render["height"].get<int>();

          // Parse multisampling settings (optional)
          if (render.contains("multisampling")) {
            const auto &ms = render["multisampling"];
            if (ms.contains("samples_per_pixel")) {
              scene.render.multisampling.samples_per_pixel = ms["samples_per_pixel"].get<int>();
            }
            if (ms.contains("sampling_pattern")) {
              scene.render.multisampling.sampling_pattern =
                  ms["sampling_pattern"].get<std::string>();
            }
            if (ms.contains("sample_integrator")) {
              scene.render.multisampling.sample_integrator =
                  ms["sample_integrator"].get<std::string>();
            }
            // Parse adaptive sampling parameters
            if (ms.contains("max_samples_per_pixel")) {
              scene.render.multisampling.max_samples_per_pixel =
                  ms["max_samples_per_pixel"].get<int>();
            }
            if (ms.contains("variance_threshold")) {
              scene.render.multisampling.variance_threshold = ms["variance_threshold"].get<float>();
            }
            if (ms.contains("adaptation_levels")) {
              scene.render.multisampling.adaptation_levels = ms["adaptation_levels"].get<int>();
            }
          }
        }

        // Parse background
        if (scene_json.contains("background")) {
          const auto &background = scene_json["background"];
          scene.background.color1 = parse_color(background["color1"]);
          scene.background.color2 = parse_color(background["color2"]);
          scene.background.rows = background["rows"].get<int>();
          scene.background.columns = background["columns"].get<int>();
          scene.background.distance = background["distance"].get<float>();
        }

        // Parse spheres from objects array
        if (scene_json.contains("objects")) {
          for (const auto &obj : scene_json["objects"]) {
            if (obj.contains("type") && obj["type"] == "sphere") {
              SceneSphere sphere;
              sphere.center = parse_vec3(obj["center"]);
              sphere.radius = obj["radius"].get<float>();
              sphere.color = parse_color(obj["color"]);
              if (obj.contains("reflectance")) {
                sphere.reflectance = obj["reflectance"].get<float>();
              }
              scene.spheres.push_back(sphere);
            }
            if (obj.contains("type") && obj["type"] == "triangle") {
              SceneTriangle triangle;
              triangle.vertex1 = parse_vec3(obj["vertex1"]);
              triangle.vertex2 = parse_vec3(obj["vertex2"]);
              triangle.vertex3 = parse_vec3(obj["vertex3"]);
              triangle.color = parse_color(obj["color"]);
              if (obj.contains("reflectance")) {
                triangle.reflectance = obj["reflectance"].get<float>();
              }
              scene.triangles.push_back(triangle);
            }
          }
        }

        // Parse boxes
        if (scene_json.contains("boxes")) {
          for (const auto &box_obj : scene_json["boxes"]) {
            SceneBox box;
            box.min_corner = parse_vec3(box_obj["min"]);
            box.max_corner = parse_vec3(box_obj["max"]);
            box.color = parse_color(box_obj["color"]);
            if (box_obj.contains("reflectance")) {
              box.reflectance = box_obj["reflectance"].get<float>();
            }
            scene.boxes.push_back(box);
          }
        }

        // Parse lights
        if (scene_json.contains("lights")) {
          for (const auto &light_obj : scene_json["lights"]) {
            SceneLight light;
            light.position = parse_vec3(light_obj["position"]);
            light.color = parse_color(light_obj["color"]);
            light.intensity = light_obj["intensity"].get<float>();

            // Parse light type (default to point_light for backward compatibility)
            if (light_obj.contains("type")) {
              light.type = light_obj["type"].get<std::string>();
            }

            // Parse area light specific parameters
            if (light.type == "rectangular_area_light") {
              if (light_obj.contains("width")) {
                light.width = light_obj["width"].get<float>();
              }
              if (light_obj.contains("height")) {
                light.height = light_obj["height"].get<float>();
              }
              if (light_obj.contains("u_axis")) {
                light.u_axis = parse_vec3(light_obj["u_axis"]);
              }
              if (light_obj.contains("v_axis")) {
                light.v_axis = parse_vec3(light_obj["v_axis"]);
              }
              if (light_obj.contains("samples")) {
                light.samples = light_obj["samples"].get<int>();
              }
              if (light_obj.contains("sampling_method")) {
                light.sampling_method = light_obj["sampling_method"].get<std::string>();
              }
            }

            scene.lights.push_back(light);
          }
        }

      } catch (const json::exception &e) {
        throw std::runtime_error("Error parsing scene file " + filename + ": " + e.what());
      } catch (const std::exception &e) {
        throw std::runtime_error("Error parsing scene file " + filename + ": " + e.what());
      }

      return scene;
    }

    Q::geometry::Vec3 SceneParser::parse_vec3(const json &vec_json) {
      if (!vec_json.is_array() || vec_json.size() != 3) {
        throw std::runtime_error("Vec3 must be an array of 3 numbers");
      }
      return Q::geometry::Vec3(vec_json[0].get<float>(), vec_json[1].get<float>(),
                               vec_json[2].get<float>());
    }

    Q::radiometry::Color SceneParser::parse_color(const json &color_json) {
      if (!color_json.is_array() || color_json.size() != 3) {
        throw std::runtime_error("Color must be an array of 3 numbers");
      }
      return Q::radiometry::Color(color_json[0].get<float>(), color_json[1].get<float>(),
                                  color_json[2].get<float>());
    }

  } // namespace io
} // namespace Q