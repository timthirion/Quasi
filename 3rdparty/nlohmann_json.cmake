# nlohmann/json library configuration
# Header-only library, so we just need to add it as an interface target

ExternalProject_Add(nlohmann_json
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann_json
  BINARY_DIR ${CMAKE_BINARY_DIR}/nlohmann_json-build
  INSTALL_DIR ${CMAKE_BINARY_DIR}/nlohmann_json-install
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/nlohmann_json-install
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DJSON_BuildTests=OFF
    -DJSON_Install=ON
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install
)

# Add to the definitions that will be passed to the main build
list(APPEND QUASI_CMAKE_DEFINITIONS
  "-Dnlohmann_json_DIR=${CMAKE_BINARY_DIR}/nlohmann_json-install/share/cmake/nlohmann_json"
)