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

void window::framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height) {
    auto* self = static_cast<window*>(glfwGetWindowUserPointer(glfw_window));
    if (self && self->resize_callback_) {
        self->resize_callback_(
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        );
    }
}

}  // namespace Q::host
