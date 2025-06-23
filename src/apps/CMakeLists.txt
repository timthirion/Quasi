#find_package(Dawn REQUIRED)
find_package(glfw3 REQUIRED)

project(apps)

# Test app
add_executable(test_app quasi.cpp)
target_link_libraries(test_app PRIVATE glfw)

# Raytracer app
add_executable(rt rt.cpp)
target_link_libraries(rt PRIVATE quasi)
target_compile_features(rt PRIVATE cxx_std_23)

# Metal Triangle App - build separately with: swiftc -o metal_triangle metal_triangle.swift -framework Cocoa -framework Metal -framework MetalKit
