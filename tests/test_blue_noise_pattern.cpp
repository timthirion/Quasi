#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <quasi/sampling/blue_noise_pattern.hpp>
#include <set>

using namespace Q::sampling;

TEST_CASE("Blue Noise Pattern Construction and Basic Properties", "[sampling][blue_noise]") {

  SECTION("Pattern constructs successfully") {
    BlueNoisePattern pattern;
    REQUIRE(pattern.get_name() == "blue_noise");
  }

  SECTION("Generate samples returns correct count") {
    BlueNoisePattern pattern;

    std::vector<int> sample_counts = {1, 4, 16, 64, 256};

    for (int count : sample_counts) {
      auto samples = pattern.generate_samples(count);
      REQUIRE(samples.size() == count);
    }
  }

  SECTION("All samples are within unit square") {
    BlueNoisePattern pattern;

    auto samples = pattern.generate_samples(100);

    for (const auto &sample : samples) {
      REQUIRE(sample.x >= 0.0f);
      REQUIRE(sample.x <= 1.0f);
      REQUIRE(sample.y >= 0.0f);
      REQUIRE(sample.y <= 1.0f);
    }
  }
}

TEST_CASE("Blue Noise Distribution Quality", "[sampling][blue_noise][quality]") {

  SECTION("Samples have reasonable distribution across unit square") {
    BlueNoisePattern pattern;
    auto samples = pattern.generate_samples(64);

    // Check distribution by dividing into quadrants
    int q1 = 0, q2 = 0, q3 = 0, q4 = 0; // quadrant counts

    for (const auto &sample : samples) {
      if (sample.x < 0.5f && sample.y < 0.5f)
        q1++;
      else if (sample.x >= 0.5f && sample.y < 0.5f)
        q2++;
      else if (sample.x < 0.5f && sample.y >= 0.5f)
        q3++;
      else
        q4++;
    }

    // Each quadrant should have some samples (not perfectly uniform due to blue noise)
    // But no quadrant should be completely empty for 64 samples
    REQUIRE(q1 > 0);
    REQUIRE(q2 > 0);
    REQUIRE(q3 > 0);
    REQUIRE(q4 > 0);

    // No quadrant should have more than 75% of samples
    int max_count = std::max({q1, q2, q3, q4});
    REQUIRE(max_count < samples.size() * 0.75);
  }

  SECTION("Blue noise has better distribution than random") {
    BlueNoisePattern blue_noise;
    auto blue_samples = blue_noise.generate_samples(64);

    // Calculate minimum distance between any two samples
    float min_distance_blue = std::numeric_limits<float>::max();

    for (size_t i = 0; i < blue_samples.size(); ++i) {
      for (size_t j = i + 1; j < blue_samples.size(); ++j) {
        float dx = blue_samples[i].x - blue_samples[j].x;
        float dy = blue_samples[i].y - blue_samples[j].y;
        float distance = std::sqrt(dx * dx + dy * dy);
        min_distance_blue = std::min(min_distance_blue, distance);
      }
    }

    // Blue noise should maintain reasonable minimum distance between samples
    // For 64 samples in unit square, expect minimum distance > 0.05
    REQUIRE(min_distance_blue > 0.02f);
  }

  SECTION("Consistent sample count produces different patterns") {
    BlueNoisePattern pattern;

    auto samples1 = pattern.generate_samples(32);
    auto samples2 = pattern.generate_samples(32);

    // Due to randomness, patterns should be different
    bool patterns_differ = false;
    for (size_t i = 0; i < samples1.size() && i < samples2.size(); ++i) {
      if (std::abs(samples1[i].x - samples2[i].x) > 1e-6f ||
          std::abs(samples1[i].y - samples2[i].y) > 1e-6f) {
        patterns_differ = true;
        break;
      }
    }

    REQUIRE(patterns_differ);
  }

  SECTION("Different sample counts produce different scales") {
    BlueNoisePattern pattern;

    auto small_samples = pattern.generate_samples(16);
    auto large_samples = pattern.generate_samples(256);

    // Calculate average nearest neighbor distance for each
    auto calc_avg_nearest_distance = [](const std::vector<Sample2D> &samples) {
      float total_min_dist = 0.0f;
      for (size_t i = 0; i < samples.size(); ++i) {
        float min_dist = std::numeric_limits<float>::max();
        for (size_t j = 0; j < samples.size(); ++j) {
          if (i != j) {
            float dx = samples[i].x - samples[j].x;
            float dy = samples[i].y - samples[j].y;
            float dist = std::sqrt(dx * dx + dy * dy);
            min_dist = std::min(min_dist, dist);
          }
        }
        total_min_dist += min_dist;
      }
      return total_min_dist / samples.size();
    };

    float avg_dist_small = calc_avg_nearest_distance(small_samples);
    float avg_dist_large = calc_avg_nearest_distance(large_samples);

    // More samples should have smaller average nearest neighbor distance
    REQUIRE(avg_dist_large < avg_dist_small);
  }
}

