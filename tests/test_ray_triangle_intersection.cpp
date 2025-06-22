#include "../src/geometry.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace geometry;
using Catch::Approx;

TEST_CASE("Ray-Triangle Intersection Tests", "[geometry]") {

  SECTION("1. Basic intersection - ray hits triangle center") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.25f, 0.25f, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->hit);
    REQUIRE(result->t == Approx(1.0f));
    REQUIRE(result->point.x == Approx(0.25f));
    REQUIRE(result->point.y == Approx(0.25f));
    REQUIRE(result->point.z == Approx(0.0f));
  }

  SECTION("2. Ray misses triangle - to the side") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(2, 2, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("3. Ray hits triangle at vertex v0") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0, 0, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->barycentric.x == Approx(1.0f));
    REQUIRE(result->barycentric.y == Approx(0.0f));
    REQUIRE(result->barycentric.z == Approx(0.0f));
  }

  SECTION("4. Ray hits triangle at vertex v1") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(1, 0, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->barycentric.x == Approx(0.0f));
    REQUIRE(result->barycentric.y == Approx(1.0f));
    REQUIRE(result->barycentric.z == Approx(0.0f));
  }

  SECTION("5. Ray hits triangle at vertex v2") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0, 1, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->barycentric.x == Approx(0.0f));
    REQUIRE(result->barycentric.y == Approx(0.0f));
    REQUIRE(result->barycentric.z == Approx(1.0f));
  }

  SECTION("6. Ray hits triangle edge v0-v1") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.5f, 0, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->barycentric.x == Approx(0.5f));
    REQUIRE(result->barycentric.y == Approx(0.5f));
    REQUIRE(result->barycentric.z == Approx(0.0f));
  }

  SECTION("7. Ray hits triangle edge v1-v2") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.5f, 0.5f, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->barycentric.x == Approx(0.0f));
    REQUIRE(result->barycentric.y == Approx(0.5f));
    REQUIRE(result->barycentric.z == Approx(0.5f));
  }

  SECTION("8. Ray hits triangle edge v2-v0") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0, 0.5f, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->barycentric.x == Approx(0.5f));
    REQUIRE(result->barycentric.y == Approx(0.0f));
    REQUIRE(result->barycentric.z == Approx(0.5f));
  }

  SECTION("9. Ray parallel to triangle - no intersection") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.5f, 0.5f, 1), Vec3(1, 0, 0));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("10. Ray pointing away from triangle") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.5f, 0.5f, 1), Vec3(0, 0, 1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("11. Ray origin inside triangle plane") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.25f, 0.25f, 0), Vec3(0, 0, 1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("12. Ray hits interior at different angles") {
    Triangle tri(Vec3(0, 0, 0), Vec3(2, 0, 0), Vec3(0, 2, 0));
    Ray ray(Vec3(0.5f, 0.5f, 2), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->t == Approx(2.0f));
    REQUIRE(result->point.x == Approx(0.5f));
    REQUIRE(result->point.y == Approx(0.5f));
    REQUIRE(result->point.z == Approx(0.0f));
  }

  SECTION("13. Oblique ray intersection") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0, 0, 2), Vec3(0.25f, 0.25f, -1).normalize());

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->hit);
  }

  SECTION("14. Ray hits triangle from behind") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.25f, 0.25f, -1), Vec3(0, 0, 1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->t == Approx(1.0f));
  }

  SECTION("15. Large triangle intersection") {
    Triangle tri(Vec3(-10, -10, 0), Vec3(10, -10, 0), Vec3(0, 10, 0));
    Ray ray(Vec3(0, 0, 5), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->point.z == Approx(0.0f));
  }

  SECTION("16. Very small triangle") {
    Triangle tri(Vec3(0, 0, 0), Vec3(0.001f, 0, 0), Vec3(0, 0.001f, 0));
    Ray ray(Vec3(0.0002f, 0.0002f, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
  }

  SECTION("17. Degenerate triangle (collinear points)") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(2, 0, 0));
    Ray ray(Vec3(0.5f, 0, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("18. Ray exactly on triangle edge boundary") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(1, 0, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
  }

  SECTION("19. Near-miss intersection") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(1.001f, 0, 1), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE_FALSE(result.has_value());
  }

  SECTION("20. Triangle in different orientation") {
    Triangle tri(Vec3(0, 0, 1), Vec3(1, 0, 1), Vec3(0.5f, 1, 1));
    Ray ray(Vec3(0.5f, 0.5f, 0), Vec3(0, 0, 1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->point.z == Approx(1.0f));
  }

  SECTION("21. Negative ray direction components") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(1, 1, 1), Vec3(-0.5f, -0.5f, -1).normalize());

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
  }

  SECTION("22. Multiple potential intersections (closest)") {
    Triangle tri(Vec3(0, 0, 1), Vec3(1, 0, 1), Vec3(0, 1, 1));
    Ray ray(Vec3(0.25f, 0.25f, -2), Vec3(0, 0, 1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
    REQUIRE(result->t == Approx(3.0f));
  }

  SECTION("23. Ray with very small direction magnitude") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.5f, 0.5f, 1), Vec3(0, 0, -0.001f));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
  }

  SECTION("24. Intersection very close to triangle plane") {
    Triangle tri(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 1, 0));
    Ray ray(Vec3(0.25f, 0.25f, 1e-7f), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());
  }

  SECTION("25. Barycentric coordinate validation for interior point") {
    Triangle tri(Vec3(0, 0, 0), Vec3(3, 0, 0), Vec3(0, 3, 0));
    Ray ray(Vec3(1, 1, 2), Vec3(0, 0, -1));

    auto result = rayTriangleIntersection(ray, tri);
    REQUIRE(result.has_value());

    float u = result->barycentric.y;
    float v = result->barycentric.z;
    float w = result->barycentric.x;

    REQUIRE(u >= 0.0f);
    REQUIRE(v >= 0.0f);
    REQUIRE(w >= 0.0f);
    REQUIRE((u + v + w) == Approx(1.0f));

    Vec3 reconstructed = tri.v0 * w + tri.v1 * u + tri.v2 * v;
    REQUIRE(reconstructed.x == Approx(result->point.x));
    REQUIRE(reconstructed.y == Approx(result->point.y));
    REQUIRE(reconstructed.z == Approx(result->point.z));
  }
}