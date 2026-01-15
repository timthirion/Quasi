/// @file window.hpp
/// @brief GLFW window wrapper for the host application.

#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <expected>
#include <functional>
#include <string>
#include <string_view>

namespace Q::host {

/// @brief Error codes for window operations.
enum class window_error {
    glfw_init_failed,  ///< GLFW initialization failed.
    create_failed,     ///< Window creation failed.
};

/// @brief Converts a window_error to a human-readable string.
[[nodiscard]] constexpr const char* to_string(window_error e) noexcept {
    switch (e) {
        case window_error::glfw_init_failed: return "GLFW initialization failed";
        case window_error::create_failed:    return "Window creation failed";
    }
    return "Unknown error";
}

/// @class window
/// @brief RAII wrapper for a GLFW window.
///
/// Handles window creation, event polling, and native handle access
/// for GPU context setup.
class window {
public:
    template <typename T>
    using result = std::expected<T, window_error>;

    /// @brief Callback type for resize events.
    using resize_callback = std::function<void(uint32_t width, uint32_t height)>;

    /// @brief Creates a new window.
    /// @param title Window title.
    /// @param width Initial width in screen coordinates.
    /// @param height Initial height in screen coordinates.
    /// @return The window, or an error if creation failed.
    [[nodiscard]] static result<window> create(
        std::string_view title,
        uint32_t width,
        uint32_t height
    );

    window() = default;
    ~window();

    window(window&& other) noexcept;
    window& operator=(window&& other) noexcept;

    window(const window&) = delete;
    window& operator=(const window&) = delete;

    /// @brief Polls for window events (non-blocking).
    void poll_events();

    /// @brief Returns true if the window should close.
    [[nodiscard]] bool should_close() const;

    /// @brief Requests window closure.
    void close();

    /// @brief Returns the native window handle (NSWindow* on macOS).
    [[nodiscard]] void* native_handle() const;

    /// @brief Returns the GLFW window pointer.
    [[nodiscard]] GLFWwindow* glfw_handle() const noexcept { return window_; }

    /// @brief Returns the framebuffer width in pixels.
    [[nodiscard]] uint32_t framebuffer_width() const;

    /// @brief Returns the framebuffer height in pixels.
    [[nodiscard]] uint32_t framebuffer_height() const;

    /// @brief Sets a callback for resize events.
    void set_resize_callback(resize_callback callback);

    /// @brief Checks if the window is valid.
    [[nodiscard]] bool is_valid() const noexcept { return window_ != nullptr; }

    /// @brief Boolean conversion for validity check.
    [[nodiscard]] explicit operator bool() const noexcept { return is_valid(); }

private:
    GLFWwindow* window_ = nullptr;
    resize_callback resize_callback_;

    static void framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height);
};

}  // namespace Q::host
