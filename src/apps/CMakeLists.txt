#find_package(Dawn REQUIRED)
find_package(glfw3 REQUIRED)

project(test_app)

add_executable(test_app quasi.cpp)

target_link_libraries(test_app PRIVATE glfw)

# Metal Triangle App - build separately with: swiftc -o metal_triangle metal_triangle.swift -framework Cocoa -framework Metal -framework MetalKit
