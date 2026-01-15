/// @file loader.hpp
/// @brief C++ wrapper for loaded plugins.

#pragma once

#include <quasi/plugin/plugin_interface.hpp>
#include <quasi/plugin/dynamic_library.hpp>

#include <expected>
#include <memory>
#include <optional>
#include <string>

namespace Q::plugin {

/// @class loader
/// @brief C++ wrapper that manages a loaded plugin's lifecycle.
///
/// Provides a type-safe interface over the raw C function pointers from
/// a plugin. Handles symbol resolution, ABI version checking, and
/// automatic cleanup.
///
/// Example usage:
/// @code
/// auto lib_result = dynamic_library::open("libbackend.dylib");
/// if (!lib_result) return;
///
/// plugin_context ctx{.viewport_width = 1920, .viewport_height = 1080};
/// auto plugin_result = loader::load(*lib_result, &ctx);
/// if (!plugin_result) return;
///
/// plugin_result->update(delta_time);
/// plugin_result->render();
/// @endcode
class loader {
public:
    /// @brief Error codes for plugin loading.
    enum class error {
        abi_mismatch,      ///< Plugin ABI version doesn't match host.
        symbol_not_found,  ///< Required symbol missing from library.
        create_failed,     ///< Q_plugin_create() returned nullptr.
    };

    template <typename T>
    using result = std::expected<T, error>;

    /// @brief Function pointer types matching the C interface.
    /// @{
    using abi_version_fn = uint32_t (*)();
    using get_info_fn    = plugin_info (*)();
    using create_fn      = plugin_handle* (*)(plugin_context*);
    using destroy_fn     = void (*)(plugin_handle*);
    using update_fn      = void (*)(plugin_handle*, float);
    using render_fn      = void (*)(plugin_handle*, Q::gpu::render_frame*);
    /// @}

    /// @brief Loads a plugin from a dynamic library.
    /// @param library The library containing the plugin.
    /// @param context Host-provided context for the plugin.
    /// @return The loaded plugin wrapper, or an error.
    [[nodiscard]] static result<loader> load(
        dynamic_library& library,
        plugin_context* context
    ) {
        loader p;

        // Resolve all required symbols
        if (auto sym = library.get_symbol<abi_version_fn>(k_symbol_abi_version)) {
            p.fn_abi_version_ = *sym;
        } else {
            return std::unexpected{error::symbol_not_found};
        }

        if (auto sym = library.get_symbol<get_info_fn>(k_symbol_get_info)) {
            p.fn_get_info_ = *sym;
        } else {
            return std::unexpected{error::symbol_not_found};
        }

        if (auto sym = library.get_symbol<create_fn>(k_symbol_create)) {
            p.fn_create_ = *sym;
        } else {
            return std::unexpected{error::symbol_not_found};
        }

        if (auto sym = library.get_symbol<destroy_fn>(k_symbol_destroy)) {
            p.fn_destroy_ = *sym;
        } else {
            return std::unexpected{error::symbol_not_found};
        }

        if (auto sym = library.get_symbol<update_fn>(k_symbol_update)) {
            p.fn_update_ = *sym;
        } else {
            return std::unexpected{error::symbol_not_found};
        }

        if (auto sym = library.get_symbol<render_fn>(k_symbol_render)) {
            p.fn_render_ = *sym;
        } else {
            return std::unexpected{error::symbol_not_found};
        }

        // Check ABI version
        uint32_t plugin_abi = p.fn_abi_version_();
        if (plugin_abi != k_plugin_abi_version) {
            return std::unexpected{error::abi_mismatch};
        }

        // Create the plugin instance
        p.handle_ = p.fn_create_(context);
        if (!p.handle_) {
            return std::unexpected{error::create_failed};
        }

        return p;
    }

    loader() = default;

    ~loader() {
        destroy();
    }

    loader(loader&& other) noexcept
        : handle_{std::exchange(other.handle_, nullptr)}
        , fn_abi_version_{std::exchange(other.fn_abi_version_, nullptr)}
        , fn_get_info_{std::exchange(other.fn_get_info_, nullptr)}
        , fn_create_{std::exchange(other.fn_create_, nullptr)}
        , fn_destroy_{std::exchange(other.fn_destroy_, nullptr)}
        , fn_update_{std::exchange(other.fn_update_, nullptr)}
        , fn_render_{std::exchange(other.fn_render_, nullptr)}
    {}

    loader& operator=(loader&& other) noexcept {
        if (this != &other) {
            destroy();
            handle_ = std::exchange(other.handle_, nullptr);
            fn_abi_version_ = std::exchange(other.fn_abi_version_, nullptr);
            fn_get_info_ = std::exchange(other.fn_get_info_, nullptr);
            fn_create_ = std::exchange(other.fn_create_, nullptr);
            fn_destroy_ = std::exchange(other.fn_destroy_, nullptr);
            fn_update_ = std::exchange(other.fn_update_, nullptr);
            fn_render_ = std::exchange(other.fn_render_, nullptr);
        }
        return *this;
    }

    loader(const loader&) = delete;
    loader& operator=(const loader&) = delete;

    /// @brief Calls the plugin's update function.
    /// @param delta_time Seconds since the last update.
    void update(float delta_time) {
        if (handle_ && fn_update_) {
            fn_update_(handle_, delta_time);
        }
    }

    /// @brief Calls the plugin's render function.
    /// @param frame Per-frame render data (drawable, command buffer, etc.)
    void render(Q::gpu::render_frame* frame) {
        if (handle_ && fn_render_) {
            fn_render_(handle_, frame);
        }
    }

    /// @brief Returns the plugin's metadata.
    [[nodiscard]] plugin_info info() const {
        if (fn_get_info_) {
            return fn_get_info_();
        }
        return {};
    }

    /// @brief Returns the plugin's ABI version.
    [[nodiscard]] uint32_t abi_version() const {
        if (fn_abi_version_) {
            return fn_abi_version_();
        }
        return 0;
    }

    /// @brief Checks if the plugin is valid and ready to use.
    [[nodiscard]] bool is_valid() const noexcept {
        return handle_ != nullptr;
    }

    /// @brief Boolean conversion for validity check.
    [[nodiscard]] explicit operator bool() const noexcept {
        return is_valid();
    }

    /// @brief Manually destroys the plugin instance.
    void destroy() {
        if (handle_ && fn_destroy_) {
            fn_destroy_(handle_);
            handle_ = nullptr;
        }
    }

private:
    plugin_handle* handle_ = nullptr;

    abi_version_fn fn_abi_version_ = nullptr;
    get_info_fn    fn_get_info_    = nullptr;
    create_fn      fn_create_      = nullptr;
    destroy_fn     fn_destroy_     = nullptr;
    update_fn      fn_update_      = nullptr;
    render_fn      fn_render_      = nullptr;
};

/// @brief Converts a loader error to a human-readable string.
/// @param e The error code.
/// @return String description of the error.
[[nodiscard]] inline constexpr const char* to_string(loader::error e) noexcept {
    switch (e) {
        case loader::error::abi_mismatch:     return "ABI version mismatch";
        case loader::error::symbol_not_found: return "Required symbol not found";
        case loader::error::create_failed:    return "Plugin creation failed";
    }
    return "Unknown error";
}

}  // namespace Q::plugin