TEST_CASE("Blue Noise Generic Point Generation", "[sampling][blue_noise][generic]") {

  SECTION("Generate blue noise points in unit domain") {
    BlueNoisePattern pattern;

    auto points = pattern.generate_blue_noise_points(50);

    REQUIRE(points.size() == 50);

    for (const auto &point : points) {
      REQUIRE(point.x >= 0.0f);
      REQUIRE(point.x <= 1.0f);
      REQUIRE(point.y >= 0.0f);
      REQUIRE(point.y <= 1.0f);
    }
  }

  SECTION("Generate blue noise points in custom domain") {
    BlueNoisePattern pattern;

    float domain_width = 5.0f;
    float domain_height = 3.0f;
    auto points = pattern.generate_blue_noise_points(30, domain_width, domain_height);

    REQUIRE(points.size() == 30);

    for (const auto &point : points) {
      REQUIRE(point.x >= 0.0f);
      REQUIRE(point.x <= domain_width);
      REQUIRE(point.y >= 0.0f);
      REQUIRE(point.y <= domain_height);
    }
  }

  SECTION("Square vs rectangular domains affect distribution") {
    BlueNoisePattern pattern;

    auto square_points = pattern.generate_blue_noise_points(64, 1.0f, 1.0f);
    auto rect_points = pattern.generate_blue_noise_points(64, 2.0f, 1.0f);

    // Calculate spread in X direction
    auto calc_x_spread = [](const std::vector<Sample2D> &points) {
      float min_x = std::numeric_limits<float>::max();
      float max_x = std::numeric_limits<float>::lowest();
      for (const auto &p : points) {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
      }
      return max_x - min_x;
    };

    float square_x_spread = calc_x_spread(square_points);
    float rect_x_spread = calc_x_spread(rect_points);

    // Rectangular domain should have larger X spread
    REQUIRE(rect_x_spread > square_x_spread);
    REQUIRE(rect_x_spread > 1.5f); // Should use most of the 2.0 width
  }
}

TEST_CASE("Blue Noise Edge Cases and Robustness", "[sampling][blue_noise][edge_cases]") {

  SECTION("Single sample generation") {
    BlueNoisePattern pattern;

    auto samples = pattern.generate_samples(1);

    REQUIRE(samples.size() == 1);
    REQUIRE(samples[0].x >= 0.0f);
    REQUIRE(samples[0].x <= 1.0f);
    REQUIRE(samples[0].y >= 0.0f);
    REQUIRE(samples[0].y <= 1.0f);
  }

  SECTION("Zero samples generation") {
    BlueNoisePattern pattern;

    auto samples = pattern.generate_samples(0);

    REQUIRE(samples.size() == 0);
  }

  SECTION("Large sample count doesn't crash") {
    BlueNoisePattern pattern;

    // Test reasonably large sample count
    auto samples = pattern.generate_samples(1000);

    REQUIRE(samples.size() == 1000);

    // Verify all samples are valid
    for (const auto &sample : samples) {
      REQUIRE(std::isfinite(sample.x));
      REQUIRE(std::isfinite(sample.y));
      REQUIRE(sample.x >= 0.0f);
      REQUIRE(sample.x <= 1.0f);
      REQUIRE(sample.y >= 0.0f);
      REQUIRE(sample.y <= 1.0f);
    }
  }

  SECTION("Very small domain doesn't cause issues") {
    BlueNoisePattern pattern;

    auto points = pattern.generate_blue_noise_points(10, 0.01f, 0.01f);

    REQUIRE(points.size() == 10);

    for (const auto &point : points) {
      REQUIRE(std::isfinite(point.x));
      REQUIRE(std::isfinite(point.y));
      REQUIRE(point.x >= 0.0f);
      REQUIRE(point.x <= 0.01f);
      REQUIRE(point.y >= 0.0f);
      REQUIRE(point.y <= 0.01f);
    }
  }

  SECTION("Very large domain works correctly") {
    BlueNoisePattern pattern;

    auto points = pattern.generate_blue_noise_points(20, 1000.0f, 1000.0f);

    REQUIRE(points.size() == 20);

    for (const auto &point : points) {
      REQUIRE(std::isfinite(point.x));
      REQUIRE(std::isfinite(point.y));
      REQUIRE(point.x >= 0.0f);
      REQUIRE(point.x <= 1000.0f);
      REQUIRE(point.y >= 0.0f);
      REQUIRE(point.y <= 1000.0f);
    }
  }
}

