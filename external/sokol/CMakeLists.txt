project(SOKOL)

set(SOKOL_ALL_APIS "DUMMY_BACKEND;GLCORE;GLES;D3D11;METAL;WGPU;")

list(GET SOKOL_ALL_APIS 1 SOKOL_API)
set(SOKOL_API ${SOKOL_API} CACHE STRING "")
set_property(CACHE SOKOL_API PROPERTY STRINGS ${SOKOL_ALL_APIS})

if(NOT SOKOL_API IN_LIST SOKOL_ALL_APIS)
   message(WARNING "Selected API \"${SOKOL_API}\" is not an available API (${SOKOL_ALL_APIS}), defaulting to ${DEFAULT_API}.")
   list(GET SOKOL_ALL_APIS 0 SOKOL_API)
   set(SOKOL_API ${SOKOL_API} CACHE STRING "")
endif()

add_library(SOKOL STATIC "impl.c")
target_compile_features(SOKOL PRIVATE c_std_11)
target_include_directories(SOKOL PUBLIC "lib/")
target_compile_definitions(SOKOL PUBLIC "SOKOL_${SOKOL_API}")

if(LINUX AND ${SOKOL_API} STREQUAL "GLCORE")
include("FindOpenGL")
target_link_libraries(SOKOL PRIVATE OpenGL::GL)
endif()
