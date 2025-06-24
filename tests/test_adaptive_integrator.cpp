#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <quasi/radiometry/color.hpp>
#include <quasi/sampling/adaptive_integrator.hpp>
#include <quasi/sampling/stratified_pattern.hpp>

using namespace Q::sampling;
using namespace Q::radiometry;

TEST_CASE("Adaptive Integrator Construction", "[sampling][adaptive][construction]") {

  SECTION("Basic construction with default parameters") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern));

    REQUIRE(integrator.get_name() == "adaptive");
    REQUIRE(integrator.get_base_samples() == 4);
    REQUIRE(integrator.get_max_samples() == 64);
    REQUIRE(integrator.get_variance_threshold() == Catch::Approx(0.01f));
  }

  SECTION("Construction with custom parameters") {
    auto pattern = std::make_unique<StratifiedPattern>();

    int base_samples = 8;
    int max_samples = 128;
    float threshold = 0.005f;
    int levels = 5;

    AdaptiveIntegrator integrator(std::move(pattern), base_samples, max_samples, threshold, levels);

    REQUIRE(integrator.get_base_samples() == base_samples);
    REQUIRE(integrator.get_max_samples() == max_samples);
    REQUIRE(integrator.get_variance_threshold() == Catch::Approx(threshold));
  }

  SECTION("Edge case parameters") {
    auto pattern = std::make_unique<StratifiedPattern>();

    // Minimum values
    AdaptiveIntegrator min_integrator(std::move(pattern), 1, 2, 0.001f, 1);
    REQUIRE(min_integrator.get_base_samples() == 1);
    REQUIRE(min_integrator.get_max_samples() == 2);

    // Very high variance threshold (should require very little adaptation)
    auto pattern2 = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator high_threshold(std::move(pattern2), 4, 64, 1.0f, 3);
    REQUIRE(high_threshold.get_variance_threshold() == Catch::Approx(1.0f));
  }
}

TEST_CASE("Adaptive Integrator Basic Sample Integration", "[sampling][adaptive][integration]") {

  SECTION("Integrate uniform colors gives average") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern));

    // Create uniform samples and colors
    std::vector<Sample2D> samples = {Sample2D{0.25f, 0.25f}, Sample2D{0.75f, 0.25f},
                                     Sample2D{0.25f, 0.75f}, Sample2D{0.75f, 0.75f}};

    Color uniform_color(0.5f, 0.7f, 0.3f);
    std::vector<Color> colors = {uniform_color, uniform_color, uniform_color, uniform_color};

    Color result = integrator.integrate_samples(samples, colors);

    REQUIRE(result.r == Catch::Approx(uniform_color.r));
    REQUIRE(result.g == Catch::Approx(uniform_color.g));
    REQUIRE(result.b == Catch::Approx(uniform_color.b));
  }

  SECTION("Integrate mixed colors gives correct average") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern));

    std::vector<Sample2D> samples = {Sample2D{0.0f, 0.0f}, Sample2D{1.0f, 1.0f}};

    std::vector<Color> colors = {
        Color(1.0f, 0.0f, 0.0f), // Pure red
        Color(0.0f, 0.0f, 1.0f)  // Pure blue
    };

    Color result = integrator.integrate_samples(samples, colors);

    // Should be average: (1+0)/2, (0+0)/2, (0+1)/2
    REQUIRE(result.r == Catch::Approx(0.5f));
    REQUIRE(result.g == Catch::Approx(0.0f));
    REQUIRE(result.b == Catch::Approx(0.5f));
  }

  SECTION("Empty sample lists handled gracefully") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern));

    std::vector<Sample2D> empty_samples;
    std::vector<Color> empty_colors;

    Color result = integrator.integrate_samples(empty_samples, empty_colors);

    // Should return black for empty input
    REQUIRE(result.r == Catch::Approx(0.0f));
    REQUIRE(result.g == Catch::Approx(0.0f));
    REQUIRE(result.b == Catch::Approx(0.0f));
  }

  SECTION("Single sample integration") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern));

    std::vector<Sample2D> samples = {Sample2D{0.5f, 0.5f}};
    std::vector<Color> colors = {Color(0.8f, 0.4f, 0.2f)};

    Color result = integrator.integrate_samples(samples, colors);

    REQUIRE(result.r == Catch::Approx(0.8f));
    REQUIRE(result.g == Catch::Approx(0.4f));
    REQUIRE(result.b == Catch::Approx(0.2f));
  }
}

