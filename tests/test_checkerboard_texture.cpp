#include "../src/materials/CheckerboardTexture.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Q::materials;
using namespace Q::radiometry;

TEST_CASE("CheckerboardTexture Tests", "[materials][texture][checkerboard]") {

  SECTION("Basic checkerboard pattern with 2x2 grid") {
    Color white(1.0f, 1.0f, 1.0f);
    Color black(0.0f, 0.0f, 0.0f);
    CheckerboardTexture texture(white, black, 2, 2);

    // Test corners - should alternate
    Color top_left = texture.sample(0.0f, 0.0f);
    Color top_right = texture.sample(0.99f, 0.0f);
    Color bottom_left = texture.sample(0.0f, 0.99f);
    Color bottom_right = texture.sample(0.99f, 0.99f);

    REQUIRE(top_left.r == Catch::Approx(1.0f));     // white
    REQUIRE(top_right.r == Catch::Approx(0.0f));    // black
    REQUIRE(bottom_left.r == Catch::Approx(0.0f));  // black
    REQUIRE(bottom_right.r == Catch::Approx(1.0f)); // white
  }

  SECTION("Checkerboard pattern with different colors") {
    Color red(1.0f, 0.0f, 0.0f);
    Color blue(0.0f, 0.0f, 1.0f);
    CheckerboardTexture texture(red, blue, 2, 2);

    Color sample1 = texture.sample(0.25f, 0.25f); // First quadrant
    Color sample2 = texture.sample(0.75f, 0.25f); // Second quadrant

    REQUIRE(sample1.r == Catch::Approx(1.0f)); // red
    REQUIRE(sample1.b == Catch::Approx(0.0f));
    REQUIRE(sample2.r == Catch::Approx(0.0f)); // blue
    REQUIRE(sample2.b == Catch::Approx(1.0f));
  }

  SECTION("UV coordinates wrapping behavior") {
    Color white(1.0f, 1.0f, 1.0f);
    Color black(0.0f, 0.0f, 0.0f);
    CheckerboardTexture texture(white, black, 2, 2);

    // Test that UV coordinates wrap around
    Color sample1 = texture.sample(0.0f, 0.0f);
    Color sample2 = texture.sample(1.0f, 1.0f); // Should wrap to (0,0)
    Color sample3 = texture.sample(2.5f, 3.7f); // Should wrap to (0.5, 0.7)

    REQUIRE(sample1.r == sample2.r);
    REQUIRE(sample1.g == sample2.g);
    REQUIRE(sample1.b == sample2.b);

    Color reference = texture.sample(0.5f, 0.7f);
    REQUIRE(sample3.r == reference.r);
  }

  SECTION("Different grid dimensions") {
    Color color1(0.2f, 0.4f, 0.6f);
    Color color2(0.8f, 0.6f, 0.4f);
    CheckerboardTexture texture(color1, color2, 4, 8);

    // Test that we get different patterns with different grid sizes
    Color sample_center = texture.sample(0.5f, 0.5f);
    Color sample_quarter = texture.sample(0.25f, 0.25f);

    // These should be different colors due to the 4x8 grid
    bool colors_different = (sample_center.r != sample_quarter.r) ||
                            (sample_center.g != sample_quarter.g) ||
                            (sample_center.b != sample_quarter.b);
    REQUIRE(colors_different);
  }

  SECTION("Getter methods work correctly") {
    Color red(1.0f, 0.0f, 0.0f);
    Color green(0.0f, 1.0f, 0.0f);
    CheckerboardTexture texture(red, green, 3, 5);

    REQUIRE(texture.get_color1().r == Catch::Approx(1.0f));
    REQUIRE(texture.get_color2().g == Catch::Approx(1.0f));
    REQUIRE(texture.get_rows() == 3);
    REQUIRE(texture.get_columns() == 5);
  }

  SECTION("Edge cases with small grid") {
    Color white(1.0f, 1.0f, 1.0f);
    Color black(0.0f, 0.0f, 0.0f);
    CheckerboardTexture texture(white, black, 1, 1);

    // With 1x1 grid, everything should be the same color (color1)
    Color sample1 = texture.sample(0.0f, 0.0f);
    Color sample2 = texture.sample(0.5f, 0.5f);
    Color sample3 = texture.sample(0.99f, 0.99f);

    REQUIRE(sample1.r == sample2.r);
    REQUIRE(sample2.r == sample3.r);
    REQUIRE(sample1.r == Catch::Approx(1.0f)); // Should be color1 (white)
  }

  SECTION("Checkerboard pattern consistency") {
    Color white(1.0f, 1.0f, 1.0f);
    Color black(0.0f, 0.0f, 0.0f);
    CheckerboardTexture texture(white, black, 4, 4);

    // Test that adjacent squares have different colors
    Color center_square = texture.sample(0.375f, 0.375f); // Center of (1,1) square
    Color right_square = texture.sample(0.625f, 0.375f);  // Center of (2,1) square
    Color below_square = texture.sample(0.375f, 0.625f);  // Center of (1,2) square

    // Adjacent squares should have different colors
    bool right_different = (center_square.r != right_square.r);
    bool below_different = (center_square.r != below_square.r);

    REQUIRE(right_different);
    REQUIRE(below_different);
  }

  SECTION("Boundary conditions") {
    Color red(1.0f, 0.0f, 0.0f);
    Color blue(0.0f, 0.0f, 1.0f);
    CheckerboardTexture texture(red, blue, 3, 3);

    // Test exactly at boundaries
    Color at_zero = texture.sample(0.0f, 0.0f);
    Color at_third = texture.sample(1.0f / 3.0f, 1.0f / 3.0f);
    Color at_two_thirds = texture.sample(2.0f / 3.0f, 2.0f / 3.0f);

    // These should alternate in checkerboard pattern
    REQUIRE(at_zero.r == Catch::Approx(1.0f));       // red (0,0) -> sum=0 (even)
    REQUIRE(at_third.r == Catch::Approx(1.0f));      // red (1,1) -> sum=2 (even)
    REQUIRE(at_two_thirds.r == Catch::Approx(1.0f)); // red (2,2) -> sum=4 (even)
  }

  SECTION("Negative UV coordinates") {
    Color white(1.0f, 1.0f, 1.0f);
    Color black(0.0f, 0.0f, 0.0f);
    CheckerboardTexture texture(white, black, 2, 2);

    // Negative coordinates should wrap properly
    Color positive = texture.sample(0.25f, 0.25f);
    Color negative = texture.sample(-0.75f, -0.75f); // Should wrap to (0.25, 0.25)

    REQUIRE(positive.r == negative.r);
    REQUIRE(positive.g == negative.g);
    REQUIRE(positive.b == negative.b);
  }

  SECTION("Non-square checkerboards") {
    Color yellow(1.0f, 1.0f, 0.0f);
    Color purple(1.0f, 0.0f, 1.0f);
    CheckerboardTexture texture(yellow, purple, 2, 6); // 2 rows, 6 columns

    // Sample different parts of the non-square grid
    Color sample1 = texture.sample(0.08f, 0.25f); // First row, first column (0,0)
    Color sample2 = texture.sample(0.25f, 0.25f); // First row, second column (1,0)
    Color sample3 = texture.sample(0.08f, 0.75f); // Second row, first column (0,1)

    // Should alternate properly (compare green component since yellow has g=1, purple has g=0)
    bool row_alternates = (sample1.g != sample2.g);
    bool col_alternates = (sample1.g != sample3.g);

    REQUIRE(row_alternates);
    REQUIRE(col_alternates);
  }
}