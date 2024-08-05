# From: https://stackoverflow.com/a/60856725/4005637

find_package(Qt6 COMPONENTS Core REQUIRED)

# get absolute path to qmake, then use it to find windeployqt executable

get_target_property(_qmake_executable Qt::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

function(windeployqt target)

    if (WIN32)

        # POST_BUILD step
        # - after build, we have a bin/lib for analyzing qt dependencies
        # - we run windeployqt on target and deploy Qt libs
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${_qt_bin_dir}/windeployqt.exe"
                --verbose 1
                $<IF:$<CONFIG:Debug>,--debug,--release>
                \"$<TARGET_FILE:${target}>\"
                COMMENT "Deploying Qt libraries using windeployqt for compilation target '${target}' ..."
                )

    endif ()

endfunction()
