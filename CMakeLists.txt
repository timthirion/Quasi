cmake_minimum_required(VERSION 3.14.1)

project(quasi)

option(QUASI_USE_SUPERBUILD "Build everything" ON)
if (QUASI_USE_SUPERBUILD)
  include(Superbuild.cmake)
  return()
endif()

set(CMAKE_CXX_STANDARD 23)

# Find nlohmann/json
find_package(nlohmann_json REQUIRED)

# Build geometry library
add_subdirectory(src)

# Build all apps
add_subdirectory(src/apps)

# Build tests
enable_testing()
add_subdirectory(tests)
