find_package(Catch2 3 REQUIRED)

add_executable(quasi_tests 
    test_basic.cpp
    test_ray_triangle_intersection.cpp
    test_ray_sphere_intersection.cpp
    test_checkerboard_texture.cpp
    test_blue_noise_pattern.cpp
    test_adaptive_integrator.cpp
)
target_link_libraries(quasi_tests PRIVATE 
    Catch2::Catch2WithMain
    quasi
)
target_compile_features(quasi_tests PRIVATE cxx_std_23)

message(STATUS "CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")

include(CTest)
include(Catch)
catch_discover_tests(quasi_tests)
