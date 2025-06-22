add_library(geometry 
    geometry.cpp
    geometry.h
)

target_include_directories(geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})