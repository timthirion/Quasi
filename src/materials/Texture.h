#pragma once

#include "../radiometry/Color.h"

namespace Q::materials {

  /**
   * Abstract base class for texture sampling.
   * Textures provide color values at given UV coordinates.
   */
  class Texture {
  public:
    virtual ~Texture() = default;

    /**
     * Sample the texture at the given UV coordinates.
     * @param u Horizontal texture coordinate (typically 0-1)
     * @param v Vertical texture coordinate (typically 0-1)
     * @return Color at the specified UV coordinates
     */
    virtual Q::radiometry::Color sample(float u, float v) const = 0;
  };

} // namespace Q::materials