TEST_CASE("Adaptive Integrator Adaptive Sampling", "[sampling][adaptive][adaptive_sampling]") {

  SECTION("Low variance signal uses base samples only") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern), 4, 64, 0.01f, 3);

    int total_samples_used = 0;

    // Ray tracer that returns consistent color (low variance)
    auto consistent_ray_tracer = [&](const Sample2D &sample) -> Color {
      total_samples_used++;
      return Color(0.5f, 0.5f, 0.5f); // Always same color
    };

    Color result = integrator.integrate_adaptive(10, 10, consistent_ray_tracer);

    // Should use approximately base samples (4) due to low variance
    REQUIRE(total_samples_used >= 4);
    REQUIRE(total_samples_used <= 16); // Shouldn't need many more

    // Result should be close to the consistent color
    REQUIRE(result.r == Catch::Approx(0.5f).margin(0.01f));
    REQUIRE(result.g == Catch::Approx(0.5f).margin(0.01f));
    REQUIRE(result.b == Catch::Approx(0.5f).margin(0.01f));
  }

  SECTION("High variance signal triggers more sampling") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern), 4, 64, 0.01f, 3);

    int total_samples_used = 0;

    // Ray tracer that alternates between very different colors (high variance)
    auto high_variance_ray_tracer = [&](const Sample2D &sample) -> Color {
      total_samples_used++;
      if (total_samples_used % 2 == 0) {
        return Color(1.0f, 1.0f, 1.0f); // White
      } else {
        return Color(0.0f, 0.0f, 0.0f); // Black
      }
    };

    Color result = integrator.integrate_adaptive(5, 5, high_variance_ray_tracer);

    // Should use more than base samples due to high variance
    REQUIRE(total_samples_used > 4);

    // Result should be approximately gray (average of black and white)
    REQUIRE(result.r == Catch::Approx(0.5f).margin(0.2f));
    REQUIRE(result.g == Catch::Approx(0.5f).margin(0.2f));
    REQUIRE(result.b == Catch::Approx(0.5f).margin(0.2f));
  }

  SECTION("Respects maximum sample limit") {
    auto pattern = std::make_unique<StratifiedPattern>();
    int max_samples = 16;
    AdaptiveIntegrator integrator(std::move(pattern), 4, max_samples, 0.001f, 3);

    int total_samples_used = 0;

    // Ray tracer with extreme variance that should trigger max sampling
    auto extreme_variance_ray_tracer = [&](const Sample2D &sample) -> Color {
      total_samples_used++;
      // Return very different colors to trigger maximum adaptation
      float intensity = (total_samples_used % 3 == 0) ? 1.0f : 0.0f;
      return Color(intensity, intensity * 0.5f, intensity * 0.2f);
    };

    Color result = integrator.integrate_adaptive(0, 0, extreme_variance_ray_tracer);

    // Should not exceed maximum sample count
    REQUIRE(total_samples_used <= max_samples);
    REQUIRE(total_samples_used >= 4); // Should at least use base samples

    // Result should be well-defined (not NaN or infinite)
    REQUIRE(std::isfinite(result.r));
    REQUIRE(std::isfinite(result.g));
    REQUIRE(std::isfinite(result.b));
  }

  SECTION("Different pixel positions produce independent results") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern), 4, 32, 0.01f, 2);

    int samples_pixel1 = 0;
    int samples_pixel2 = 0;

    auto counting_ray_tracer1 = [&](const Sample2D &sample) -> Color {
      samples_pixel1++;
      return Color(0.3f, 0.6f, 0.9f);
    };

    auto counting_ray_tracer2 = [&](const Sample2D &sample) -> Color {
      samples_pixel2++;
      return Color(0.9f, 0.3f, 0.6f);
    };

    Color result1 = integrator.integrate_adaptive(10, 20, counting_ray_tracer1);
    Color result2 = integrator.integrate_adaptive(30, 40, counting_ray_tracer2);

    // Each should use their own samples
    REQUIRE(samples_pixel1 >= 4);
    REQUIRE(samples_pixel2 >= 4);

    // Results should be different (different input colors)
    REQUIRE(std::abs(result1.r - result2.r) > 0.1f);
    REQUIRE(std::abs(result1.g - result2.g) > 0.1f);
    REQUIRE(std::abs(result1.b - result2.b) > 0.1f);
  }
}

