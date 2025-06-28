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

# Metal raytracer app (GPU-accelerated)
if(APPLE)
    add_executable(rt_metal rt_metal.cpp)
    target_link_libraries(rt_metal PRIVATE quasi)
    target_compile_features(rt_metal PRIVATE cxx_std_23)
    
    # Link Metal framework on macOS
    find_library(METAL_FRAMEWORK Metal)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    target_link_libraries(rt_metal PRIVATE ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK})
    
    # Copy Metal shader file to build directory
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/raytracer.metal ${CMAKE_BINARY_DIR}/raytracer.metal COPYONLY)
endif()

# Metal Triangle App - build separately with: swiftc -o metal_triangle metal_triangle.swift -framework Cocoa -framework Metal -framework MetalKit
