if(NOT DEFINED SOURCE_DIR OR NOT DEFINED CLANG_FORMAT)
    message(FATAL_ERROR "SOURCE_DIR and CLANG_FORMAT are required")
endif()

file(
    GLOB_RECURSE format_files
    LIST_DIRECTORIES false
    "${SOURCE_DIR}/compiler/*.c"
    "${SOURCE_DIR}/compiler/*.cc"
    "${SOURCE_DIR}/compiler/*.cpp"
    "${SOURCE_DIR}/compiler/*.cxx"
    "${SOURCE_DIR}/compiler/*.h"
    "${SOURCE_DIR}/compiler/*.hh"
    "${SOURCE_DIR}/compiler/*.hpp"
    "${SOURCE_DIR}/runtime/*.c"
    "${SOURCE_DIR}/runtime/*.cc"
    "${SOURCE_DIR}/runtime/*.cpp"
    "${SOURCE_DIR}/runtime/*.cxx"
    "${SOURCE_DIR}/runtime/*.h"
    "${SOURCE_DIR}/runtime/*.hh"
    "${SOURCE_DIR}/runtime/*.hpp"
    "${SOURCE_DIR}/tests/*.c"
    "${SOURCE_DIR}/tests/*.cc"
    "${SOURCE_DIR}/tests/*.cpp"
    "${SOURCE_DIR}/tests/*.cxx"
    "${SOURCE_DIR}/tests/*.h"
    "${SOURCE_DIR}/tests/*.hh"
    "${SOURCE_DIR}/tests/*.hpp"
)

if(NOT format_files)
    message(STATUS "format-check: no C/C++ files yet")
    return()
endif()

execute_process(
    COMMAND "${CLANG_FORMAT}" --dry-run --Werror ${format_files}
    RESULT_VARIABLE format_result
)

if(NOT format_result EQUAL 0)
    message(FATAL_ERROR "clang-format found files that need formatting")
endif()
