/// @file context.hpp
/// @brief Metal-specific GPU context management.
///
/// Provides RAII wrapper for Metal device, command queue, and layer setup.
/// This is macOS-only code that uses Objective-C++ internally.

#pragma once

#include <quasi/gpu/types.hpp>

#include <expected>
#include <memory>
#include <string>
#include <string_view>

namespace Q::gpu::metal {

/// @brief Error codes for Metal context operations.
enum class context_error {
    device_not_found,   ///< No Metal-capable GPU found.
    layer_setup_failed, ///< Failed to create or configure CAMetalLayer.
    no_drawable,        ///< Failed to get next drawable from layer.
};

/// @brief Converts a context_error to a human-readable string.
[[nodiscard]] constexpr const char* to_string(context_error e) noexcept {
    switch (e) {
        case context_error::device_not_found:   return "Metal device not found";
        case context_error::layer_setup_failed: return "Metal layer setup failed";
        case context_error::no_drawable:        return "Failed to get drawable";
    }
    return "Unknown error";
}

/// @class context
/// @brief RAII wrapper for Metal GPU resources.
///
/// Manages MTLDevice, MTLCommandQueue, and CAMetalLayer. Provides methods
/// to begin and end frames for rendering.
///
/// Example:
/// @code
/// auto ctx_result = context::create(native_window);
/// if (!ctx_result) return;
///
/// while (running) {
///     auto frame = ctx_result->begin_frame();
///     if (frame) {
///         // Render using frame->command_buffer and frame->drawable
///         ctx_result->end_frame(*frame);
///     }
/// }
/// @endcode
class context {
public:
    template <typename T>
    using result = std::expected<T, context_error>;

    /// @brief Creates a Metal context attached to a native window.
    /// @param native_window Platform window handle (NSWindow* on macOS).
    /// @return The Metal context, or an error if setup failed.
    [[nodiscard]] static result<context> create(void* native_window);

    context() = default;
    ~context();

    context(context&& other) noexcept;
    context& operator=(context&& other) noexcept;

    context(const context&) = delete;
    context& operator=(const context&) = delete;

    /// @brief Returns the C ABI GPU context for plugins.
    [[nodiscard]] gpu_context* gpu() noexcept { return &gpu_ctx_; }
    [[nodiscard]] const gpu_context* gpu() const noexcept { return &gpu_ctx_; }

    /// @brief Begins a new frame for rendering.
    /// @return Per-frame render data, or error if no drawable available.
    [[nodiscard]] result<render_frame> begin_frame();

    /// @brief Ends the current frame and presents.
    /// @param frame The frame data from begin_frame().
    void end_frame(render_frame& frame);

    /// @brief Resizes the Metal layer to match new dimensions.
    /// @param width New width in pixels.
    /// @param height New height in pixels.
    void resize(uint32_t width, uint32_t height);

    /// @brief Returns current drawable width.
    [[nodiscard]] uint32_t width() const noexcept { return width_; }

    /// @brief Returns current drawable height.
    [[nodiscard]] uint32_t height() const noexcept { return height_; }

private:
    gpu_context gpu_ctx_{};
    void* frame_semaphore_ = nullptr;  // dispatch_semaphore_t for triple buffering
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    void release();
};

}  // namespace Q::gpu::metal
