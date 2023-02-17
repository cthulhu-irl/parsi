cmake_minimum_required(VERSION 3.20)

if (NOT DEFINED PARSI_MAIN_PROJECT)
    set(PARSI_MAIN_PROJECT OFF)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(PARSI_MAIN_PROJECT ON)
        message(STATUS "CMake version: ${CMAKE_VERSION}")
    endif()
endif()

project(parsi
    DESCRIPTION "lightweight simple parser combinator"
    LANGUAGES CXX)

option(PARSI_TESTS "build tests" ${PARSI_MAIN_PROJECT})
option(PARSI_DOCS "build docs" ${PARSI_MAIN_PROJECT})
option(PARSI_EXAMPLES "build examples" ${PARSI_MAIN_PROJECT})
option(PARSI_BENCHMARK "build benchmarks" ${PARSI_MAIN_PROJECT})
option(PARSI_INSTALL "generate install configs" ${PARSI_MAIN_PROJECT})

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

if (NOT DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

add_library(${PROJECT_NAME}-options INTERFACE)
target_compile_features(${PROJECT_NAME}-options INTERFACE cxx_std_20)
target_include_directories(${PROJECT_NAME}-options
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

# TODO decide whether to go header-only or not.
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} INTERFACE ${PROJECT_NAME}-options)

if (PARSI_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (PARSI_DOCS)
    add_subdirectory(docs)
endif()

if (PARSI_EXAMPLES)
    add_subdirectory(examples)
endif()

if (PARSI_BENCHMARK)
    add_subdirectory(benchmark)
endif()

if (PARSI_INSTALL)
    # TODO
endif()

