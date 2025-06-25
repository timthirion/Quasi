#find_package(Dawn REQUIRED)
find_package(glfw3 REQUIRED)

project(apps)

# Test app
add_executable(test_app quasi.cpp)
target_link_libraries(test_app PRIVATE glfw)

# Raytracer app (multi-threaded tile-based)
add_executable(rt rt.cpp)
target_link_libraries(rt PRIVATE quasi)
target_compile_features(rt PRIVATE cxx_std_23)

# Single-threaded raytracer app (reference implementation)
add_executable(rt_single rt_single.cpp)
target_link_libraries(rt_single PRIVATE quasi)
target_compile_features(rt_single PRIVATE cxx_std_23)

# Metal Triangle App - build separately with: swiftc -o metal_triangle metal_triangle.swift -framework Cocoa -framework Metal -framework MetalKit
