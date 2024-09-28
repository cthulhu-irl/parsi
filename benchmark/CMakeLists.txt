find_package(benchmark CONFIG REQUIRED)

add_library(${PROJECT_NAME}-benchmark-options INTERFACE)

target_link_libraries(${PROJECT_NAME}-benchmark-options
    INTERFACE
        ${PROJECT_NAME}
        ${PROJECT_NAME}-options
        ${PROJECT_NAME}::parsi-c
        benchmark::benchmark
        benchmark::benchmark_main)

add_executable(${PROJECT_NAME}-bench-core core.cpp)
target_link_libraries(${PROJECT_NAME}-bench-core PRIVATE ${PROJECT_NAME}-benchmark-options)
