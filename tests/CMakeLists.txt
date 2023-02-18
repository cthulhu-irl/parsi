find_package(Catch2 CONFIG REQUIRED)

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

if (PARSI_ENABLE_COVERAGE)
    find_program(GCOVR_PROGRAM gcovr REQUIRED)

    set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")

    add_custom_target(
        coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}
        COMMAND ctest
        COMMAND ${GCOVR_PROGRAM} -r "${CMAKE_SOURCE_DIR}/src"
                                 --print-summary
                                 --fail-under-line 70
                                 --fail-under-branch 70
                                 --html "${COVERAGE_OUTPUT_DIR}/index.html" --html-details
                                 --xml "${COVERAGE_OUTPUT_DIR}/coverage.xml"
                                 --json "${COVERAGE_OUTPUT_DIR}/coverage.json"
                                 --object-directory=${PROJECT_BINARY_DIR}
        DEPENDS ${PROJECT_NAME}-tests
        BYPRODUCTS ${COVERAGE_OUTPUT_DIR}
        VERBATIM
    )
endif()

