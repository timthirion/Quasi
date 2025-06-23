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

target_include_directories(geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})