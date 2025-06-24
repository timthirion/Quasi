#pragma once

#include "../radiometry/color.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace Q::io {

  enum class ToneMapType {
    NONE,     // No tone mapping (clamp to [0,1])
    REINHARD, // Reinhard tone mapping
    EXPOSURE, // Exposure adjustment + Reinhard
    ACES      // ACES tone mapping
  };

  class PPMWriter {
  public:
    // Enhanced PPM writer with tone mapping support
    static void write_ppm(const std::string &filename,
                          const std::vector<Q::radiometry::Color> &pixels, int width, int height,
                          ToneMapType tone_map = ToneMapType::REINHARD, float exposure = 0.0f,
                          float gamma = 2.2f) {
      std::ofstream file(filename);
      if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return;
      }

      // PPM header
      file << "P3\n";
      file << width << " " << height << "\n";
      file << "255\n";

      // Pixel data with tone mapping
      for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
          Q::radiometry::Color pixel = pixels[y * width + x];

          // Apply tone mapping
          switch (tone_map) {
          case ToneMapType::NONE:
            // Just clamp to [0,1] - no tone mapping
            pixel = Q::radiometry::Color(std::clamp(pixel.r, 0.0f, 1.0f),
                                         std::clamp(pixel.g, 0.0f, 1.0f),
                                         std::clamp(pixel.b, 0.0f, 1.0f));
            break;
          case ToneMapType::REINHARD:
            pixel = pixel.tone_map_reinhard();
            break;
          case ToneMapType::EXPOSURE:
            pixel = pixel.tone_map_exposure(exposure);
            break;
          case ToneMapType::ACES:
            pixel = pixel.tone_map_aces();
            break;
          }

          // Apply gamma correction
          if (gamma != 1.0f) {
            pixel = pixel.apply_gamma(gamma);
          }

          file << pixel.r_int() << " " << pixel.g_int() << " " << pixel.b_int() << " ";
        }
        file << "\n";
      }

      file.close();
      std::cout << "Image written to " << filename << std::endl;
    }

    // Backward compatibility - original method without tone mapping
    static void write_ppm(const std::string &filename,
                          const std::vector<Q::radiometry::Color> &pixels, int width, int height) {
      write_ppm(filename, pixels, width, height, ToneMapType::NONE, 0.0f, 1.0f);
    }
  };

} // namespace Q::io