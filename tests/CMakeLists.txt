find_package(catch2 CONFIG REQUIRED)

add_executable(${PROJECT_NAME}-tests)

target_sources(${PROJECT_NAME}-tests PRIVATE sample.cpp)

target_link_libraries(${PROJECT_NAME}-tests
    PRIVATE
        ${PROJECT_NAME}
        ${PROJECT_NAME}-options
        Catch2::Catch2
        Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

catch_discover_tests(${PROJECT_NAME}-tests)

