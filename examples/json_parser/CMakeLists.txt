add_executable(example_json_parser)

target_sources(example_json_parser PRIVATE main.cpp)

target_compile_features(example_json_parser PRIVATE cxx_std_20)
target_link_libraries(example_json_parser PRIVATE parsi::parsi)
