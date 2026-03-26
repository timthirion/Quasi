/// @file window.cpp
/// @brief GLFW window wrapper implementation.

#include <quasi/host/window.hpp>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

namespace Q::host {

window::~window() {
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
}

window::window(window&& other) noexcept
    : window_{other.window_}
    , resize_callback_{std::move(other.resize_callback_)}
    , mouse_button_callback_{std::move(other.mouse_button_callback_)}
    , cursor_pos_callback_{std::move(other.cursor_pos_callback_)}
    , scroll_callback_{std::move(other.scroll_callback_)}
    , key_callback_{std::move(other.key_callback_)}
{
    other.window_ = nullptr;
    if (window_) {
        glfwSetWindowUserPointer(window_, this);
    }
}

window& window::operator=(window&& other) noexcept {
    if (this != &other) {
        if (window_) {
            glfwDestroyWindow(window_);
        }
        window_ = other.window_;
        resize_callback_ = std::move(other.resize_callback_);
        mouse_button_callback_ = std::move(other.mouse_button_callback_);
        cursor_pos_callback_ = std::move(other.cursor_pos_callback_);
        scroll_callback_ = std::move(other.scroll_callback_);
        key_callback_ = std::move(other.key_callback_);
        other.window_ = nullptr;
        if (window_) {
            glfwSetWindowUserPointer(window_, this);
        }
    }
    return *this;
}

auto window::create(
    std::string_view title,
    uint32_t width,
    uint32_t height
) -> result<window> {
    if (!glfwInit()) {
        return std::unexpected{window_error::glfw_init_failed};
    }

    // No OpenGL context - we're using Metal
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* glfw_window = glfwCreateWindow(
        static_cast<int>(width),
        static_cast<int>(height),
        std::string{title}.c_str(),
        nullptr,
        nullptr
    );

    if (!glfw_window) {
        return std::unexpected{window_error::create_failed};
    }

    window win;
    win.window_ = glfw_window;
    glfwSetWindowUserPointer(glfw_window, &win);
    glfwSetFramebufferSizeCallback(glfw_window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(glfw_window, mouse_button_callback_glfw);
    glfwSetCursorPosCallback(glfw_window, cursor_pos_callback_glfw);
    glfwSetScrollCallback(glfw_window, scroll_callback_glfw);
    glfwSetKeyCallback(glfw_window, key_callback_glfw);

    return win;
}

void window::poll_events() {
    glfwPollEvents();
}

bool window::should_close() const {
    return window_ && glfwWindowShouldClose(window_);
}

void window::close() {
    if (window_) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

void* window::native_handle() const {
    if (!window_) {
        return nullptr;
    }
    return static_cast<void*>(glfwGetCocoaWindow(window_));
}

uint32_t window::framebuffer_width() const {
    if (!window_) return 0;
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    return static_cast<uint32_t>(width);
}

uint32_t window::framebuffer_height() const {
    if (!window_) return 0;
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    return static_cast<uint32_t>(height);
}

void window::set_resize_callback(resize_callback callback) {
    resize_callback_ = std::move(callback);
}

void window::set_mouse_button_callback(mouse_button_callback callback) {
    mouse_button_callback_ = std::move(callback);
}

void window::set_cursor_pos_callback(cursor_pos_callback callback) {
    cursor_pos_callback_ = std::move(callback);
}

void window::framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height) {
    auto* self = static_cast<window*>(glfwGetWindowUserPointer(glfw_window));
    if (self && self->resize_callback_) {
        self->resize_callback_(
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        );
    }
}

void window::mouse_button_callback_glfw(GLFWwindow* glfw_window, int button, int action, int mods) {
    auto* self = static_cast<window*>(glfwGetWindowUserPointer(glfw_window));
    if (self && self->mouse_button_callback_) {
        self->mouse_button_callback_(button, action, mods);
    }
}

void window::cursor_pos_callback_glfw(GLFWwindow* glfw_window, double x, double y) {
    auto* self = static_cast<window*>(glfwGetWindowUserPointer(glfw_window));
    if (self && self->cursor_pos_callback_) {
        self->cursor_pos_callback_(x, y);
    }
}

void window::set_scroll_callback(scroll_callback callback) {
    scroll_callback_ = std::move(callback);
}

void window::scroll_callback_glfw(GLFWwindow* glfw_window, double x_offset, double y_offset) {
    auto* self = static_cast<window*>(glfwGetWindowUserPointer(glfw_window));
    if (self && self->scroll_callback_) {
        self->scroll_callback_(x_offset, y_offset);
    }
}

void window::set_key_callback(key_callback callback) {
    key_callback_ = std::move(callback);
}

void window::key_callback_glfw(GLFWwindow* glfw_window, int key, int scancode, int action, int mods) {
    auto* self = static_cast<window*>(glfwGetWindowUserPointer(glfw_window));
    if (self && self->key_callback_) {
        self->key_callback_(key, scancode, action, mods);
    }
}

}  // namespace Q::host
