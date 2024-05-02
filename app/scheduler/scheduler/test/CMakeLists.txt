# Prepare test library

# Add test cases files
set(TARGET_FMS_TEST_FILES "fms-test_files")
set(TEST_FILES_DIR "${PROJECT_SOURCE_DIR}/../benchmarks/test_problems")
set(TEST_FILES_DIR_OUT "${CMAKE_CURRENT_BINARY_DIR}/test_problems")

cmake_path(NORMAL_PATH TEST_FILES_DIR)
file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS "${TEST_FILES_DIR}/*.xml" "${TEST_FILES_DIR}/*.seq.json")

foreach(TEST_FILE ${TEST_FILES})
    cmake_path(RELATIVE_PATH TEST_FILE BASE_DIRECTORY ${TEST_FILES_DIR} OUTPUT_VARIABLE TEST_FILE_RELATIVE)
    cmake_path(GET TEST_FILE_RELATIVE PARENT_PATH TEST_FILE_RELATIVE_PARENT)
    list(APPEND TEST_FILES_OUT "${TEST_FILES_DIR_OUT}/${TEST_FILE_RELATIVE}")
    list(APPEND TEST_FILES_DIRS_OUT "${TEST_FILES_DIR_OUT}/${TEST_FILE_RELATIVE_PARENT}")
endforeach()
list(REMOVE_DUPLICATES TEST_FILES_DIRS_OUT)

add_custom_target(${TARGET_FMS_TEST_FILES} ALL
    COMMAND_EXPAND_LISTS
    COMMENT "Creating test case directories"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TEST_FILES_DIRS_OUT}"
)

foreach(TEST_FILE TEST_FILE_OUT IN ZIP_LISTS TEST_FILES TEST_FILES_OUT)
    add_custom_command(TARGET ${TARGET_FMS_TEST_FILES} POST_BUILD
        COMMENT "Copying test case ${TEST_FILE_OUT}"
        BYPRODUCTS "${TEST_FILE_OUT}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TEST_FILE}" "${TEST_FILE_OUT}"
    )
endforeach()

CPMAddPackage(
    GITHUB_REPOSITORY google/googletest
    VERSION 1.13.0
    OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
    EXCLUDE_FROM_ALL
    SYSTEM
)

set(TARGET_FMS_TEST "fms-test")

# Find test files
file(GLOB_RECURSE EXEC_TEST_SOURCES CONFIGURE_DEPENDS "*.cc" "*.cpp")
add_executable(${TARGET_FMS_TEST} ${EXEC_TEST_SOURCES})

target_include_directories(${TARGET_FMS_TEST} PRIVATE "./")
target_link_libraries(${TARGET_FMS_TEST} common GTest::gtest)
target_precompile_headers(${TARGET_FMS_TEST} REUSE_FROM common)

# Configure CTest compatibility with GoogleTest
include(GoogleTest)
gtest_discover_tests(
    ${TARGET_FMS_TEST}
    WORKING_DIRECTORY "${TEST_FILES_DIR_OUT}"
)

if (CODE_COVERAGE)
    include(ProcessorCount)
    ProcessorCount(PROCESSOR_COUNT)

    # From CMake version 3.21 we can output the coverage report in Junit XML format
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.21.0)
        message(STATUS "Adding Junit report flag")
        set(COVERAGE_EXTRA_FLAGS --output-junit report.xml)
    endif()

    message(STATUS "Configuring code coverage for ${TARGET_FMS_TEST}")
    setup_target_for_coverage_gcovr_html(
        NAME "${TARGET_FMS_TEST}_coverage"
        EXECUTABLE ctest -j ${PROCESSOR_COUNT} ${COVERAGE_EXTRA_FLAGS}
        DEPENDENCIES ${TARGET_FMS_TEST} ${TARGET_FMS_TEST_FILES}
        EXCLUDE "${CMAKE_BINARY_DIR}/*" "${PROJECT_SOURCE_DIR}/flowshopvis/*" "${PROJECT_SOURCE_DIR}/test/*" "${PROJECT_SOURCE_DIR}/app/*"
    )
endif()


