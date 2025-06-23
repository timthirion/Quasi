#pragma once

#include "../radiometry/Color.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace Q::io {

  class PPMWriter {
  public:
    static void write_ppm(const std::string &filename,
                          const std::vector<Q::radiometry::Color> &pixels, int width, int height) {
      std::ofstream file(filename);
      if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return;
      }

      // PPM header
      file << "P3\n";
      file << width << " " << height << "\n";
      file << "255\n";

      // Pixel data
      for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
          const Q::radiometry::Color &pixel = pixels[y * width + x];
          file << pixel.r_int() << " " << pixel.g_int() << " " << pixel.b_int() << " ";
        }
        file << "\n";
      }

      file.close();
      std::cout << "Image written to " << filename << std::endl;
    }
  };

} // namespace Q::io