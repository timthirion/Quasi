#include "scene_parser.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace Q {
  namespace io {

    SceneData SceneParser::parse_scene_file(const std::string &filename) {
      std::ifstream file(filename);
      if (!file.is_open()) {
        throw std::runtime_error("Could not open scene file: " + filename);
      }

      std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      file.close();

      SceneData scene;

      // Simple JSON parsing for our specific format
      // This is a basic parser - in production you'd use nlohmann/json or similar

      try {
        // Parse camera
        scene.camera.position = parse_vec3_from_json(content, "\"position\"");
        scene.camera.look_at = parse_vec3_from_json(content, "\"look_at\"");
        scene.camera.up = parse_vec3_from_json(content, "\"up\"");
        scene.camera.fov = parse_float_from_json(content, "\"fov\"");

        // Parse render settings
        scene.render.width = static_cast<int>(parse_float_from_json(content, "\"width\""));
        scene.render.height = static_cast<int>(parse_float_from_json(content, "\"height\""));

        // Parse background
        scene.background.color1 = parse_color_from_json(content, "\"color1\"");
        scene.background.color2 = parse_color_from_json(content, "\"color2\"");
        scene.background.rows = static_cast<int>(parse_float_from_json(content, "\"rows\""));
        scene.background.columns = static_cast<int>(parse_float_from_json(content, "\"columns\""));
        scene.background.distance = parse_float_from_json(content, "\"distance\"");

        // Parse objects
        parse_spheres_from_json(content, scene.spheres);
        parse_triangles_from_json(content, scene.triangles);
        parse_boxes_from_json(content, scene.boxes);
        parse_lights_from_json(content, scene.lights);

      } catch (const std::exception &e) {
        throw std::runtime_error("Error parsing scene file: " + std::string(e.what()));
      }

      return scene;
    }

    float SceneParser::parse_float(const std::string &str) {
      return std::stof(trim(str));
    }

    Q::geometry::Vec3 SceneParser::parse_vec3(const std::string &str) {
      auto values = split(str, ',');
      if (values.size() != 3) {
        throw std::runtime_error("Vec3 must have exactly 3 components");
      }
      return Q::geometry::Vec3(parse_float(values[0]), parse_float(values[1]),
                               parse_float(values[2]));
    }

    Q::radiometry::Color SceneParser::parse_color(const std::string &str) {
      auto values = split(str, ',');
      if (values.size() != 3) {
        throw std::runtime_error("Color must have exactly 3 components");
      }
      return Q::radiometry::Color(parse_float(values[0]), parse_float(values[1]),
                                  parse_float(values[2]));
    }

    std::string SceneParser::trim(const std::string &str) {
      auto start = str.find_first_not_of(" \t\n\r[]");
      if (start == std::string::npos)
        return "";
      auto end = str.find_last_not_of(" \t\n\r[]");
      return str.substr(start, end - start + 1);
    }

    std::vector<std::string> SceneParser::split(const std::string &str, char delimiter) {
      std::vector<std::string> tokens;
      std::string token;
      std::istringstream tokenStream(str);
      while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(trim(token));
      }
      return tokens;
    }

    // Helper functions for JSON parsing
    float SceneParser::parse_float_from_json(const std::string &content, const std::string &key) {
      auto pos = content.find(key);
      if (pos == std::string::npos) {
        throw std::runtime_error("Key not found: " + key);
      }

      // Find the value after the key
      pos = content.find(":", pos);
      if (pos == std::string::npos) {
        throw std::runtime_error("Invalid format for key: " + key);
      }

      // Find the start of the value
      pos = content.find_first_not_of(" \t\n\r", pos + 1);
      if (pos == std::string::npos) {
        throw std::runtime_error("No value found for key: " + key);
      }

      // Find the end of the value (comma or closing bracket)
      auto end_pos = content.find_first_of(",}\n", pos);
      if (end_pos == std::string::npos) {
        end_pos = content.length();
      }

      std::string value = content.substr(pos, end_pos - pos);
      return parse_float(value);
    }

    Q::geometry::Vec3 SceneParser::parse_vec3_from_json(const std::string &content,
                                                        const std::string &key) {
      auto pos = content.find(key);
      if (pos == std::string::npos) {
        throw std::runtime_error("Key not found: " + key);
      }

      // Find the array start
      pos = content.find("[", pos);
      if (pos == std::string::npos) {
        throw std::runtime_error("Array not found for key: " + key);
      }

      // Find the array end
      auto end_pos = content.find("]", pos);
      if (end_pos == std::string::npos) {
        throw std::runtime_error("Array not closed for key: " + key);
      }

      std::string array_content = content.substr(pos + 1, end_pos - pos - 1);
      return parse_vec3(array_content);
    }

    Q::radiometry::Color SceneParser::parse_color_from_json(const std::string &content,
                                                            const std::string &key) {
      auto vec = parse_vec3_from_json(content, key);
      return Q::radiometry::Color(vec.x, vec.y, vec.z);
    }

    void SceneParser::parse_spheres_from_json(const std::string &content,
                                              std::vector<SceneSphere> &spheres) {
      // Find the objects array
      auto objects_pos = content.find("\"objects\"");
      if (objects_pos == std::string::npos) {
        return; // No objects defined
      }

      auto array_start = content.find("[", objects_pos);
      if (array_start == std::string::npos) {
        return;
      }

      // Find matching closing bracket
      int bracket_count = 0;
      auto array_end = array_start;
      for (size_t i = array_start; i < content.length(); i++) {
        if (content[i] == '[')
          bracket_count++;
        else if (content[i] == ']') {
          bracket_count--;
          if (bracket_count == 0) {
            array_end = i;
            break;
          }
        }
      }

      std::string objects_content = content.substr(array_start + 1, array_end - array_start - 1);

      // Parse each object (simple parsing for sphere objects)
      size_t pos = 0;
      while (pos < objects_content.length()) {
        auto obj_start = objects_content.find("{", pos);
        if (obj_start == std::string::npos)
          break;

        auto obj_end = objects_content.find("}", obj_start);
        if (obj_end == std::string::npos)
          break;

        std::string obj_content = objects_content.substr(obj_start, obj_end - obj_start + 1);

        // Check if it's a sphere
        if (obj_content.find("\"sphere\"") != std::string::npos) {
          SceneSphere sphere;
          sphere.center = parse_vec3_from_json(obj_content, "\"center\"");
          sphere.radius = parse_float_from_json(obj_content, "\"radius\"");
          sphere.color = parse_color_from_json(obj_content, "\"color\"");
          spheres.push_back(sphere);
        }

        pos = obj_end + 1;
      }
    }

    void SceneParser::parse_triangles_from_json(const std::string &content,
                                                std::vector<SceneTriangle> &triangles) {
      // Find the objects array (triangles are in objects)
      auto objects_pos = content.find("\"objects\"");
      if (objects_pos == std::string::npos) {
        return; // No objects defined
      }

      auto array_start = content.find("[", objects_pos);
      if (array_start == std::string::npos) {
        return;
      }

      // Find matching closing bracket
      int bracket_count = 0;
      auto array_end = array_start;
      for (size_t i = array_start; i < content.length(); i++) {
        if (content[i] == '[')
          bracket_count++;
        else if (content[i] == ']') {
          bracket_count--;
          if (bracket_count == 0) {
            array_end = i;
            break;
          }
        }
      }

      std::string objects_content = content.substr(array_start + 1, array_end - array_start - 1);

      // Parse each object (looking for triangle objects)
      size_t pos = 0;
      while (pos < objects_content.length()) {
        auto obj_start = objects_content.find("{", pos);
        if (obj_start == std::string::npos)
          break;

        auto obj_end = objects_content.find("}", obj_start);
        if (obj_end == std::string::npos)
          break;

        std::string obj_content = objects_content.substr(obj_start, obj_end - obj_start + 1);

        // Check if it's a triangle
        if (obj_content.find("\"type\": \"triangle\"") != std::string::npos) {
          SceneTriangle triangle;
          triangle.vertex1 = parse_vec3_from_json(obj_content, "\"vertex1\"");
          triangle.vertex2 = parse_vec3_from_json(obj_content, "\"vertex2\"");
          triangle.vertex3 = parse_vec3_from_json(obj_content, "\"vertex3\"");
          triangle.color = parse_color_from_json(obj_content, "\"color\"");
          triangles.push_back(triangle);
        }

        pos = obj_end + 1;
      }
    }

    void SceneParser::parse_boxes_from_json(const std::string &content,
                                            std::vector<SceneBox> &boxes) {
      // Find the boxes array
      auto boxes_pos = content.find("\"boxes\"");
      if (boxes_pos == std::string::npos) {
        return; // No boxes defined
      }

      auto array_start = content.find("[", boxes_pos);
      if (array_start == std::string::npos) {
        return;
      }

      // Find matching closing bracket
      int bracket_count = 0;
      auto array_end = array_start;
      for (size_t i = array_start; i < content.length(); i++) {
        if (content[i] == '[')
          bracket_count++;
        else if (content[i] == ']') {
          bracket_count--;
          if (bracket_count == 0) {
            array_end = i;
            break;
          }
        }
      }

      std::string boxes_content = content.substr(array_start + 1, array_end - array_start - 1);

      // Parse each box object
      size_t pos = 0;
      while (pos < boxes_content.length()) {
        auto obj_start = boxes_content.find("{", pos);
        if (obj_start == std::string::npos)
          break;

        auto obj_end = boxes_content.find("}", obj_start);
        if (obj_end == std::string::npos)
          break;

        std::string obj_content = boxes_content.substr(obj_start, obj_end - obj_start + 1);

        // Check if it's a box
        if (obj_content.find("\"box\"") != std::string::npos) {
          SceneBox box;
          box.min_corner = parse_vec3_from_json(obj_content, "\"min\"");
          box.max_corner = parse_vec3_from_json(obj_content, "\"max\"");
          box.color = parse_color_from_json(obj_content, "\"color\"");
          boxes.push_back(box);
        }

        pos = obj_end + 1;
      }
    }

    void SceneParser::parse_lights_from_json(const std::string &content,
                                             std::vector<SceneLight> &lights) {
      // Find the lights array
      auto lights_pos = content.find("\"lights\"");
      if (lights_pos == std::string::npos) {
        return; // No lights defined
      }

      auto array_start = content.find("[", lights_pos);
      if (array_start == std::string::npos) {
        return;
      }

      // Find matching closing bracket
      int bracket_count = 0;
      auto array_end = array_start;
      for (size_t i = array_start; i < content.length(); i++) {
        if (content[i] == '[')
          bracket_count++;
        else if (content[i] == ']') {
          bracket_count--;
          if (bracket_count == 0) {
            array_end = i;
            break;
          }
        }
      }

      std::string lights_content = content.substr(array_start + 1, array_end - array_start - 1);

      // Parse each light object
      size_t pos = 0;
      while (pos < lights_content.length()) {
        auto obj_start = lights_content.find("{", pos);
        if (obj_start == std::string::npos)
          break;

        auto obj_end = lights_content.find("}", obj_start);
        if (obj_end == std::string::npos)
          break;

        std::string obj_content = lights_content.substr(obj_start, obj_end - obj_start + 1);

        // Check if it's a light
        if (obj_content.find("\"light\"") != std::string::npos ||
            obj_content.find("\"point_light\"") != std::string::npos) {
          SceneLight light;
          light.position = parse_vec3_from_json(obj_content, "\"position\"");
          light.color = parse_color_from_json(obj_content, "\"color\"");
          light.intensity = parse_float_from_json(obj_content, "\"intensity\"");
          lights.push_back(light);
        }

        pos = obj_end + 1;
      }
    }

  } // namespace io
} // namespace Q