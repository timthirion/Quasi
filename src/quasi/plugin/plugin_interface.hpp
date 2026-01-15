/// @file plugin_interface.hpp
/// @brief Stable C ABI for hot-reloadable plugins.
///
/// This header defines the boundary between host and plugin. Everything here
/// uses C types and calling conventions for ABI stability across compiler
/// versions and build configurations.
///
/// The pattern:
/// - Q_plugin_context: Host -> Plugin communication (viewport, callbacks)
/// - Q_plugin_handle:  Opaque pointer to plugin's internal C++ state
/// - C functions:      The actual interface (create, destroy, update, render)

#pragma once

#include <quasi/gpu/types.hpp>

#include <cstddef>
#include <cstdint>

extern "C" {

/// @brief Opaque handle to plugin-internal state.
///
/// The host only ever sees a pointer to this type. The plugin defines
/// what's inside. This allows plugins to use any C++ types internally
/// while maintaining a stable C ABI.
struct Q_plugin_handle;

/// @brief Semantic version for a plugin.
struct Q_plugin_version {
    uint32_t major;  ///< Major version (breaking changes).
    uint32_t minor;  ///< Minor version (new features).
    uint32_t patch;  ///< Patch version (bug fixes).
};

/// @brief Plugin metadata returned by Q_plugin_get_info().
struct Q_plugin_info {
    const char*       name;         ///< Human-readable plugin name.
    Q_plugin_version  version;      ///< Plugin version.
    const char*       description;  ///< Brief description.
    const char*       author;       ///< Author or organization.
};

/// @brief Host-provided context passed to plugins.
///
/// Plugins receive this during creation and can use it to communicate
/// back to the host through callbacks and access GPU resources.
struct Q_plugin_context {
    uint32_t viewport_width;   ///< Current viewport width in pixels.
    uint32_t viewport_height;  ///< Current viewport height in pixels.
    void* host_data;           ///< Opaque pointer to host-specific data.
    Q_gpu_context* gpu;        ///< GPU device context (Metal, Vulkan, etc.)

    /// @brief Callback for plugin logging.
    void (*log)(void* host_data, const char* message);

    /// @brief Callback to request graceful shutdown.
    void (*request_shutdown)(void* host_data);
};

/// @name Plugin C API
/// @brief Functions that plugins must implement with extern "C" linkage.
///
/// Lifecycle:
/// 1. Host loads .dylib/.so
/// 2. Host calls Q_plugin_abi_version() to check compatibility
/// 3. Host calls Q_plugin_get_info() to get metadata
/// 4. Host calls Q_plugin_create(ctx) to instantiate
/// 5. Host calls Q_plugin_update()/Q_plugin_render() each frame
/// 6. Host calls Q_plugin_destroy() before unloading
/// 7. Host unloads .dylib/.so
/// @{

/// @brief Returns the plugin's ABI version.
uint32_t Q_plugin_abi_version(void);

/// @brief Returns plugin metadata.
Q_plugin_info Q_plugin_get_info(void);

/// @brief Creates a plugin instance.
/// @param ctx Host-provided context.
/// @return Opaque plugin handle, or nullptr on failure.
Q_plugin_handle* Q_plugin_create(Q_plugin_context* ctx);

/// @brief Destroys a plugin instance.
/// @param handle The plugin handle to destroy.
void Q_plugin_destroy(Q_plugin_handle* handle);

/// @brief Called each frame for game logic updates.
/// @param handle The plugin handle.
/// @param delta_time Seconds since the last update.
void Q_plugin_update(Q_plugin_handle* handle, float delta_time);

/// @brief Called each frame to render.
/// @param handle The plugin handle.
/// @param frame Per-frame render data (drawable, command buffer, etc.)
void Q_plugin_render(Q_plugin_handle* handle, Q_render_frame* frame);

/// @}

}  // extern "C"

namespace Q::plugin {

/// @brief Type aliases for use in C++ code.
/// @{
using plugin_handle  = Q_plugin_handle;
using plugin_version = Q_plugin_version;
using plugin_info    = Q_plugin_info;
using plugin_context = Q_plugin_context;
/// @}

/// @brief Symbol names for dlsym() lookup.
/// @{
inline constexpr const char* k_symbol_abi_version = "Q_plugin_abi_version";
inline constexpr const char* k_symbol_get_info    = "Q_plugin_get_info";
inline constexpr const char* k_symbol_create      = "Q_plugin_create";
inline constexpr const char* k_symbol_destroy     = "Q_plugin_destroy";
inline constexpr const char* k_symbol_update      = "Q_plugin_update";
inline constexpr const char* k_symbol_render      = "Q_plugin_render";
/// @}

/// @brief Current ABI version. Increment when the interface changes.
inline constexpr uint32_t k_plugin_abi_version = 1;

/// @brief Equality comparison for plugin versions.
[[nodiscard]] constexpr bool operator==(plugin_version a, plugin_version b) noexcept {
    return a.major == b.major && a.minor == b.minor && a.patch == b.patch;
}

/// @brief Less-than comparison for plugin versions.
[[nodiscard]] constexpr bool operator<(plugin_version a, plugin_version b) noexcept {
    if (a.major != b.major) return a.major < b.major;
    if (a.minor != b.minor) return a.minor < b.minor;
    return a.patch < b.patch;
}

}  // namespace Q::plugin
