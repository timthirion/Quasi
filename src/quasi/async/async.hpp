/// @file async.hpp
/// @brief Main header for the async coroutine module.
///
/// This header includes all async module components. For finer-grained
/// control, include individual headers directly.

#pragma once

#include <quasi/async/task.hpp>
#include <quasi/async/scheduler.hpp>
#include <quasi/async/awaitables.hpp>
#include <quasi/async/file_watcher.hpp>

namespace Q::async {

/// @brief Major version of the async module.
inline constexpr int k_version_major = 0;

/// @brief Minor version of the async module.
inline constexpr int k_version_minor = 1;

/// @brief Patch version of the async module.
inline constexpr int k_version_patch = 0;

}  // namespace Q::async
