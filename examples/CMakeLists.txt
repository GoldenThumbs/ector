project(EctorTools C)

add_executable(SampleApp "sample_app.c")
target_compile_features(SampleApp PRIVATE c_std_11)
target_link_libraries(SampleApp PRIVATE Ector GLAD)

add_executable(LightingTest "lighting_test.c")
target_compile_features(LightingTest PRIVATE c_std_11)
target_include_directories(LightingTest PRIVATE "./")
target_link_libraries(LightingTest PRIVATE Ector GLAD)
