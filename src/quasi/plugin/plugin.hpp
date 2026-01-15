/// @file plugin.hpp
/// @brief Main header for the plugin module.
///
/// This header includes all plugin module components. For finer-grained
/// control, include individual headers directly.

#pragma once

#include <quasi/plugin/plugin_interface.hpp>
#include <quasi/plugin/dynamic_library.hpp>
#include <quasi/plugin/loader.hpp>
#include <quasi/plugin/manager.hpp>

namespace Q::plugin {

/// @brief Major version of the plugin module.
inline constexpr int k_version_major = 0;

/// @brief Minor version of the plugin module.
inline constexpr int k_version_minor = 1;

/// @brief Patch version of the plugin module.
inline constexpr int k_version_patch = 0;

}  // namespace Q::plugin
