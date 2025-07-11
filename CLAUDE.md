# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Quasi is a rendering system with GLFW for windowing and geometry processing capabilities. The project uses a CMake-based superbuild system to manage external dependencies.

## Build System Architecture

This project uses a two-stage CMake superbuild approach:

1. **Superbuild stage** (`QUASI_USE_SUPERBUILD=ON`, default): Downloads and builds external dependencies (GLFW, Catch2) as ExternalProjects, then builds the main application
2. **Main build stage** (`QUASI_USE_SUPERBUILD=OFF`): Builds only the Quasi application, expecting dependencies to already be available

The superbuild automatically handles dependency management and ensures proper build order.

## Essential Commands

### Important Build Note
**If you don't see expected output or encounter unexpected behavior, immediately do a full clean build:**
```bash
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Building the Project
```bash
# Clean build (recommended for first time)
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Development build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### Running the Applications
```bash
# From build directory

# Multi-threaded tile-based ray tracer (default, recommended)
./quasi-build/src/apps/rt [scene.json] [output.ppm]

# Single-threaded ray tracer (reference implementation)
./quasi-build/src/apps/rt_single [scene.json] [output.ppm]

# Test application (GLFW windowing)
./quasi-build/src/apps/test_app
```

## Code Architecture

- **Unified library**: Single `libquasi` library containing all core functionality
- **Core modules**: 
  - `geometry/` - Ray-triangle intersection, vector math, geometric primitives
  - `radiometry/` - Color, camera, and rendering utilities  
  - `io/` - File I/O including PPM image writing
  - `materials/` - Texture system with base class and implementations
- **Dependencies**: Managed via CMake ExternalProjects in `3rdparty/`
  - GLFW (windowing) - included as git submodule
  - Catch2 (testing framework) - included as git submodule
- **Build configuration**: `CMakeLists.txt` handles superbuild logic, `Superbuild.cmake` orchestrates dependency builds

## Dependency Management

External dependencies are built automatically via CMake ExternalProjects:
- GLFW is included as a git submodule in `3rdparty/glfw/`
- Catch2 is included as a git submodule in `3rdparty/catch2/`
- All dependencies install to `${CMAKE_BINARY_DIR}/{dep}-install` directories

## Important Build Notes

- Uses C++23 standard
- All dependencies are included as git submodules for reliable CI builds
- Build artifacts are separated: external deps in `{dep}-build/` directories, main app in `quasi-build/`
- The superbuild ensures proper dependency ordering (GLFW and Catch2 built before main application)

## Coding Style Guidelines

### Namespace Convention
- All C++ code should be placed in the global `Q` namespace
- Use nested namespaces for organization (e.g., `Q::geometry`, `Q::rendering`, `Q::utils`)

### Naming Conventions
- **Functions and methods**: Use `snake_case` (e.g., `ray_triangle_intersection()`, `get_length()`)
- **Variables**: Use `snake_case` (e.g., `intersection_point`, `edge_length`)
- **Classes and structs**: Use `PascalCase` (e.g., `Vec3`, `Ray`, `Triangle`)
- **Constants**: Use `UPPER_SNAKE_CASE` (e.g., `EPSILON`, `MAX_ITERATIONS`)
- **Namespaces**: Use `lowercase` (e.g., `geometry`, `rendering`)
- **Files**: Use `lowercase` with underscores for separation (e.g., `vec3.hpp`, `ppm_writer.hpp`)
- **Headers**: Use `.hpp` extension for all header files
- **Getters**: Use simple property name without `get_` prefix (e.g., `width()` not `get_width()`, `position()` not `get_position()`)

### Code Organization
- **File Structure**: Generally use one class per header/source file pair unless classes are very simple or always work together
- **Header files**: Should use `#pragma once` for include guards
- **Implementation**: Place all implementation in `.cpp` files when possible
- **Naming**: Use meaningful, descriptive names that clearly indicate purpose
- **Clarity**: Prefer explicit over implicit when it improves clarity

### File Naming Convention
- **All files use lowercase**: `vec3.hpp`/`vec3.cpp`, `ray.hpp`/`ray.cpp`, `sphere.hpp`/`sphere.cpp`
- **Header extension**: Use `.hpp` for all header files
- **Source extension**: Use `.cpp` for all source files  
- **Module organization**: Classes organized in module directories (e.g., `src/geometry/`, `src/radiometry/`)
- **Include style**: External consumers use `#include <quasi/module/file.hpp>` format
- **One class per file**: Each class has its own header and source file for better modularity

### Example
```cpp
// External consumers use namespaced includes
#include <quasi/geometry/geometry.hpp>
#include <quasi/radiometry/color.hpp>
#include <quasi/materials/texture.hpp>

namespace Q {
namespace geometry {
  
  struct Vec3 {
    float x, y, z;
    
    float get_length() const;
    Vec3 get_normalized() const;
    float dot_product(const Vec3& other) const;
  };
  
  std::optional<IntersectionResult> ray_triangle_intersection(const Ray& ray, const Triangle& triangle);
  
} // namespace geometry
} // namespace Q
```

## Design Philosophy

### Asynchronous Programming Preference
- **Modern async APIs**: Prefer asynchronous APIs where they provide performance or design benefits
- **Futures and promises**: Use std::future/std::promise for asynchronous task coordination
- **Multithreading**: Leverage thread pools and parallel algorithms for CPU-intensive work
- **Coroutines (C++20)**: Use coroutines for async I/O operations, task pipelines, and elegant async control flow
- **Performance-first**: Choose async patterns that genuinely improve performance, not complexity for its own sake

## Project Rules

### File Naming Requirements
- **All header files must use .hpp extension** - No .h files allowed
- **All file names must be lowercase** - Use underscores for word separation (e.g., `ppm_writer.hpp`)
- **External includes use quasi/ prefix** - Applications should include `<quasi/module/file.hpp>`

### Git Workflow Requirements
- **Use `git mv` for file moves** - Always use `git mv` instead of `mv` + `git add` to preserve git history
- **Use `git mv` for directory reorganization** - When restructuring directories, use `git mv` to maintain version control history
- **Example**: `git mv scenes/ data/scenes/` instead of `mv scenes/ data/` followed by `git add data/`
- **Rationale**: Preserves file history, blame information, and makes diffs cleaner

### Library Architecture  
- **Unified libquasi**: Single library contains all core functionality
- **Modular organization**: Code organized into logical modules (geometry, radiometry, io, materials)
- **Clean external API**: External consumers use standardized `quasi/` include paths
