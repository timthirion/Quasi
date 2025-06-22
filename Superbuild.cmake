cmake_minimum_required(VERSION 3.14.1)

project(quasi-superbuild)

include(ExternalProject)

set(QUASI_CMAKE_DEFINITIONS "")

include(3rdparty/3rdparty.cmake)

ExternalProject_Add(quasi
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.
  BINARY_DIR ${CMAKE_BINARY_DIR}/quasi-build
  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  INSTALL_COMMAND ""
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DQUASI_USE_SUPERBUILD:BOOL=OFF
    ${QUASI_CMAKE_DEFINITIONS}
    ${CMAKE_MODULE_PATH}
  DEPENDS
    glfw
    catch2
)
