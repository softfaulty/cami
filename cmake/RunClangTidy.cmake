if(NOT DEFINED SOURCE_DIR OR NOT DEFINED BUILD_DIR OR NOT DEFINED CLANG_TIDY)
    message(FATAL_ERROR "SOURCE_DIR, BUILD_DIR and CLANG_TIDY are required")
endif()

file(
    GLOB_RECURSE tidy_files
    LIST_DIRECTORIES false
    "${SOURCE_DIR}/compiler/*.cc"
    "${SOURCE_DIR}/compiler/*.cpp"
    "${SOURCE_DIR}/compiler/*.cxx"
    "${SOURCE_DIR}/runtime/*.cc"
    "${SOURCE_DIR}/runtime/*.cpp"
    "${SOURCE_DIR}/runtime/*.cxx"
    "${SOURCE_DIR}/tests/*.cc"
    "${SOURCE_DIR}/tests/*.cpp"
    "${SOURCE_DIR}/tests/*.cxx"
)

if(NOT tidy_files)
    message(STATUS "clang-tidy: no C++ files yet")
    return()
endif()

foreach(tidy_file IN LISTS tidy_files)
    execute_process(
        COMMAND "${CLANG_TIDY}" -p "${BUILD_DIR}" "${tidy_file}"
        RESULT_VARIABLE tidy_result
    )

    if(NOT tidy_result EQUAL 0)
        message(FATAL_ERROR "clang-tidy failed for ${tidy_file}")
    endif()
endforeach()
