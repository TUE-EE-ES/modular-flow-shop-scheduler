# CMakeLists.txt for flowshopvis

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

set(EXEC_VIS flowshopvis)
set(EXEC_VIS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB_RECURSE EXEC_VIS_HEADERS CONFIGURE_DEPENDS "${EXEC_VIS_SRC}/*.hpp" "${EXEC_VIS_SRC}/*.h")
file(GLOB_RECURSE EXEC_VIS_SOURCES CONFIGURE_DEPENDS "${EXEC_VIS_SRC}/*.cpp" "${EXEC_VIS_SRC}/*.ui")
file(GLOB_RECURSE EXEC_VIS_RESOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qrc")

add_executable(${EXEC_VIS} ${EXEC_VIS_SOURCES} ${EXEC_VIS_HEADERS} ${EXEC_VIS_RESOURCES})
target_link_libraries(${EXEC_VIS} PUBLIC Qt::Widgets Qt::OpenGLWidgets fms-common PRIVATE Microsoft.GSL::GSL)
target_compile_definitions(${EXEC_VIS} PRIVATE QT_DEPRECATED_WARNINGS)
target_include_directories(${EXEC_VIS} PRIVATE ${EXEC_VIS_SRC})
set_target_properties(${EXEC_VIS} PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
        INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
        WIN32_EXECUTABLE TRUE
)

# IDEs should put the headers in a nice place
source_group(
        TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
        PREFIX "Header Files"
        FILES ${EXEC_VIS_HEADERS}
)

# IDEs should put the sources in a nice place
source_group(
        TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
        PREFIX "Source Files"
        FILES ${EXEC_VIS_SOURCES}
)

# Deploying on windows requires adding extra DLLs
if (WIN32)
    include(../config/windeployqt.cmake)
    windeployqt(${EXEC_VIS})
endif ()

