project(EctorTools C)

add_executable(SampleApp "sample_app.c")
target_compile_features(SampleApp PRIVATE c_std_11)
target_link_libraries(SampleApp PRIVATE Ector GLAD)

add_executable(SampleApp3D "sample_app_3d.c")
target_compile_features(SampleApp3D PRIVATE c_std_11)
target_link_libraries(SampleApp3D PRIVATE Ector GLAD)

add_executable(ScriptingSample "scripting_sample.c")
target_compile_features(ScriptingSample PRIVATE c_std_11)
target_include_directories(ScriptingSample PRIVATE "./")
target_link_libraries(ScriptingSample PRIVATE Ector STB)

add_executable(LightingTest "lighting_test.c")
target_compile_features(LightingTest PRIVATE c_std_11)
target_include_directories(LightingTest PRIVATE "./")
target_link_libraries(LightingTest PRIVATE Ector STB)
