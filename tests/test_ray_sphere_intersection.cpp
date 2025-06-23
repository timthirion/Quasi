#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <quasi/geometry/geometry.h>

using namespace Q::geometry;

TEST_CASE("Ray-Sphere Intersection", "[ray][sphere][intersection]") {

  SECTION("Ray hits sphere from outside - direct center hit") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(-3, 0, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near == Catch::Approx(2.0f));
    REQUIRE(result->t_far == Catch::Approx(4.0f));
    REQUIRE(result->point_near.x == Catch::Approx(-1.0f));
    REQUIRE(result->point_far.x == Catch::Approx(1.0f));
  }

  SECTION("Ray hits sphere from outside - off-center hit") {
    Sphere sphere(Vec3(0, 0, 0), 2.0f);
    Ray ray(Vec3(-5, 1, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);

    // Calculate expected intersection points
    float expected_x_near = -std::sqrt(3.0f);
    float expected_x_far = std::sqrt(3.0f);

    REQUIRE(result->point_near.x == Catch::Approx(expected_x_near));
    REQUIRE(result->point_near.y == Catch::Approx(1.0f));
    REQUIRE(result->point_far.x == Catch::Approx(expected_x_far));
    REQUIRE(result->point_far.y == Catch::Approx(1.0f));
  }

  SECTION("Ray misses sphere - parallel") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(-3, 2, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Ray misses sphere - pointing away") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(-3, 0, 0), Vec3(-1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Ray starts inside sphere") {
    Sphere sphere(Vec3(0, 0, 0), 2.0f);
    Ray ray(Vec3(0, 0, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near == Catch::Approx(-2.0f));
    REQUIRE(result->t_far == Catch::Approx(2.0f));
  }

  SECTION("Ray is tangent to sphere") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(-3, 1, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near == Catch::Approx(result->t_far));
    REQUIRE(result->point_near.x == Catch::Approx(0.0f));
    REQUIRE(result->point_near.y == Catch::Approx(1.0f));
  }

  SECTION("Ray hits sphere at origin") {
    Sphere sphere(Vec3(5, 3, 2), 1.5f);
    Ray ray(Vec3(0, 0, 0), Vec3(5, 3, 2).get_normalized());

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);

    Vec3 center_to_origin = Vec3(0, 0, 0) - sphere.center;
    float distance_to_center = center_to_origin.get_length();
    float expected_t_near = distance_to_center - sphere.radius;
    float expected_t_far = distance_to_center + sphere.radius;

    REQUIRE(result->t_near == Catch::Approx(expected_t_near));
    REQUIRE(result->t_far == Catch::Approx(expected_t_far));
  }

  SECTION("Ray with diagonal direction") {
    Sphere sphere(Vec3(0, 0, 0), std::sqrt(3.0f));
    Ray ray(Vec3(-2, -2, -2), Vec3(1, 1, 1).get_normalized());

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);

    // Ray should hit sphere symmetrically
    REQUIRE(result->t_near > 0);
    REQUIRE(result->t_far > result->t_near);
  }

  SECTION("Very small sphere") {
    Sphere sphere(Vec3(0, 0, 0), 0.001f);
    Ray ray(Vec3(-1, 0, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near == Catch::Approx(0.999f).margin(1e-6));
    REQUIRE(result->t_far == Catch::Approx(1.001f).margin(1e-6));
  }

  SECTION("Very large sphere") {
    Sphere sphere(Vec3(0, 0, 0), 1000.0f);
    Ray ray(Vec3(-500, 0, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near == Catch::Approx(-500.0f));
    REQUIRE(result->t_far == Catch::Approx(1500.0f));
  }

  SECTION("Ray barely misses sphere") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(-3, 1.0001f, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Ray hits sphere with negative t values only") {
    Sphere sphere(Vec3(-5, 0, 0), 1.0f);
    Ray ray(Vec3(0, 0, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("Sphere at origin, ray from positive axis") {
    Sphere sphere(Vec3(0, 0, 0), 2.0f);
    Ray ray(Vec3(5, 0, 0), Vec3(-1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near == Catch::Approx(3.0f));
    REQUIRE(result->t_far == Catch::Approx(7.0f));
    REQUIRE(result->point_near.x == Catch::Approx(2.0f));
    REQUIRE(result->point_far.x == Catch::Approx(-2.0f));
  }

  SECTION("Ray intersects sphere at grazing angle") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(-2, 0.9f, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);

    // Should have two distinct intersection points
    REQUIRE(result->t_far > result->t_near);
    REQUIRE(result->point_near.y == Catch::Approx(0.9f));
    REQUIRE(result->point_far.y == Catch::Approx(0.9f));
  }

  SECTION("Normal vectors are correctly calculated") {
    Sphere sphere(Vec3(1, 2, 3), 2.0f);
    Ray ray(Vec3(-5, 2, 3), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);

    // Check that normals point outward from sphere center
    Vec3 expected_normal_near = (result->point_near - sphere.center).get_normalized();
    Vec3 expected_normal_far = (result->point_far - sphere.center).get_normalized();

    REQUIRE(result->normal_near.x == Catch::Approx(expected_normal_near.x));
    REQUIRE(result->normal_near.y == Catch::Approx(expected_normal_near.y));
    REQUIRE(result->normal_near.z == Catch::Approx(expected_normal_near.z));

    REQUIRE(result->normal_far.x == Catch::Approx(expected_normal_far.x));
    REQUIRE(result->normal_far.y == Catch::Approx(expected_normal_far.y));
    REQUIRE(result->normal_far.z == Catch::Approx(expected_normal_far.z));
  }

  SECTION("Ray hits sphere from different directions") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);

    // Test from +Y direction
    Ray ray_y(Vec3(0, 3, 0), Vec3(0, -1, 0));
    auto result_y = ray_sphere_intersection(ray_y, sphere);
    REQUIRE(result_y.has_value());
    REQUIRE(result_y->point_near.y == Catch::Approx(1.0f));
    REQUIRE(result_y->point_far.y == Catch::Approx(-1.0f));

    // Test from +Z direction
    Ray ray_z(Vec3(0, 0, 3), Vec3(0, 0, -1));
    auto result_z = ray_sphere_intersection(ray_z, sphere);
    REQUIRE(result_z.has_value());
    REQUIRE(result_z->point_near.z == Catch::Approx(1.0f));
    REQUIRE(result_z->point_far.z == Catch::Approx(-1.0f));
  }

  SECTION("Sphere methods work correctly") {
    Sphere sphere(Vec3(1, 2, 3), 2.0f);

    // Test contains_point
    REQUIRE(sphere.contains_point(Vec3(1, 2, 3)) == true);  // center
    REQUIRE(sphere.contains_point(Vec3(3, 2, 3)) == true);  // on surface
    REQUIRE(sphere.contains_point(Vec3(4, 2, 3)) == false); // outside

    // Test surface area and volume
    REQUIRE(sphere.get_surface_area() == Catch::Approx(4.0f * M_PI * 4.0f));
    REQUIRE(sphere.get_volume() == Catch::Approx((4.0f / 3.0f) * M_PI * 8.0f));

    // Test normal calculation
    Vec3 point_on_surface(3, 2, 3);
    Vec3 normal = sphere.get_normal_at(point_on_surface);
    REQUIRE(normal.x == Catch::Approx(1.0f));
    REQUIRE(normal.y == Catch::Approx(0.0f));
    REQUIRE(normal.z == Catch::Approx(0.0f));
  }

  SECTION("Edge case: Ray origin exactly on sphere surface") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(1, 0, 0), Vec3(1, 0, 0)); // Starting on sphere surface

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    // One intersection should be at t=0, the other at t=2
    REQUIRE((result->t_near == Catch::Approx(0.0f) || result->t_far == Catch::Approx(0.0f)));
  }

  SECTION("Ray with zero-length direction (should be normalized)") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(-2, 0, 0), Vec3(2, 0, 0)); // Direction will be normalized to (1,0,0)

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near == Catch::Approx(1.0f));
    REQUIRE(result->t_far == Catch::Approx(3.0f));
  }

  SECTION("Multiple sphere intersections consistency") {
    Sphere sphere1(Vec3(0, 0, 0), 1.0f);
    Sphere sphere2(Vec3(0, 0, 0), 2.0f);
    Ray ray(Vec3(-5, 0, 0), Vec3(1, 0, 0));

    auto result1 = ray_sphere_intersection(ray, sphere1);
    auto result2 = ray_sphere_intersection(ray, sphere2);

    REQUIRE(result1.has_value());
    REQUIRE(result2.has_value());

    // Larger sphere should have intersections that encompass smaller sphere
    REQUIRE(result2->t_near < result1->t_near);
    REQUIRE(result2->t_far > result1->t_far);
  }

  SECTION("Intersection with sphere at extreme coordinates") {
    Sphere sphere(Vec3(1000, 1000, 1000), 10.0f);
    Ray ray(Vec3(990, 1000, 1000), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near == Catch::Approx(0.0f));
    REQUIRE(result->t_far == Catch::Approx(20.0f));
  }

  SECTION("Numerical precision test with very close miss") {
    Sphere sphere(Vec3(0, 0, 0), 1.0f);
    Ray ray(Vec3(-3, 0.99999f, 0), Vec3(1, 0, 0));

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value()); // Should still hit due to numerical precision
    REQUIRE(result->hit == true);
  }

  SECTION("Ray intersection with negative radius sphere") {
    Sphere sphere(Vec3(0, 0, 0), -1.0f); // Invalid sphere
    Ray ray(Vec3(-2, 0, 0), Vec3(1, 0, 0));

    // Implementation should handle gracefully (this tests robustness)
    auto result = ray_sphere_intersection(ray, sphere);
    // Behavior with negative radius is implementation-defined
    // The test mainly ensures no crash occurs
  }

  SECTION("Performance test with distant sphere") {
    Sphere sphere(Vec3(1000.0f, 1000.0f, 1000.0f), 100.0f);
    Ray ray(Vec3(0, 0, 0), Vec3(1, 1, 1).get_normalized());

    auto result = ray_sphere_intersection(ray, sphere);
    REQUIRE(result.has_value());
    REQUIRE(result->hit == true);
    REQUIRE(result->t_near > 0);
    REQUIRE(result->t_far > result->t_near);
  }
}