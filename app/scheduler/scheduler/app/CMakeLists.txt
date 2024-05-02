#### APP ####
add_executable(app "main.cpp")
target_link_libraries(app common)
target_precompile_headers(app REUSE_FROM common)
set_target_properties(app PROPERTIES INTERPROCEDURAL_OPTIMIZATION $<CONFIG:Release,RelWithDebInfo>) # Enable LTO for release builds)
