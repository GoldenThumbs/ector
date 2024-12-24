add_subdirectory("glad")
add_subdirectory("glfw")

add_library(external INTERFACE)
target_link_libraries(external INTERFACE
   GLAD
   glfw
)
