# AGENTS.md

Guidance for AI agents working on this repository.

## Project Overview

Quasi is a C++23 rendering system using hot-swappable plugins to manage multiple GPU backends (Metal, WebGPU/Dawn, Vulkan). Each backend is a dynamically-loaded plugin that can be reloaded at runtime without restarting the host.

## Build System

- **Build Tool**: Bazel with Bzlmod
- **C++ Standard**: C++23 required
- **Compilers**: GCC 13+, Clang 16+, Apple Clang 15+
- **Testing**: Catch2 for unit tests; all features must have automated tests

### Dependencies

Use Bzlmod (`MODULE.bazel`) for all dependencies. Prefer libraries from the [Bazel Central Registry](https://registry.bazel.build/).

```python
# MODULE.bazel - preferred approach
bazel_dep(name = "catch2", version = "3.5.2")
bazel_dep(name = "abseil-cpp", version = "20240116.0")
```

For libraries not in BCR, use overrides:

```python
# From a git repository
bazel_dep(name = "some_lib", version = "1.0.0")
git_override(
    module_name = "some_lib",
    remote = "https://github.com/org/some_lib.git",
    commit = "abc123...",
)

# From an archive URL
archive_override(
    module_name = "some_lib",
    urls = ["https://github.com/org/some_lib/archive/v1.0.0.tar.gz"],
    strip_prefix = "some_lib-1.0.0",
)
```

Do not use git submodules or WORKSPACE-based dependencies.

## Architecture

- **Plugin System**: C ABI boundary with opaque handles; C++ internals hidden from host
- **Async Model**: Coroutine-based cooperative scheduler (single-threaded, no mutexes)
- **Hot Reload**: File watching → pre-unload hooks → copy library to temp → dlopen → ABI check → post-load hooks

## Coding Style

### Naming Conventions

| Category | Style | Examples |
|----------|-------|----------|
| Namespaces | snake_case | `Q::`, `Q::gpu::`, `Q::async::` |
| Classes/Structs | snake_case | `task`, `plugin`, `dynamic_library` |
| Functions | snake_case | `get_symbol()`, `has_changed()` |
| Member variables | snake_case + trailing `_` | `handle_`, `path_`, `context_` |
| Constants | k_prefix or UPPER_CASE | `k_plugin_abi_version`, `VERSION` |
| Template params | PascalCase | `Rep`, `Period`, `Pred` |
| Thread-local | t_ prefix | `t_current_scheduler` |
| Function pointers | snake_case + `_fn` suffix | `create_fn`, `destroy_fn` |

### File Organization

- **Extensions**: `.hpp` headers, `.cpp` sources (no `.h` files)
- **Naming**: lowercase with underscores (`ppm_writer.hpp`)
- **Include style**: `#include <quasi/module/file.hpp>`
- **Guards**: `#pragma once`
- **One class per file** unless tightly coupled

### Formatting

- **Indentation**: 4 spaces
- **Braces**: K&R style (open brace at end of line)
- **Line length**: ~100 characters soft limit

### Documentation

Use Doxygen-style comments. Never use decorative bar comments (e.g., `//------`).

```cpp
/// @brief Loads a plugin from a dynamic library.
/// @param library The dynamic library containing the plugin.
/// @param context Host-provided context for plugin communication.
/// @return The loaded plugin, or an error if loading failed.
[[nodiscard]] result<plugin> load(dynamic_library& library, plugin_context* context);

/// Single-line descriptions use triple-slash.
[[nodiscard]] bool is_valid() const noexcept;
```

For classes and files:

```cpp
/// @file manager.hpp
/// @brief Manages hot-reloadable plugins with async file watching.

/// @class manager
/// @brief Orchestrates plugin lifecycle with hot-reload support.
///
/// Watches the plugin library file for changes and automatically
/// reloads when modifications are detected. Integrates with the
/// async scheduler for non-blocking operation.
class manager { ... };
```

### Modern C++ Patterns

```cpp
// Error handling: std::expected instead of exceptions
template <typename T>
using result = std::expected<T, error>;

// Attributes for clarity
[[nodiscard]] result<plugin> load(const path& lib_path);

// RAII and move-only types
class dynamic_library {
    dynamic_library(const dynamic_library&) = delete;
    dynamic_library& operator=(const dynamic_library&) = delete;
    dynamic_library(dynamic_library&&) noexcept;
    ~dynamic_library() { close(); }
};

// Concepts and constraints
template <std::invocable<> Pred>
requires std::convertible_to<std::invoke_result_t<Pred>, bool>
task<void> wait_until(Pred pred);

// Designated initializers
return plugin_info{
    .name = NAME,
    .version = VERSION,
};
```

### Namespace

All code in the `Q` namespace with nested organization:
- `Q::async::` - Coroutines and scheduling
- `Q::plugin::` - Plugin loading and hot-reload management
- `Q::gpu::` - GPU backend abstractions (future)

## Git Workflow

- Use `git mv` for moves/renames to preserve history
- Never force push to main
