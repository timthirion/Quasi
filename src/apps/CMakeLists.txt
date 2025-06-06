find_package(glfw3 REQUIRED)

add_executable(test quasi.cpp)

target_link_libraries(test PRIVATE glfw)
