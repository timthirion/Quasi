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

/// @brief CPU-side framebuffer data returned by Q_plugin_readback().
///
/// The plugin allocates the data buffer. The host must call
/// Q_plugin_readback_free() when finished.
struct Q_readback_result {
    float*   data;      ///< RGBA float pixel data, row-major, top-to-bottom.
    uint32_t width;     ///< Image width in pixels.
    uint32_t height;    ///< Image height in pixels.
    uint32_t channels;  ///< Number of channels (always 4 for RGBA).
};

/// @brief Identifies an AOV buffer type.
enum Q_aov_type : uint32_t {
    Q_AOV_BEAUTY = 0,  ///< Path-traced beauty (accumulated).
    Q_AOV_ALBEDO = 1,  ///< First-hit surface albedo.
    Q_AOV_NORMAL = 2,  ///< First-hit world-space normal.
    Q_AOV_DEPTH  = 3,  ///< First-hit ray distance.
    Q_AOV_COUNT  = 4,  ///< Number of AOV types.
};

/// @brief Single AOV buffer from readback.
struct Q_aov_buffer {
    float*   data;      ///< Pixel data (RGBA float, row-major). nullptr if unavailable.
    uint32_t width;     ///< Image width in pixels.
    uint32_t height;    ///< Image height in pixels.
    uint32_t channels;  ///< Number of channels (always 4 for RGBA).
};

/// @brief Result of reading back all AOV buffers.
struct Q_readback_aov_result {
    Q_aov_buffer buffers[Q_AOV_COUNT];  ///< Indexed by Q_aov_type.
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

/// @brief Reads back the current accumulated HDR framebuffer to CPU memory.
/// @param handle The plugin handle.
/// @return Readback result with pixel data, or a result with data == nullptr on failure.
Q_readback_result Q_plugin_readback(Q_plugin_handle* handle);

/// @brief Frees memory allocated by Q_plugin_readback().
/// @param result The readback result to free.
void Q_plugin_readback_free(Q_readback_result* result);

/// @brief Reads back all AOV buffers (beauty + albedo + normal + depth).
/// @param handle The plugin handle.
/// @return AOV readback result. Buffers with data == nullptr are unavailable.
Q_readback_aov_result Q_plugin_readback_aov(Q_plugin_handle* handle);

/// @brief Frees memory allocated by Q_plugin_readback_aov().
/// @param result The AOV readback result to free.
void Q_plugin_readback_aov_free(Q_readback_aov_result* result);

/// @}

}  // extern "C"

namespace Q::plugin {

/// @brief Type aliases for use in C++ code.
/// @{
using plugin_handle  = Q_plugin_handle;
using plugin_version = Q_plugin_version;
using plugin_info    = Q_plugin_info;
using plugin_context = Q_plugin_context;
using readback_result     = Q_readback_result;
using aov_type            = Q_aov_type;
using aov_buffer          = Q_aov_buffer;
using readback_aov_result = Q_readback_aov_result;
/// @}

/// @brief Symbol names for dlsym() lookup.
/// @{
inline constexpr const char* k_symbol_abi_version       = "Q_plugin_abi_version";
inline constexpr const char* k_symbol_get_info          = "Q_plugin_get_info";
inline constexpr const char* k_symbol_create            = "Q_plugin_create";
inline constexpr const char* k_symbol_destroy           = "Q_plugin_destroy";
inline constexpr const char* k_symbol_update            = "Q_plugin_update";
inline constexpr const char* k_symbol_render            = "Q_plugin_render";
inline constexpr const char* k_symbol_readback          = "Q_plugin_readback";
inline constexpr const char* k_symbol_readback_free     = "Q_plugin_readback_free";
inline constexpr const char* k_symbol_readback_aov      = "Q_plugin_readback_aov";
inline constexpr const char* k_symbol_readback_aov_free = "Q_plugin_readback_aov_free";
/// @}

/// @brief Current ABI version. Increment when the interface changes.
inline constexpr uint32_t k_plugin_abi_version = 3;

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
