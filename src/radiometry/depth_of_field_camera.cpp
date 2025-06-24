#include "depth_of_field_camera.hpp"
#include <algorithm>

namespace Q::radiometry {

  DepthOfFieldCamera::DepthOfFieldCamera(Q::geometry::Vec3 look_from, Q::geometry::Vec3 look_at,
                                         Q::geometry::Vec3 vup, float vfov, float aspect_ratio,
                                         float aperture, float focus_dist)
      : lens_radius(aperture / 2.0f), focus_distance(focus_dist), gen(rd()), dist(0.0f, 1.0f) {

    // Calculate viewport dimensions
    float theta = vfov * M_PI / 180.0f;
    float half_height = std::tan(theta / 2.0f);
    float half_width = aspect_ratio * half_height;

    // Set up camera coordinate system
    origin = look_from;
    w = (look_from - look_at).get_normalized();
    u = vup.cross_product(w).get_normalized();
    v = w.cross_product(u);

    // Calculate viewport corners at focus distance
    lower_left_corner = origin - u * (half_width * focus_distance) -
                        v * (half_height * focus_distance) - w * focus_distance;
    horizontal = u * (2.0f * half_width * focus_distance);
    vertical = v * (2.0f * half_height * focus_distance);
  }

  Q::geometry::Vec3 DepthOfFieldCamera::sample_aperture() const {
    // Uniform sampling on unit disk using rejection method
    Q::geometry::Vec3 p;
    do {
      p = Q::geometry::Vec3(2.0f * dist(gen) - 1.0f, 2.0f * dist(gen) - 1.0f, 0.0f);
    } while (p.dot_product(p) >= 1.0f);

    return p * lens_radius;
  }

  Q::geometry::Vec3 DepthOfFieldCamera::sample_aperture_blue_noise() const {
    // For now, fall back to uniform sampling
    // TODO: Implement blue noise disk sampling for better quality
    return sample_aperture();
  }

  Q::geometry::Ray DepthOfFieldCamera::get_ray(float s, float t) const {
    if (lens_radius <= 0.0f) {
      // Pinhole camera - no depth of field
      Q::geometry::Vec3 direction = lower_left_corner + horizontal * s + vertical * t - origin;
      return Q::geometry::Ray(origin, direction.get_normalized());
    }

    // Sample aperture for depth of field
    Q::geometry::Vec3 rd = sample_aperture();
    Q::geometry::Vec3 offset = u * rd.x + v * rd.y;
    Q::geometry::Vec3 aperture_origin = origin + offset;

    // Calculate ray direction through focus plane
    Q::geometry::Vec3 focus_point = lower_left_corner + horizontal * s + vertical * t;
    Q::geometry::Vec3 direction = focus_point - aperture_origin;

    return Q::geometry::Ray(aperture_origin, direction.get_normalized());
  }

  Q::geometry::Ray
  DepthOfFieldCamera::get_ray_with_aperture_sample(float s, float t,
                                                   const Q::geometry::Vec3 &aperture_sample) const {
    if (lens_radius <= 0.0f) {
      // Pinhole camera - ignore aperture sample
      Q::geometry::Vec3 direction = lower_left_corner + horizontal * s + vertical * t - origin;
      return Q::geometry::Ray(origin, direction.get_normalized());
    }

    // Convert [0,1]x[0,1] sample to disk coordinates
    float r = std::sqrt(aperture_sample.x);
    float theta = 2.0f * M_PI * aperture_sample.y;
    Q::geometry::Vec3 rd(r * std::cos(theta) * lens_radius, r * std::sin(theta) * lens_radius,
                         0.0f);

    Q::geometry::Vec3 offset = u * rd.x + v * rd.y;
    Q::geometry::Vec3 aperture_origin = origin + offset;

    // Calculate ray direction through focus plane
    Q::geometry::Vec3 focus_point = lower_left_corner + horizontal * s + vertical * t;
    Q::geometry::Vec3 direction = focus_point - aperture_origin;

    return Q::geometry::Ray(aperture_origin, direction.get_normalized());
  }

} // namespace Q::radiometry