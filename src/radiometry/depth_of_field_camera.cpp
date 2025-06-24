#include "depth_of_field_camera.hpp"
#include <algorithm>

namespace Q::radiometry {

  // Using aliases for commonly used types
  using Vec3 = Q::geometry::Vec3;
  using Ray = Q::geometry::Ray;

  DepthOfFieldCamera::DepthOfFieldCamera(Vec3 look_from, Vec3 look_at, Vec3 vup, float vfov,
                                         float aspect_ratio, float aperture, float focus_dist)
      : lens_radius(aperture / 2.0f), focus_distance(focus_dist), gen(rd()), dist(0.0f, 1.0f) {

    // Calculate viewport dimensions
    float theta = vfov * M_PI / 180.0f;
    float half_height = std::tan(theta / 2.0f);
    float half_width = aspect_ratio * half_height;

    // Set up camera coordinate system
    origin = look_from;
    w = (look_from - look_at).get_normalized();
    u = vup.cross(w).get_normalized();
    v = w.cross(u);

    // Calculate viewport corners at focus distance
    lower_left_corner = origin - u * (half_width * focus_distance) -
                        v * (half_height * focus_distance) - w * focus_distance;
    horizontal = u * (2.0f * half_width * focus_distance);
    vertical = v * (2.0f * half_height * focus_distance);
  }

  Vec3 DepthOfFieldCamera::sample_aperture() const {
    // Uniform sampling on unit disk using rejection method
    Vec3 p;
    do {
      p = Vec3(2.0f * dist(gen) - 1.0f, 2.0f * dist(gen) - 1.0f, 0.0f);
    } while (p.dot(p) >= 1.0f);

    return p * lens_radius;
  }

  Vec3 DepthOfFieldCamera::sample_aperture_blue_noise() const {
    // For now, fall back to uniform sampling
    // TODO: Implement blue noise disk sampling for better quality
    return sample_aperture();
  }

  Ray DepthOfFieldCamera::get_ray(float s, float t) const {
    if (lens_radius <= 0.0f) {
      // Pinhole camera - no depth of field
      Vec3 direction = lower_left_corner + horizontal * s + vertical * t - origin;
      return Ray(origin, direction.get_normalized());
    }

    // Sample aperture for depth of field
    Vec3 rd = sample_aperture();
    Vec3 offset = u * rd.x + v * rd.y;
    Vec3 aperture_origin = origin + offset;

    // Calculate ray direction through focus plane
    Vec3 focus_point = lower_left_corner + horizontal * s + vertical * t;
    Vec3 direction = focus_point - aperture_origin;

    return Ray(aperture_origin, direction.get_normalized());
  }

  Ray DepthOfFieldCamera::get_ray_with_aperture_sample(float s, float t,
                                                       const Vec3 &aperture_sample) const {
    if (lens_radius <= 0.0f) {
      // Pinhole camera - ignore aperture sample
      Vec3 direction = lower_left_corner + horizontal * s + vertical * t - origin;
      return Ray(origin, direction.get_normalized());
    }

    // Convert [0,1]x[0,1] sample to disk coordinates
    float r = std::sqrt(aperture_sample.x);
    float theta = 2.0f * M_PI * aperture_sample.y;
    Vec3 rd(r * std::cos(theta) * lens_radius, r * std::sin(theta) * lens_radius, 0.0f);

    Vec3 offset = u * rd.x + v * rd.y;
    Vec3 aperture_origin = origin + offset;

    // Calculate ray direction through focus plane
    Vec3 focus_point = lower_left_corner + horizontal * s + vertical * t;
    Vec3 direction = focus_point - aperture_origin;

    return Ray(aperture_origin, direction.get_normalized());
  }

} // namespace Q::radiometry