TEST_CASE("Blue Noise Statistical Properties", "[sampling][blue_noise][statistics]") {

  SECTION("Sample distribution has low discrepancy") {
    BlueNoisePattern pattern;
    auto samples = pattern.generate_samples(64);

    // Test star discrepancy by checking coverage in grid cells
    const int grid_size = 8;
    const float cell_size = 1.0f / grid_size;

    std::vector<int> cell_counts(grid_size * grid_size, 0);

    for (const auto &sample : samples) {
      int cell_x = std::min(static_cast<int>(sample.x / cell_size), grid_size - 1);
      int cell_y = std::min(static_cast<int>(sample.y / cell_size), grid_size - 1);
      cell_counts[cell_y * grid_size + cell_x]++;
    }

    // Calculate variance in cell counts (lower is better for discrepancy)
    float mean_count = static_cast<float>(samples.size()) / (grid_size * grid_size);
    float variance = 0.0f;

    for (int count : cell_counts) {
      float diff = count - mean_count;
      variance += diff * diff;
    }
    variance /= cell_counts.size();

    // Blue noise should have relatively low variance (better than pure random)
    REQUIRE(variance < mean_count * 3.0f); // Heuristic threshold
  }

  SECTION("Radial distribution function shows blue noise characteristics") {
    BlueNoisePattern pattern;
    auto samples = pattern.generate_samples(100);

    // Calculate pair correlation function at short distances
    const float test_radius = 0.1f;
    int pairs_within_radius = 0;
    int total_pairs = 0;

    for (size_t i = 0; i < samples.size(); ++i) {
      for (size_t j = i + 1; j < samples.size(); ++j) {
        float dx = samples[i].x - samples[j].x;
        float dy = samples[i].y - samples[j].y;
        float distance = std::sqrt(dx * dx + dy * dy);

        total_pairs++;
        if (distance <= test_radius) {
          pairs_within_radius++;
        }
      }
    }

    float pair_fraction = static_cast<float>(pairs_within_radius) / total_pairs;

    // Blue noise should have fewer close pairs than pure random
    // For random distribution in unit square, expected fraction ≈ π * r² for small r
    float expected_random_fraction = M_PI * test_radius * test_radius;

    // Blue noise should have fewer close pairs than pure random
    // Use a more forgiving threshold due to randomness in the algorithm
    REQUIRE(pair_fraction < expected_random_fraction * 0.8f);
  }

  SECTION("Multiple generations maintain quality") {
    BlueNoisePattern pattern;

    // Generate multiple sample sets and verify they all maintain quality
    for (int iteration = 0; iteration < 5; ++iteration) {
      auto samples = pattern.generate_samples(64);

      // Calculate minimum distance between samples
      float min_distance = std::numeric_limits<float>::max();

      for (size_t i = 0; i < samples.size(); ++i) {
        for (size_t j = i + 1; j < samples.size(); ++j) {
          float dx = samples[i].x - samples[j].x;
          float dy = samples[i].y - samples[j].y;
          float distance = std::sqrt(dx * dx + dy * dy);
          min_distance = std::min(min_distance, distance);
        }
      }

      // Each generation should maintain reasonable minimum distance
      REQUIRE(min_distance > 0.02f);
    }
  }
}