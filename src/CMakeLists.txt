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
    geometry/geometry.hpp
    
    # Radiometry module (header-only)
    radiometry/color.hpp
    radiometry/camera.hpp
    
    # IO module (header-only)
    io/ppm_writer.hpp
    
    # Materials module (header-only)
    materials/texture.hpp
    materials/checkerboard_texture.hpp
)

# Create include directory structure in build directory
set(QUASI_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/geometry)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/radiometry)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/io)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/materials)

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
)

add_dependencies(quasi copy_headers)

# Set up include directories for the library
target_include_directories(quasi 
    PUBLIC 
        $<BUILD_INTERFACE:${QUASI_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Install headers with proper directory structure
install(DIRECTORY geometry/ DESTINATION include/quasi/geometry FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY radiometry/ DESTINATION include/quasi/radiometry FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY io/ DESTINATION include/quasi/io FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY materials/ DESTINATION include/quasi/materials FILES_MATCHING PATTERN "*.hpp")

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
