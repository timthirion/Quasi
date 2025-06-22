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

### Running the Application
```bash
# From build directory
./quasi-build/src/apps/test_app
```

## Code Architecture

- **Core geometry**: `src/geometry.h` and `src/geometry.cpp` - Ray-triangle intersection and vector math
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

### Code Organization
- Header files should use `#pragma once` for include guards
- Place all implementation in `.cpp` files when possible
- Use meaningful, descriptive names that clearly indicate purpose
- Prefer explicit over implicit when it improves clarity

### Example
```cpp
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
