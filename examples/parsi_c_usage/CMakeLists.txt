project(example_parsi_c_usage C)

add_executable(parsi_c_usage main.c)
target_compile_features(parsi_c_usage PRIVATE c_std_11)
target_link_libraries(parsi_c_usage PRIVATE parsi::parsi-c)
