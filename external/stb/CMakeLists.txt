PROJECT(STB)

add_library(STB STATIC "impl.c" "lib/stb_image.h")
target_compile_features(STB PRIVATE c_std_11)
target_include_directories(STB PUBLIC "lib/")
