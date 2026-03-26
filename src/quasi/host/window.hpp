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

    /// @brief Callback type for mouse button events.
    using mouse_button_callback = std::function<void(int button, int action, int mods)>;

    /// @brief Callback type for cursor position events.
    using cursor_pos_callback = std::function<void(double x, double y)>;

    /// @brief Callback type for scroll events.
    using scroll_callback = std::function<void(double x_offset, double y_offset)>;

    /// @brief Callback type for key events.
    using key_callback = std::function<void(int key, int scancode, int action, int mods)>;

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

    /// @brief Sets a callback for mouse button events.
    void set_mouse_button_callback(mouse_button_callback callback);

    /// @brief Sets a callback for cursor position events.
    void set_cursor_pos_callback(cursor_pos_callback callback);

    /// @brief Sets a callback for scroll events.
    void set_scroll_callback(scroll_callback callback);

    /// @brief Sets a callback for key events.
    void set_key_callback(key_callback callback);

    /// @brief Checks if the window is valid.
    [[nodiscard]] bool is_valid() const noexcept { return window_ != nullptr; }

    /// @brief Boolean conversion for validity check.
    [[nodiscard]] explicit operator bool() const noexcept { return is_valid(); }

private:
    GLFWwindow* window_ = nullptr;
    resize_callback resize_callback_;
    mouse_button_callback mouse_button_callback_;
    cursor_pos_callback cursor_pos_callback_;
    scroll_callback scroll_callback_;
    key_callback key_callback_;

    static void framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height);
    static void mouse_button_callback_glfw(GLFWwindow* glfw_window, int button, int action, int mods);
    static void cursor_pos_callback_glfw(GLFWwindow* glfw_window, double x, double y);
    static void scroll_callback_glfw(GLFWwindow* glfw_window, double x_offset, double y_offset);
    static void key_callback_glfw(GLFWwindow* glfw_window, int key, int scancode, int action, int mods);
};

}  // namespace Q::host
