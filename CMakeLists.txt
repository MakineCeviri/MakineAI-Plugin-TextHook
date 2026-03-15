cmake_minimum_required(VERSION 3.25)
project(makineai-texthook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

add_library(makineai-texthook SHARED
    src/plugin.cpp
    src/hook_core.cpp
)

target_include_directories(makineai-texthook PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(makineai-texthook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "makineai-texthook"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/release"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/release"
)

configure_file(
    ${CMAKE_SOURCE_DIR}/manifest.json
    ${CMAKE_BINARY_DIR}/release/manifest.json
    COPYONLY
)
