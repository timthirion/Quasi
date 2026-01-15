/// @file main.cpp
/// @brief Host application entry point.
///
/// Creates a window, sets up Metal, loads a plugin, and runs the main loop.

#include <quasi/host/window.hpp>
#include <quasi/gpu/metal/context.hpp>
#include <quasi/plugin/plugin.hpp>

#include "tools/cpp/runfiles/runfiles.h"

#include <chrono>
#include <cmath>
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

/// @brief Simple orbit camera controller.
struct orbit_camera {
    float target[3] = {0.0f, 1.0f, 0.0f};  // Look-at target (center of Cornell Box).
    float distance  = 3.5f;                 // Distance from target.
    float azimuth   = 0.0f;                 // Horizontal angle (radians).
    float elevation = 0.0f;                 // Vertical angle (radians).
    float fov       = 40.0f;                // Field of view in degrees.

    // Computed position.
    float position[3] = {0.0f, 1.0f, 3.5f};

    // Input state.
    bool dragging = false;
    double last_x = 0.0;
    double last_y = 0.0;
    bool dirty = true;  // True if camera changed this frame.

    void update_position() {
        // Clamp elevation to avoid gimbal lock.
        constexpr float max_elev = 1.5f;  // ~86 degrees
        if (elevation > max_elev) elevation = max_elev;
        if (elevation < -max_elev) elevation = -max_elev;

        // Spherical to Cartesian.
        float cos_elev = std::cos(elevation);
        position[0] = target[0] + distance * std::sin(azimuth) * cos_elev;
        position[1] = target[1] + distance * std::sin(elevation);
        position[2] = target[2] + distance * std::cos(azimuth) * cos_elev;
    }

    void on_mouse_button(int button, int action, int /*mods*/) {
        if (button == 0) {  // Left button.
            dragging = (action == 1);  // GLFW_PRESS = 1
        }
    }

    void on_cursor_pos(double x, double y) {
        if (dragging) {
            double dx = x - last_x;
            double dy = y - last_y;

            // Sensitivity.
            constexpr float sens = 0.005f;
            azimuth -= static_cast<float>(dx) * sens;
            elevation += static_cast<float>(dy) * sens;

            update_position();
            dirty = true;
        }
        last_x = x;
        last_y = y;
    }

    void on_scroll(double /*x_offset*/, double y_offset) {
        // Zoom in/out by adjusting distance.
        constexpr float zoom_sens = 0.15f;
        distance -= static_cast<float>(y_offset) * zoom_sens;

        // Clamp distance to reasonable bounds.
        constexpr float min_dist = 1.0f;
        constexpr float max_dist = 10.0f;
        if (distance < min_dist) distance = min_dist;
        if (distance > max_dist) distance = max_dist;

        update_position();
        dirty = true;
    }

    void fill_camera(Q_camera& cam) const {
        cam.position[0] = position[0];
        cam.position[1] = position[1];
        cam.position[2] = position[2];
        cam.target[0] = target[0];
        cam.target[1] = target[1];
        cam.target[2] = target[2];
        cam.up[0] = 0.0f;
        cam.up[1] = 1.0f;
        cam.up[2] = 0.0f;
        cam.fov = fov;
    }
};

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

    // Set up orbit camera.
    orbit_camera camera;
    camera.update_position();

    // Mouse input callbacks.
    window.set_mouse_button_callback([&camera](int button, int action, int mods) {
        camera.on_mouse_button(button, action, mods);
    });
    window.set_cursor_pos_callback([&camera](double x, double y) {
        camera.on_cursor_pos(x, y);
    });
    window.set_scroll_callback([&camera](double x_offset, double y_offset) {
        camera.on_scroll(x_offset, y_offset);
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

            // Fill in camera data from orbit controller.
            camera.fill_camera(frame.camera);
            frame.camera_dirty = camera.dirty ? 1 : 0;
            camera.dirty = false;

            // Render plugin
            plugin.render(&frame);

            // Present
            metal.end_frame(frame);
        }
    }

    std::printf("Shutting down...\n");
    return EXIT_SUCCESS;
}
