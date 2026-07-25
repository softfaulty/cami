if(NOT GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "expected Ninja, got ${GENERATOR}")
endif()

if(NOT CXX_STANDARD STREQUAL "20")
    message(FATAL_ERROR "expected C++20, got C++${CXX_STANDARD}")
endif()

if(NOT LLVM_VERSION)
    message(FATAL_ERROR "LLVM was not discovered")
endif()

if(NOT EXISTS "${BUILD_DIR}/build.ninja")
    message(FATAL_ERROR "Ninja build file was not generated")
endif()

if(NOT EXISTS "${CLANG_FORMAT}")
    message(FATAL_ERROR "clang-format was not discovered")
endif()

if(NOT EXISTS "${CLANG_TIDY}")
    message(FATAL_ERROR "clang-tidy was not discovered")
endif()
