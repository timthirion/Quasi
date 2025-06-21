#find_package(Dawn REQUIRED)
find_package(glfw3 REQUIRED)

project(test_app)

add_executable(test_app quasi.cpp)

target_link_libraries(test_app PRIVATE glfw)
