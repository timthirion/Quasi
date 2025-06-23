# Create unified Quasi library
add_library(quasi 
    # Geometry module
    geometry/Vec3.cpp
    geometry/Vec3.h
    geometry/Ray.cpp
    geometry/Ray.h
    geometry/Triangle.cpp
    geometry/Triangle.h
    geometry/Sphere.cpp
    geometry/Sphere.h
    geometry/geometry.h
    
    # Radiometry module (header-only)
    radiometry/Color.h
    radiometry/Camera.h
    
    # IO module (header-only)
    io/PPMWriter.h
    
    # Materials module (header-only)
    materials/Texture.h
    materials/CheckerboardTexture.h
)

# Create include directory structure in build directory
set(QUASI_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/geometry)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/radiometry)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/io)
file(MAKE_DIRECTORY ${QUASI_INCLUDE_DIR}/quasi/materials)

# Create custom targets to copy and transform headers
add_custom_target(copy_headers
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/geometry.h ${QUASI_INCLUDE_DIR}/quasi/geometry/geometry.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/Vec3.h ${QUASI_INCLUDE_DIR}/quasi/geometry/Vec3.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/Ray.h ${QUASI_INCLUDE_DIR}/quasi/geometry/Ray.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/Triangle.h ${QUASI_INCLUDE_DIR}/quasi/geometry/Triangle.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/geometry/Sphere.h ${QUASI_INCLUDE_DIR}/quasi/geometry/Sphere.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/radiometry/Color.h ${QUASI_INCLUDE_DIR}/quasi/radiometry/Color.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/io/PPMWriter.h ${QUASI_INCLUDE_DIR}/quasi/io/PPMWriter.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/radiometry/Camera.h ${QUASI_INCLUDE_DIR}/quasi/radiometry/Camera.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/materials/Texture.h ${QUASI_INCLUDE_DIR}/quasi/materials/Texture.h  
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/materials/CheckerboardTexture.h ${QUASI_INCLUDE_DIR}/quasi/materials/CheckerboardTexture.h
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
install(DIRECTORY geometry/ DESTINATION include/quasi/geometry FILES_MATCHING PATTERN "*.h")
install(DIRECTORY radiometry/ DESTINATION include/quasi/radiometry FILES_MATCHING PATTERN "*.h")
install(DIRECTORY io/ DESTINATION include/quasi/io FILES_MATCHING PATTERN "*.h")
install(DIRECTORY materials/ DESTINATION include/quasi/materials FILES_MATCHING PATTERN "*.h")

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