#pragma once

#include "../geometry/geometry.hpp"
#include "../sampling/blue_noise_pattern.hpp"
#include <cmath>
#include <random>

namespace Q::radiometry {

  // Using aliases for commonly used types
  using Vec3 = Q::geometry::Vec3;
  using Ray = Q::geometry::Ray;

  /**
   * Enhanced camera with depth of field support using thin lens model.
   * Provides realistic aperture effects and bokeh for artistic control.
   */
  class DepthOfFieldCamera {
  private:
    // Camera coordinate system
    Vec3 origin;
    Vec3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
    Vec3 u, v, w; // Camera basis vectors

    // Depth of field parameters
    float lens_radius;    // Half of aperture diameter
    float focus_distance; // Distance to focus plane

    // Random sampling for aperture
    mutable std::random_device rd;
    mutable std::mt19937 gen;
    mutable std::uniform_real_distribution<float> dist;

    /**
     * Sample a point on the circular aperture using uniform disk sampling
     */
    Vec3 sample_aperture() const;

    /**
     * Sample aperture using blue noise for better quality (when available)
     */
    Vec3 sample_aperture_blue_noise() const;

  public:
    /**
     * Constructor for depth of field camera
     * @param look_from Camera position
     * @param look_at Point camera is looking at
     * @param vup Up vector for camera orientation
     * @param vfov Vertical field of view in degrees
     * @param aspect_ratio Width/height ratio
     * @param aperture Aperture size (larger = more blur). 0 = pinhole camera
     * @param focus_dist Distance to focus plane
     */
    DepthOfFieldCamera(Vec3 look_from, Vec3 look_at, Vec3 vup, float vfov, float aspect_ratio,
                       float aperture, float focus_dist);

    /**
     * Get ray for given screen coordinates with depth of field
     * @param s Horizontal coordinate [0,1]
     * @param t Vertical coordinate [0,1]
     * @return Ray from aperture sample through focus plane
     */
    Ray get_ray(float s, float t) const;

    /**
     * Get ray with explicit aperture sample for deterministic sampling
     * @param s Horizontal coordinate [0,1]
     * @param t Vertical coordinate [0,1]
     * @param aperture_sample 2D sample point for aperture [0,1]x[0,1]
     * @return Ray from specific aperture point through focus plane
     */
    Ray get_ray_with_aperture_sample(float s, float t, const Vec3 &aperture_sample) const;

    // Getters for camera parameters
    float get_aperture() const { return lens_radius * 2.0f; }
    float get_focus_distance() const { return focus_distance; }
    Vec3 get_position() const { return origin; }

    /**
     * Convert f-stop to aperture size for more intuitive control
     * @param f_stop F-stop number (e.g., 1.4, 2.8, 5.6)
     * @param focal_length Lens focal length in scene units
     * @return Aperture diameter
     */
    static float f_stop_to_aperture(float f_stop, float focal_length = 1.0f) {
      return focal_length / f_stop;
    }
  };

} // namespace Q::radiometry