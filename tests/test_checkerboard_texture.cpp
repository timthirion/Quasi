#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <quasi/materials/checkerboard_texture.hpp>

using namespace Q::materials;
using namespace Q::radiometry;

TEST_CASE("CheckerboardTexture Tests", "[materials][texture]") {

  SECTION("Basic checkerboard pattern") {
    Color white(1.0f, 1.0f, 1.0f);
    Color black(0.0f, 0.0f, 0.0f);
    CheckerboardTexture texture(white, black, 2, 2);

    // Test corners
    REQUIRE(texture.sample(0.0f, 0.0f) == white); // Bottom-left: even + even = even
    REQUIRE(texture.sample(1.0f, 0.0f) ==
            white); // Bottom-right: odd + even = odd -> black, but clamped
    REQUIRE(texture.sample(0.0f, 1.0f) ==
            white); // Top-left: even + odd = odd -> black, but clamped
    REQUIRE(texture.sample(1.0f, 1.0f) == white); // Top-right: odd + odd = even

    // Test center points of each quadrant
    REQUIRE(texture.sample(0.25f, 0.25f) == white); // Bottom-left quadrant
    REQUIRE(texture.sample(0.75f, 0.25f) == black); // Bottom-right quadrant
    REQUIRE(texture.sample(0.25f, 0.75f) == black); // Top-left quadrant
    REQUIRE(texture.sample(0.75f, 0.75f) == white); // Top-right quadrant
  }

  SECTION("Single checker (1x1)") {
    Color red(1.0f, 0.0f, 0.0f);
    Color blue(0.0f, 0.0f, 1.0f);
    CheckerboardTexture texture(red, blue, 1, 1);

    // Should always return the first color since (0+0) % 2 == 0
    REQUIRE(texture.sample(0.0f, 0.0f) == red);
    REQUIRE(texture.sample(0.5f, 0.5f) == red);
    REQUIRE(texture.sample(1.0f, 1.0f) == red);
  }

  SECTION("Asymmetric checker pattern (3x2)") {
    Color color1(0.5f, 0.5f, 0.5f);
    Color color2(0.8f, 0.2f, 0.6f);
    CheckerboardTexture texture(color1, color2, 3, 2);

    // Test specific positions
    REQUIRE(texture.sample(0.1f, 0.1f) == color1); // Checker (0,0): 0+0=0 (even)
    REQUIRE(texture.sample(0.4f, 0.1f) == color2); // Checker (1,0): 1+0=1 (odd)
    REQUIRE(texture.sample(0.7f, 0.1f) == color1); // Checker (2,0): 2+0=2 (even)
    REQUIRE(texture.sample(0.1f, 0.7f) == color2); // Checker (0,1): 0+1=1 (odd)
    REQUIRE(texture.sample(0.4f, 0.7f) == color1); // Checker (1,1): 1+1=2 (even)
    REQUIRE(texture.sample(0.7f, 0.7f) == color2); // Checker (2,1): 2+1=3 (odd)
  }

  SECTION("UV coordinate clamping") {
    Color white(1.0f, 1.0f, 1.0f);
    Color black(0.0f, 0.0f, 0.0f);
    CheckerboardTexture texture(white, black, 2, 2);

    // Test values outside [0,1] range
    REQUIRE(texture.sample(-0.5f, 0.5f) == white);  // Clamped to (0, 0.5)
    REQUIRE(texture.sample(1.5f, 0.5f) == black);   // Clamped to (1, 0.5)
    REQUIRE(texture.sample(0.5f, -0.5f) == white);  // Clamped to (0.5, 0)
    REQUIRE(texture.sample(0.5f, 1.5f) == black);   // Clamped to (0.5, 1)
    REQUIRE(texture.sample(-1.0f, -1.0f) == white); // Clamped to (0, 0)
    REQUIRE(texture.sample(2.0f, 2.0f) == white);   // Clamped to (1, 1)
  }

  SECTION("Edge case: exactly at checker boundaries") {
    Color red(1.0f, 0.0f, 0.0f);
    Color green(0.0f, 1.0f, 0.0f);
    CheckerboardTexture texture(red, green, 4, 4);

    // Test boundaries between checkers
    REQUIRE(texture.sample(0.25f, 0.25f) == red);  // Center of checker (0,0)
    REQUIRE(texture.sample(0.5f, 0.25f) == green); // Center of checker (1,0)
    REQUIRE(texture.sample(0.25f, 0.5f) == green); // Center of checker (0,1)
    REQUIRE(texture.sample(0.5f, 0.5f) == red);    // Center of checker (1,1)

    // Test exact boundaries (should be consistent)
    REQUIRE(texture.sample(0.25f, 0.0f) == red);  // Boundary
    REQUIRE(texture.sample(0.5f, 0.0f) == green); // Boundary
  }

  SECTION("High resolution checker pattern") {
    Color white(1.0f, 1.0f, 1.0f);
    Color black(0.0f, 0.0f, 0.0f);
    CheckerboardTexture texture(white, black, 8, 8);

    // Test a few specific positions in high-res pattern
    REQUIRE(texture.sample(0.0625f, 0.0625f) == white); // Center of checker (0,0)
    REQUIRE(texture.sample(0.1875f, 0.0625f) == black); // Center of checker (1,0)
    REQUIRE(texture.sample(0.0625f, 0.1875f) == black); // Center of checker (0,1)
    REQUIRE(texture.sample(0.1875f, 0.1875f) == white); // Center of checker (1,1)
  }

  SECTION("Color equality test") {
    Color c1(0.3f, 0.7f, 0.2f);
    Color c2(0.9f, 0.1f, 0.8f);
    CheckerboardTexture texture(c1, c2, 2, 2);

    Color sampled = texture.sample(0.25f, 0.25f);
    REQUIRE(sampled.r == Catch::Approx(c1.r));
    REQUIRE(sampled.g == Catch::Approx(c1.g));
    REQUIRE(sampled.b == Catch::Approx(c1.b));

    sampled = texture.sample(0.75f, 0.25f);
    REQUIRE(sampled.r == Catch::Approx(c2.r));
    REQUIRE(sampled.g == Catch::Approx(c2.g));
    REQUIRE(sampled.b == Catch::Approx(c2.b));
  }

  SECTION("Pattern consistency across texture") {
    Color color1(1.0f, 0.0f, 0.0f);
    Color color2(0.0f, 1.0f, 0.0f);
    CheckerboardTexture texture(color1, color2, 4, 4);

    // Verify the pattern is consistent
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        float u = (i + 0.5f) / 4.0f; // Center of checker
        float v = (j + 0.5f) / 4.0f;

        Color expected = ((i + j) % 2 == 0) ? color1 : color2;
        Color actual = texture.sample(u, v);

        REQUIRE(actual == expected);
      }
    }
  }
}