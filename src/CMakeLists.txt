add_library(geometry 
    geometry/Vec3.cpp
    geometry/Vec3.h
    geometry/Ray.cpp
    geometry/Ray.h
    geometry/Triangle.cpp
    geometry/Triangle.h
    geometry/Sphere.cpp
    geometry/Sphere.h
    geometry/geometry.h
)

add_library(radiometry INTERFACE
    radiometry/Color.h
    radiometry/Camera.h
)

add_library(io INTERFACE
    io/PPMWriter.h
)

target_include_directories(geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(radiometry INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(io INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Set up library dependencies
target_link_libraries(radiometry INTERFACE geometry)
target_link_libraries(io INTERFACE radiometry)