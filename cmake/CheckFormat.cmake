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
    "${SOURCE_DIR}/compiler/*.hpp.in"
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
    message(STATUS "format: no C/C++ files yet")
    return()
endif()

if(FORMAT_MODE STREQUAL "write")
    execute_process(COMMAND "${CLANG_FORMAT}" -i ${format_files} RESULT_VARIABLE format_result)
elseif(FORMAT_MODE STREQUAL "check")
    execute_process(
        COMMAND "${CLANG_FORMAT}" --dry-run --Werror ${format_files}
        RESULT_VARIABLE format_result
    )
else()
    message(FATAL_ERROR "FORMAT_MODE must be 'write' or 'check'")
endif()

if(NOT format_result EQUAL 0)
    message(FATAL_ERROR "clang-format failed in ${FORMAT_MODE} mode")
endif()
