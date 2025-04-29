#### APP ####
add_executable(fms-scheduler "main.cpp")
target_link_libraries(fms-scheduler fms-common)
target_precompile_headers(fms-scheduler REUSE_FROM fms-common)
set_target_properties(fms-scheduler PROPERTIES INTERPROCEDURAL_OPTIMIZATION $<CONFIG:Release,RelWithDebInfo>) # Enable LTO for release builds)
