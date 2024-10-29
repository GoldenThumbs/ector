add_subdirectory("stb")
add_subdirectory("sokol")
add_subdirectory("glfw")

add_library(external INTERFACE)
target_link_libraries(external INTERFACE
   STB
   SOKOL
   glfw
)
