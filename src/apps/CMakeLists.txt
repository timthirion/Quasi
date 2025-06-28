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

# Metal test app (macOS only)
if(APPLE)
    add_executable(test_metal test_metal.cpp)
    
    # Link Metal frameworks for test
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    
    if(METAL_FRAMEWORK AND METALKIT_FRAMEWORK AND FOUNDATION_FRAMEWORK)
        target_link_libraries(test_metal PRIVATE 
            ${METAL_FRAMEWORK} 
            ${METALKIT_FRAMEWORK} 
            ${FOUNDATION_FRAMEWORK}
        )
        
        # Enable Objective-C++ compilation for test
        set_source_files_properties(test_metal.cpp PROPERTIES
            COMPILE_FLAGS "-x objective-c++"
        )
    endif()
endif()

# Metal-accelerated raytracer app (macOS only)
if(APPLE)
    add_executable(rt_metal rt_metal.cpp)
    target_link_libraries(rt_metal PRIVATE quasi)
    target_compile_features(rt_metal PRIVATE cxx_std_23)
    
    # Link Metal frameworks
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    
    if(METAL_FRAMEWORK AND METALKIT_FRAMEWORK AND FOUNDATION_FRAMEWORK)
        target_link_libraries(rt_metal PRIVATE 
            ${METAL_FRAMEWORK} 
            ${METALKIT_FRAMEWORK} 
            ${FOUNDATION_FRAMEWORK}
        )
        
        # Enable Objective-C++ compilation
        set_source_files_properties(rt_metal.cpp PROPERTIES
            COMPILE_FLAGS "-x objective-c++"
        )
        
        # Copy Metal shader to build directory
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/raytracer.metal
            ${CMAKE_CURRENT_BINARY_DIR}/raytracer.metal
            COPYONLY
        )
        
        message(STATUS "Metal ray tracer enabled - rt_metal executable will be built")
    else()
        message(WARNING "Metal frameworks not found - rt_metal will not be built")
    endif()
else()
    message(STATUS "Metal ray tracer is only available on macOS")
endif()

# Metal Triangle App - build separately with: swiftc -o metal_triangle metal_triangle.swift -framework Cocoa -framework Metal -framework MetalKit
