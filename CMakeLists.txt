cmake_minimum_required(VERSION 3.25)

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
endif()
