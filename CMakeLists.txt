cmake_minimum_required(VERSION 3.25)
project(makineai-texthook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Plugin DLL (loaded by launcher)
add_library(makineai-texthook SHARED
    src/plugin.cpp
    src/hook_manager.cpp
)

target_include_directories(makineai-texthook PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(makineai-texthook PRIVATE
    psapi
)

set_target_properties(makineai-texthook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "makineai-texthook"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/release"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/release"
)

# Hook DLL (injected into game process)
add_library(makineai-hook SHARED
    hook/dllmain.cpp
    hook/text_hooks.cpp
    hook/pipe_client.cpp
)

target_include_directories(makineai-hook PRIVATE
    ${CMAKE_SOURCE_DIR}/hook
)

target_link_libraries(makineai-hook PRIVATE
    gdi32
    user32
)

set_target_properties(makineai-hook PROPERTIES
    PREFIX ""
    OUTPUT_NAME "makineai-hook"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/release"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/release"
)

# Copy manifest to output
configure_file(
    ${CMAKE_SOURCE_DIR}/manifest.json
    ${CMAKE_BINARY_DIR}/release/manifest.json
    COPYONLY
)
