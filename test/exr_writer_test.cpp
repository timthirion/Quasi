/// @file exr_writer_test.cpp
/// @brief Tests for the EXR writer module.

#include <quasi/io/exr_writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

TEST_CASE("write_exr rejects null data", "[io][exr]") {
    Q_readback_result result{.data = nullptr, .width = 0, .height = 0, .channels = 0};
    auto r = Q::io::write_exr("/tmp/quasi_test_null.exr", result);
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r.error() == Q::io::exr_error::invalid_data);
}

TEST_CASE("write_exr rejects zero dimensions", "[io][exr]") {
    float data[] = {1.0f, 0.0f, 0.0f, 1.0f};
    Q_readback_result result{.data = data, .width = 0, .height = 0, .channels = 4};
    auto r = Q::io::write_exr("/tmp/quasi_test_zero.exr", result);
    REQUIRE_FALSE(r.has_value());
    REQUIRE(r.error() == Q::io::exr_error::invalid_data);
}

TEST_CASE("write_exr writes valid file", "[io][exr]") {
    float data[] = {
        1.0f, 0.0f, 0.0f, 1.0f,  // Red
        0.0f, 1.0f, 0.0f, 1.0f,  // Green
        0.0f, 0.0f, 1.0f, 1.0f,  // Blue
        1.0f, 1.0f, 1.0f, 1.0f,  // White
    };
    Q_readback_result result{.data = data, .width = 2, .height = 2, .channels = 4};

    auto path = std::filesystem::temp_directory_path() / "quasi_test_write.exr";
    auto r = Q::io::write_exr(path, result);
    REQUIRE(r.has_value());
    REQUIRE(std::filesystem::exists(path));
    REQUIRE(std::filesystem::file_size(path) > 0);
    std::filesystem::remove(path);
}

TEST_CASE("make_timestamped_path produces .exr extension", "[io][exr]") {
    auto path = Q::io::make_timestamped_path("/tmp");
    REQUIRE(path.extension() == ".exr");
    REQUIRE(path.parent_path() == "/tmp");
}

TEST_CASE("make_timestamped_path starts with quasi_", "[io][exr]") {
    auto path = Q::io::make_timestamped_path("/tmp");
    auto filename = path.filename().string();
    REQUIRE(filename.starts_with("quasi_"));
}

TEST_CASE("to_string returns non-empty for all error codes", "[io][exr]") {
    REQUIRE(std::string_view{Q::io::to_string(Q::io::exr_error::invalid_data)}.size() > 0);
    REQUIRE(std::string_view{Q::io::to_string(Q::io::exr_error::write_failed)}.size() > 0);
    REQUIRE(std::string_view{Q::io::to_string(Q::io::exr_error::directory_error)}.size() > 0);
}
