
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROFILE_FLAGS "-pg")
endif()

function(target_set_profiling)
    set(options "")
    set(oneValuedArgs "")
    set(multiValuedArgs TARGETS)
    cmake_parse_arguments(PROFILE "${options}" "${oneValuedArgs}" "${multiValuedArgs}" "${ARGN}")

    foreach(TARGET ${PROFILE_TARGETS})
        target_compile_options(${TARGET} PRIVATE "${PROFILE_FLAGS}")
        target_link_options(${TARGET} PRIVATE "${PROFILE_FLAGS}")
    endforeach()

endfunction()

function(append_profile_compiler_flags)
    add_compile_options("${PROFILE_FLAGS}")
    add_link_options("${PROFILE_FLAGS}")
endfunction()
