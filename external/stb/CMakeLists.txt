PROJECT(STB)

add_library(STB STATIC "impl.c")
target_compile_features(STB PRIVATE c_std_11)
target_include_directories(STB PUBLIC "lib/")
