# Create unified Quasi library
add_library(quasi 
    # Geometry module
    geometry/vec3.cpp
    geometry/vec3.hpp
    geometry/ray.cpp
    geometry/ray.hpp
    geometry/triangle.cpp
    geometry/triangle.hpp
    geometry/sphere.cpp
    geometry/sphere.hpp
    geometry/box.cpp
    geometry/box.hpp
    geometry/reflection.hpp
    geometry/geometry.hpp
    
    # Radiometry module (header-only)
    radiometry/color.hpp
    radiometry/camera.hpp
    
    # IO module
    io/ppm_writer.hpp
    io/scene_parser.hpp
    io/scene_parser.cpp
    
    # Materials module
    materials/texture.hpp
    materials/checkerboard_texture.hpp
    materials/material.hpp
    
    # Lighting module
    lighting/light.hpp
    lighting/phong.hpp
    lighting/phong.cpp
    
    # Scene module
    scene/scene.hpp
    scene/scene.cpp
    scene/ray_tracer.hpp
    scene/ray_tracer.cpp
    
    # Sampling module
    sampling/sample_pattern.hpp
    sampling/sample_pattern.cpp
    sampling/sample_integrator.hpp
    sampling/sample_integrator.cpp
    sampling/stratified_pattern.hpp
    sampling/stratified_pattern.cpp
    sampling/average_integrator.hpp
    sampling/average_integrator.cpp
)

# Create include directory structure in build directory
set(QUASI_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi)

# Copy headers to build include directory (much cleaner than individual commands)
add_custom_target(copy_headers ALL
    # Copy all .hpp files while preserving directory structure
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${QUASI_INCLUDE_DIR}/quasi
    # Remove copied .cpp files - we only want headers in include directory  
    COMMAND ${CMAKE_COMMAND} -E rm -f ${QUASI_INCLUDE_DIR}/quasi/*/*.cpp
    COMMENT "Copying header files to build include directory"
)

add_dependencies(quasi copy_headers)

# Set up include directories for the library
target_include_directories(quasi 
    PUBLIC 
        $<BUILD_INTERFACE:${QUASI_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Link nlohmann/json (header-only library)
target_link_libraries(quasi PUBLIC nlohmann_json::nlohmann_json)

# Install headers with proper directory structure
install(DIRECTORY geometry/ DESTINATION include/quasi/geometry FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY radiometry/ DESTINATION include/quasi/radiometry FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY io/ DESTINATION include/quasi/io FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY materials/ DESTINATION include/quasi/materials FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY lighting/ DESTINATION include/quasi/lighting FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY scene/ DESTINATION include/quasi/scene FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY sampling/ DESTINATION include/quasi/sampling FILES_MATCHING PATTERN "*.hpp")

# Install the library
install(TARGETS quasi
    EXPORT QuasiTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Export targets
install(EXPORT QuasiTargets
    FILE QuasiTargets.cmake
    NAMESPACE Quasi::
    DESTINATION lib/cmake/Quasi
)
