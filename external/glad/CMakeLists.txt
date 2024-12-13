project(glad)

add_library(GLAD STATIC "src/gl.c")
target_compile_features(GLAD PRIVATE c_std_11)
target_include_directories(GLAD PUBLIC "include/")
