# Template from: https://gitlab.com/CLIUtils/modern-cmake/-/blob/master/examples/extended-project/CMakeLists.txt

cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(
    fms-scheduler 
    VERSION 1.0 
    DESCRIPTION "Flexible Manufacturing System Scheduler"
    LANGUAGES CXX
    )

option(CODE_COVERAGE "Enable code coverage" OFF)
option(PROFILING "Enable profiling" OFF)
option(FMS_SCHEDULER_BUILD_TESTS "Build tests" OFF)
option(BUILD_TOOLS "Build tools" OFF)

set(DEFAULT_BUILD_TYPE "RelWithDebInfo")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Let's ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD 20)

    # Set output folders
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) 
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    if (MSVC)
        # CMake uses the /Zi flag when debugging which is incompatible with ccache
        string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    endif()

    # Let's nicely support folders in IDEs
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # Testing only available if this is the main app
    # Note this needs to be done in the main CMakeLists
    # since it calls enable_testing, which must be in the
    # main CMakeLists.
    include(CTest)

    find_package(Doxygen OPTIONAL_COMPONENTS dot)
    if (Doxygen_FOUND)
        add_subdirectory(doc)
    else()
        message(STATUS "Doxygen not found, not building docs")
    endif()
endif()

include(config/CPM.cmake)

### CODE COVERAGE ###
if(CODE_COVERAGE)
    include(config/CodeCoverage.cmake)
    append_coverage_compiler_flags()
endif()

### PROFILING ###
if (PROFILING)
    include(config/profiling.cmake)
    append_profile_compiler_flags()
endif()

add_subdirectory(src) # Common library
add_subdirectory(app)

if(BUILD_TOOLS)
    message(STATUS "Building tools")
    add_subdirectory(flowshopvis)
endif()

# Testing only available if this is the main app (BUILD_TESTING is defined when 
# including CTest) 
# Emergency override FMS_SCHEDULER_BUILD_TESTS provided as well
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING) OR FMS_SCHEDULER_BUILD_TESTS)
    add_subdirectory(test)
endif()
