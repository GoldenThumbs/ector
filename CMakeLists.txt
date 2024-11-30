cmake_minimum_required(VERSION 3.14)

project(Ector C)
set(PROJECT_VERSION 0.0.01)
set(API_VERSION 0001)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin")

set(ECTOR_SOURCE
   "src/core/engine.c"
   "src/core/renderer.c"
)

add_subdirectory("external/")

add_library(Ector STATIC ${ECTOR_SOURCE})
target_compile_features(Ector PRIVATE c_std_11)
target_include_directories(Ector
    PUBLIC
        "inc/"
    PRIVATE
        "src/"
)
target_link_libraries(Ector PUBLIC external)

add_subdirectory("tools")
