/// @file plugin_test.cpp
/// @brief Unit tests for the plugin module.

#include <quasi/plugin/plugin_interface.hpp>
#include <quasi/plugin/dynamic_library.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

using namespace Q::plugin;

// ============================================================================
// plugin_version tests
// ============================================================================

TEST_CASE("plugin_version equality", "[plugin][version]") {
    plugin_version v1{1, 2, 3};
    plugin_version v2{1, 2, 3};
    plugin_version v3{1, 2, 4};

    // Use explicit function call due to ADL issues with C structs
    REQUIRE((v1 == v2));
    REQUIRE_FALSE((v1 == v3));
}

TEST_CASE("plugin_version comparison", "[plugin][version]") {
    plugin_version v100{1, 0, 0};
    plugin_version v110{1, 1, 0};
    plugin_version v111{1, 1, 1};
    plugin_version v200{2, 0, 0};

    // Use explicit function call due to ADL issues with C structs
    REQUIRE((v100 < v110));
    REQUIRE((v110 < v111));
    REQUIRE((v111 < v200));
    REQUIRE_FALSE((v200 < v100));
}

// ============================================================================
// library_error tests
// ============================================================================

TEST_CASE("library_error to_string", "[plugin][error]") {
    REQUIRE(to_string(library_error::file_not_found) == "file not found");
    REQUIRE(to_string(library_error::load_failed) == "failed to load library");
    REQUIRE(to_string(library_error::symbol_not_found) == "symbol not found");
    REQUIRE(to_string(library_error::not_loaded) == "library not loaded");
}

// ============================================================================
// dynamic_library tests
// ============================================================================

TEST_CASE("dynamic_library default construction", "[plugin][dynamic_library]") {
    dynamic_library lib;

    REQUIRE_FALSE(lib.is_loaded());
    REQUIRE_FALSE(static_cast<bool>(lib));
    REQUIRE(lib.native_handle() == nullptr);
}

TEST_CASE("dynamic_library open nonexistent file", "[plugin][dynamic_library]") {
    auto result = dynamic_library::open("/nonexistent/library.dylib");

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == library_error::file_not_found);
}

TEST_CASE("dynamic_library move semantics", "[plugin][dynamic_library]") {
    dynamic_library lib1;
    dynamic_library lib2 = std::move(lib1);

    // Both should be in valid (though empty) state
    REQUIRE_FALSE(lib1.is_loaded());
    REQUIRE_FALSE(lib2.is_loaded());
}

TEST_CASE("dynamic_library get_symbol on unloaded library", "[plugin][dynamic_library]") {
    dynamic_library lib;

    auto result = lib.get_symbol<void(*)()>("some_function");

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == library_error::not_loaded);
}

// ============================================================================
// shared_library_extension tests
// ============================================================================

TEST_CASE("shared_library_extension returns correct value", "[plugin][platform]") {
    auto ext = shared_library_extension();

#if defined(__APPLE__)
    REQUIRE(ext == ".dylib");
#elif defined(__linux__)
    REQUIRE(ext == ".so");
#endif
}

TEST_CASE("ensure_library_extension adds extension", "[plugin][platform]") {
    SECTION("adds extension to bare name") {
        auto result = ensure_library_extension("libfoo");
#if defined(__APPLE__)
        REQUIRE(result == "libfoo.dylib");
#elif defined(__linux__)
        REQUIRE(result == "libfoo.so");
#endif
    }

    SECTION("preserves existing .dylib") {
        auto result = ensure_library_extension("libfoo.dylib");
        REQUIRE(result == "libfoo.dylib");
    }

    SECTION("preserves existing .so") {
        auto result = ensure_library_extension("libfoo.so");
        REQUIRE(result == "libfoo.so");
    }

    SECTION("preserves existing .dll") {
        auto result = ensure_library_extension("libfoo.dll");
        REQUIRE(result == "libfoo.dll");
    }
}

// ============================================================================
// plugin_interface constant tests
// ============================================================================

TEST_CASE("plugin interface constants", "[plugin][interface]") {
    REQUIRE(k_symbol_abi_version == std::string_view{"Q_plugin_abi_version"});
    REQUIRE(k_symbol_get_info == std::string_view{"Q_plugin_get_info"});
    REQUIRE(k_symbol_create == std::string_view{"Q_plugin_create"});
    REQUIRE(k_symbol_destroy == std::string_view{"Q_plugin_destroy"});
    REQUIRE(k_symbol_update == std::string_view{"Q_plugin_update"});
    REQUIRE(k_symbol_render == std::string_view{"Q_plugin_render"});

    // ABI version should be positive
    REQUIRE(k_plugin_abi_version > 0);
}
