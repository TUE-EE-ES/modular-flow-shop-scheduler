
#### DEPENDENCIES ####
CPMAddPackage(
    GITHUB_REPOSITORY jarro2783/cxxopts
    VERSION 3.1.1
    OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
    EXCLUDE_FROM_ALL
    SYSTEM
)
CPMAddPackage("gh:fmtlib/fmt#11.1.3")
CPMAddPackage("gh:nlohmann/json@3.11.3")
CPMAddPackage("gh:microsoft/GSL@4.0.0")
CPMAddPackage("gh:martinus/unordered_dense@4.4.0")

# Create version file
set(VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/versioning.cpp")
add_custom_target(Version ALL
    COMMAND "${CMAKE_COMMAND}" -DVERSION_FILE_NAME="${VERSION_FILE}" -P "${PROJECT_SOURCE_DIR}/config/revision.cmake"
    BYPRODUCTS "${VERSION_FILE}"
    COMMENT "Generating version file"
)

# Find source files to compile
file(GLOB_RECURSE LIB_COMMON_HEADERS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.hpp" "${PROJECT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE LIB_COMMON_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Define common library
add_library(fms-common STATIC EXCLUDE_FROM_ALL ${LIB_COMMON_SOURCES} ${VERSION_FILE} ${LIB_COMMON_HEADERS})

target_include_directories(fms-common PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(fms-common PUBLIC cxxopts fmt nlohmann_json::nlohmann_json unordered_dense::unordered_dense PRIVATE Microsoft.GSL::GSL)
target_compile_features(fms-common PUBLIC cxx_std_20)
file(GLOB_RECURSE LIB_COMMON_PRECOMPILED_HEADERS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/fms/pch/*.hpp")
target_precompile_headers(fms-common PUBLIC ${LIB_COMMON_PRECOMPILED_HEADERS})

set_target_properties(fms-common PROPERTIES 
    # Enable LTO for release builds
    INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE
    INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE 
)

# IDEs should put the headers in a nice place
source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${LIB_COMMON_HEADERS}
)

# IDEs should put the sources in a nice place
source_group(
    TREE "${PROJECT_SOURCE_DIR}/src"
    PREFIX "Source Files"
    FILES ${LIB_COMMON_SOURCES}
)
