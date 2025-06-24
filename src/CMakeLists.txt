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
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/geometry)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/radiometry)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/io)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/materials)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/lighting)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/scene)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/sampling)

# Create custom targets to copy and transform headers
add_custom_target(copy_headers
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/geometry.hpp ${QUASI_INCLUDE_DIR}/quasi/geometry/geometry.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/vec3.hpp ${QUASI_INCLUDE_DIR}/quasi/geometry/vec3.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/ray.hpp ${QUASI_INCLUDE_DIR}/quasi/geometry/ray.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/triangle.hpp ${QUASI_INCLUDE_DIR}/quasi/geometry/triangle.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/sphere.hpp ${QUASI_INCLUDE_DIR}/quasi/geometry/sphere.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/radiometry/color.hpp ${QUASI_INCLUDE_DIR}/quasi/radiometry/color.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/io/ppm_writer.hpp ${QUASI_INCLUDE_DIR}/quasi/io/ppm_writer.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/radiometry/camera.hpp ${QUASI_INCLUDE_DIR}/quasi/radiometry/camera.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/materials/texture.hpp ${QUASI_INCLUDE_DIR}/quasi/materials/texture.hpp  
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/materials/checkerboard_texture.hpp ${QUASI_INCLUDE_DIR}/quasi/materials/checkerboard_texture.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/materials/material.hpp ${QUASI_INCLUDE_DIR}/quasi/materials/material.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/box.hpp ${QUASI_INCLUDE_DIR}/quasi/geometry/box.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lighting/light.hpp ${QUASI_INCLUDE_DIR}/quasi/lighting/light.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lighting/phong.hpp ${QUASI_INCLUDE_DIR}/quasi/lighting/phong.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/io/scene_parser.hpp ${QUASI_INCLUDE_DIR}/quasi/io/scene_parser.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/scene/scene.hpp ${QUASI_INCLUDE_DIR}/quasi/scene/scene.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sampling/sample_pattern.hpp ${QUASI_INCLUDE_DIR}/quasi/sampling/sample_pattern.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sampling/sample_integrator.hpp ${QUASI_INCLUDE_DIR}/quasi/sampling/sample_integrator.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sampling/stratified_pattern.hpp ${QUASI_INCLUDE_DIR}/quasi/sampling/stratified_pattern.hpp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/sampling/average_integrator.hpp ${QUASI_INCLUDE_DIR}/quasi/sampling/average_integrator.hpp
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
