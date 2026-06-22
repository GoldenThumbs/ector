add_subdirectory("glad")
add_subdirectory("glfw")
add_subdirectory("stb")
add_subdirectory("lua")

add_library(external INTERFACE)
target_link_libraries(external INTERFACE
   LUA
   STB
   GLAD
   glfw
)
