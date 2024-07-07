project(example_json_validator CXX)

add_executable(json_validator main.cpp)
target_compile_features(json_validator PRIVATE cxx_std_20)
target_link_libraries(json_validator PRIVATE parsi::parsi)
