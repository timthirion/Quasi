#pragma once

#include "../radiometry/Color.h"
#include "Texture.h"
#include <cmath>

namespace Q::materials {

  /**
   * A checkerboard texture that alternates between two colors in a grid pattern.
   */
  class CheckerboardTexture : public Texture {
  private:
    Q::radiometry::Color color1;
    Q::radiometry::Color color2;
    int rows;
    int columns;

  public:
    /**
     * Create a checkerboard texture.
     * @param color1 First color of the checkerboard
     * @param color2 Second color of the checkerboard
     * @param rows Number of rows in the checkerboard
     * @param columns Number of columns in the checkerboard
     */
    CheckerboardTexture(const Q::radiometry::Color &color1, const Q::radiometry::Color &color2,
                        int rows, int columns)
        : color1(color1), color2(color2), rows(rows), columns(columns) {}

    /**
     * Sample the checkerboard texture at the given UV coordinates.
     * @param u Horizontal texture coordinate (0-1)
     * @param v Vertical texture coordinate (0-1)
     * @return Color at the specified UV coordinates
     */
    Q::radiometry::Color sample(float u, float v) const override {
      // Wrap UV coordinates to [0,1) range (ensure we never hit exactly 1.0)
      u = u - std::floor(u);
      v = v - std::floor(v);
      if (u >= 1.0f)
        u = 0.0f;
      if (v >= 1.0f)
        v = 0.0f;

      // Convert UV to grid coordinates
      int grid_u = static_cast<int>(u * columns);
      int grid_v = static_cast<int>(v * rows);

      // Ensure grid coordinates are within bounds
      grid_u = std::max(0, std::min(grid_u, columns - 1));
      grid_v = std::max(0, std::min(grid_v, rows - 1));

      // Determine checkerboard pattern (alternating colors)
      bool is_even = (grid_u + grid_v) % 2 == 0;
      return is_even ? color1 : color2;
    }

    // Getters for inspection/testing
    const Q::radiometry::Color &get_color1() const { return color1; }
    const Q::radiometry::Color &get_color2() const { return color2; }
    int get_rows() const { return rows; }
    int get_columns() const { return columns; }
  };

} // namespace Q::materials