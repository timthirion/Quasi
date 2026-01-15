/// @file main.cpp
/// @brief Host application entry point.
///
/// Creates a window, sets up Metal, loads a plugin, and runs the main loop.

#include <quasi/host/window.hpp>
#include <quasi/gpu/metal/context.hpp>
#include <quasi/plugin/plugin.hpp>

#include "tools/cpp/runfiles/runfiles.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>

namespace {

/// @brief Log callback for plugins.
void plugin_log(void* /*host_data*/, const char* message) {
    std::printf("[Plugin] %s\n", message);
}

/// @brief Shutdown callback for plugins.
void plugin_request_shutdown(void* host_data) {
    auto* window = static_cast<Q::host::window*>(host_data);
    if (window) {
        window->close();
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    using bazel::tools::cpp::runfiles::Runfiles;

    // Parse command line for plugin path
    std::filesystem::path plugin_path;
    std::unique_ptr<Runfiles> runfiles;

    if (argc > 1) {
        plugin_path = argv[1];
    } else {
        // Use default plugin from runfiles
        std::string error;
        runfiles.reset(Runfiles::Create(argv[0], &error));
        if (!runfiles) {
            std::fprintf(stderr, "Failed to load runfiles: %s\n", error.c_str());
            std::fprintf(stderr, "Usage: %s <plugin.dylib>\n", argv[0]);
            return EXIT_FAILURE;
        }
        plugin_path = runfiles->Rlocation("quasi/backends/metal/libquasi_metal.dylib");
        if (plugin_path.empty() || !std::filesystem::exists(plugin_path)) {
            std::fprintf(stderr, "Default plugin not found in runfiles\n");
            std::fprintf(stderr, "Usage: %s <plugin.dylib>\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Create window (square for Cornell Box)
    auto window_result = Q::host::window::create("Quasi", 720, 720);
    if (!window_result) {
        std::fprintf(stderr, "Failed to create window: %s\n",
                     Q::host::to_string(window_result.error()));
        return EXIT_FAILURE;
    }
    auto& window = *window_result;

    // Create Metal context
    auto metal_result = Q::gpu::metal::context::create(window.native_handle());
    if (!metal_result) {
        std::fprintf(stderr, "Failed to create Metal context: %s\n",
                     Q::gpu::metal::to_string(metal_result.error()));
        return EXIT_FAILURE;
    }
    auto& metal = *metal_result;

    // Handle window resize
    window.set_resize_callback([&metal](uint32_t width, uint32_t height) {
        metal.resize(width, height);
    });

    // Load the plugin
    auto lib_result = Q::plugin::dynamic_library::open(plugin_path);
    if (!lib_result) {
        std::fprintf(stderr, "Failed to load plugin library: %s\n",
                     Q::plugin::to_string(lib_result.error()).data());
        return EXIT_FAILURE;
    }

    // Set up plugin context
    Q::plugin::plugin_context ctx{
        .viewport_width  = window.framebuffer_width(),
        .viewport_height = window.framebuffer_height(),
        .host_data       = &window,
        .gpu             = metal.gpu(),
        .log             = plugin_log,
        .request_shutdown = plugin_request_shutdown,
    };

    auto plugin_result = Q::plugin::loader::load(*lib_result, &ctx);
    if (!plugin_result) {
        std::fprintf(stderr, "Failed to load plugin: %s\n",
                     to_string(plugin_result.error()));
        return EXIT_FAILURE;
    }
    auto& plugin = *plugin_result;

    // Print plugin info
    auto info = plugin.info();
    std::printf("Loaded plugin: %s v%u.%u.%u\n",
                info.name,
                info.version.major,
                info.version.minor,
                info.version.patch);
    std::printf("Description: %s\n", info.description);

    // Main loop
    auto last_time = std::chrono::steady_clock::now();

    while (!window.should_close()) {
        window.poll_events();

        // Calculate delta time
        auto now = std::chrono::steady_clock::now();
        float delta_time = std::chrono::duration<float>(now - last_time).count();
        last_time = now;

        // Update plugin
        plugin.update(delta_time);

        // Begin frame
        auto frame_result = metal.begin_frame();
        if (frame_result) {
            auto& frame = *frame_result;

            // Render plugin
            plugin.render(&frame);

            // Present
            metal.end_frame(frame);
        }
    }

    std::printf("Shutting down...\n");
    return EXIT_SUCCESS;
}