TEST_CASE("Adaptive Integrator Configuration Effects", "[sampling][adaptive][configuration]") {

  SECTION("Higher variance threshold reduces adaptation") {
    auto pattern1 = std::make_unique<StratifiedPattern>();
    auto pattern2 = std::make_unique<StratifiedPattern>();

    // Low threshold should trigger more adaptation
    AdaptiveIntegrator sensitive(std::move(pattern1), 4, 32, 0.001f, 3);
    // High threshold should trigger less adaptation
    AdaptiveIntegrator tolerant(std::move(pattern2), 4, 32, 0.1f, 3);

    int samples_sensitive = 0;
    int samples_tolerant = 0;

    // Moderately varying ray tracer
    auto moderate_variance_tracer1 = [&](const Sample2D &sample) -> Color {
      samples_sensitive++;
      float variation = (samples_sensitive % 3) * 0.1f;
      return Color(0.5f + variation, 0.5f, 0.5f - variation);
    };

    auto moderate_variance_tracer2 = [&](const Sample2D &sample) -> Color {
      samples_tolerant++;
      float variation = (samples_tolerant % 3) * 0.1f;
      return Color(0.5f + variation, 0.5f, 0.5f - variation);
    };

    Color result1 = sensitive.integrate_adaptive(0, 0, moderate_variance_tracer1);
    Color result2 = tolerant.integrate_adaptive(0, 0, moderate_variance_tracer2);

    // Sensitive integrator should use more samples
    REQUIRE(samples_sensitive >= samples_tolerant);
  }

  SECTION("Different base samples affect minimum sampling") {
    auto pattern1 = std::make_unique<StratifiedPattern>();
    auto pattern2 = std::make_unique<StratifiedPattern>();

    AdaptiveIntegrator low_base(std::move(pattern1), 2, 32, 0.01f, 3);
    AdaptiveIntegrator high_base(std::move(pattern2), 8, 32, 0.01f, 3);

    int samples_low = 0;
    int samples_high = 0;

    // Ray tracer with low variance (shouldn't trigger much adaptation)
    auto low_variance_tracer1 = [&](const Sample2D &sample) -> Color {
      samples_low++;
      return Color(0.4f, 0.4f, 0.4f);
    };

    auto low_variance_tracer2 = [&](const Sample2D &sample) -> Color {
      samples_high++;
      return Color(0.4f, 0.4f, 0.4f);
    };

    Color result1 = low_base.integrate_adaptive(0, 0, low_variance_tracer1);
    Color result2 = high_base.integrate_adaptive(0, 0, low_variance_tracer2);

    // High base should use more samples even with low variance
    REQUIRE(samples_high >= samples_low);
    REQUIRE(samples_low >= 2);  // At least base samples
    REQUIRE(samples_high >= 8); // At least base samples
  }

  SECTION("Zero adaptation levels limits sampling") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator no_adaptation(std::move(pattern), 4, 64, 0.001f, 0);

    int total_samples = 0;

    // High variance ray tracer that would normally trigger adaptation
    auto high_variance_tracer = [&](const Sample2D &sample) -> Color {
      total_samples++;
      return (total_samples % 2 == 0) ? Color(1.0f, 1.0f, 1.0f) : Color(0.0f, 0.0f, 0.0f);
    };

    Color result = no_adaptation.integrate_adaptive(0, 0, high_variance_tracer);

    // Should use exactly base samples (no adaptation allowed)
    REQUIRE(total_samples == 4);
  }
}

TEST_CASE("Adaptive Integrator Edge Cases", "[sampling][adaptive][edge_cases]") {

  SECTION("Ray tracer that returns NaN is handled") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern), 4, 16, 0.01f, 2);

    int sample_count = 0;

    auto nan_ray_tracer = [&](const Sample2D &sample) -> Color {
      sample_count++;
      if (sample_count == 1) {
        return Color(std::numeric_limits<float>::quiet_NaN(), 0.5f, 0.5f);
      }
      return Color(0.5f, 0.5f, 0.5f);
    };

    Color result = integrator.integrate_adaptive(0, 0, nan_ray_tracer);

    // Should handle NaN gracefully and not crash
    REQUIRE(sample_count >= 4);
  }

  SECTION("Ray tracer that returns infinite values") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern), 4, 16, 0.01f, 2);

    int sample_count = 0;

    auto infinite_ray_tracer = [&](const Sample2D &sample) -> Color {
      sample_count++;
      if (sample_count <= 2) {
        return Color(std::numeric_limits<float>::infinity(), 0.0f, 0.0f);
      }
      return Color(0.2f, 0.2f, 0.2f);
    };

    Color result = integrator.integrate_adaptive(0, 0, infinite_ray_tracer);

    // Should complete without crashing
    REQUIRE(sample_count >= 4);
  }

  SECTION("Extremely high contrast doesn't cause overflow") {
    auto pattern = std::make_unique<StratifiedPattern>();
    AdaptiveIntegrator integrator(std::move(pattern), 4, 32, 0.01f, 3);

    int sample_count = 0;

    auto high_contrast_tracer = [&](const Sample2D &sample) -> Color {
      sample_count++;
      return (sample_count % 2 == 0) ? Color(1000.0f, 1000.0f, 1000.0f) : Color(0.0f, 0.0f, 0.0f);
    };

    Color result = integrator.integrate_adaptive(0, 0, high_contrast_tracer);

    // Should produce finite result
    REQUIRE(std::isfinite(result.r));
    REQUIRE(std::isfinite(result.g));
    REQUIRE(std::isfinite(result.b));

    // Result should be reasonable average
    REQUIRE(result.r > 0.0f);
    REQUIRE(result.r < 1000.0f);
  }
}