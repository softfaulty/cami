cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_APPLE AND NOT CMAKE_OSX_SYSROOT)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(CMAKE_OSX_SYSROOT "${CMAKE_OSX_SYSROOT}" CACHE PATH "macOS SDK" FORCE)
endif()

project(cami VERSION 0.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM CONFIG QUIET)

if(NOT LLVM_FOUND)
    find_program(
        LLVM_CONFIG_EXECUTABLE
        NAMES llvm-config
        HINTS
            /opt/homebrew/opt/llvm/bin
            /usr/local/opt/llvm/bin
    )

    if(LLVM_CONFIG_EXECUTABLE)
        execute_process(
            COMMAND "${LLVM_CONFIG_EXECUTABLE}" --cmakedir
            OUTPUT_VARIABLE LLVM_CMAKE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        list(PREPEND CMAKE_PREFIX_PATH "${LLVM_CMAKE_DIR}")
    endif()

    find_package(LLVM CONFIG REQUIRED)
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} in ${LLVM_DIR}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(
    CAMI_CLANG_FORMAT
    NAMES clang-format
    HINTS "${LLVM_TOOLS_BINARY_DIR}"
    REQUIRED
)
find_program(
    CAMI_CLANG_TIDY
    NAMES clang-tidy
    HINTS "${LLVM_TOOLS_BINARY_DIR}"
    REQUIRED
)

add_custom_target(
    format-check
    COMMAND
        "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DCLANG_FORMAT=${CAMI_CLANG_FORMAT}"
        -P "${PROJECT_SOURCE_DIR}/cmake/CheckFormat.cmake"
    VERBATIM
)

add_custom_target(
    clang-tidy
    COMMAND
        "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
        "-DCLANG_TIDY=${CAMI_CLANG_TIDY}"
        -P "${PROJECT_SOURCE_DIR}/cmake/RunClangTidy.cmake"
    VERBATIM
)

configure_file(
    "${PROJECT_SOURCE_DIR}/compiler/include/cami/version.hpp.in"
    "${PROJECT_BINARY_DIR}/generated/cami/version.hpp"
    @ONLY
)

add_library(cami_compiler STATIC compiler/lib/cli.cpp)
target_include_directories(
    cami_compiler
    PUBLIC "${PROJECT_SOURCE_DIR}/compiler/include"
    PRIVATE "${PROJECT_BINARY_DIR}/generated"
)

add_executable(cami compiler/main.cpp)
target_link_libraries(cami PRIVATE cami_compiler)

function(cami_enable_warnings target)
    if(MSVC)
        target_compile_options("${target}" PRIVATE /W4 /permissive-)
    else()
        target_compile_options("${target}" PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

cami_enable_warnings(cami_compiler)
cami_enable_warnings(cami)

include(CTest)

if(BUILD_TESTING)
    add_test(
        NAME bootstrap.configuration
        COMMAND
            "${CMAKE_COMMAND}"
            "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
            "-DGENERATOR=${CMAKE_GENERATOR}"
            "-DCXX_STANDARD=${CMAKE_CXX_STANDARD}"
            "-DLLVM_VERSION=${LLVM_PACKAGE_VERSION}"
            "-DCLANG_FORMAT=${CAMI_CLANG_FORMAT}"
            "-DCLANG_TIDY=${CAMI_CLANG_TIDY}"
            -P "${PROJECT_SOURCE_DIR}/cmake/VerifyBootstrap.cmake"
    )

    add_executable(cami_cli_tests tests/cli_test.cpp)
    target_link_libraries(cami_cli_tests PRIVATE cami_compiler)
    cami_enable_warnings(cami_cli_tests)

    add_test(NAME cli.unit COMMAND cami_cli_tests)
    add_test(NAME cli.version COMMAND cami version)
    set_tests_properties(
        cli.version
        PROPERTIES
            PASS_REGULAR_EXPRESSION
                "cami compiler [0-9]+\\.[0-9]+\\.[0-9]+.*cami language 0\\.1.*package manifest 1.*LLVM [0-9]+"
    )
endif()
