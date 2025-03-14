project(EctorTools C)

add_executable(SampleApp "sample_app.c")
target_compile_features(SampleApp PRIVATE c_std_11)
target_link_libraries(SampleApp PRIVATE Ector GLAD)
