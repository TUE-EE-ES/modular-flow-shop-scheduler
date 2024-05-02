
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)

# If using Qt6 we need to add the OpenGLWidgets module
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets)
    set(QT_LIBRARIES Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
    set(QT_LIBRARIES Qt${QT_VERSION_MAJOR}::Widgets)
endif()


set(EXEC_VIS flowshopvis)
file(GLOB_RECURSE EXEC_VIS_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE EXEC_VIS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
file(GLOB_RECURSE EXEC_VIS_RESOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")

add_executable(${EXEC_VIS} ${EXEC_VIS_SOURCES} ${EXEC_VIS_HEADERS} ${EXEC_VIS_RESOURCES})
target_link_libraries(${EXEC_VIS} PUBLIC ${QT_LIBRARIES} common PRIVATE Microsoft.GSL::GSL)
target_compile_definitions(${EXEC_VIS} PRIVATE QT_DEPRECATED_WARNINGS)
target_include_directories(${EXEC_VIS} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${EXEC_VIS} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    INTERPROCEDURAL_OPTIMIZATION $<CONFIG:Release,RelWithDebInfo> # Enable LTO for release builds
    WIN32_EXECUTABLE TRUE
)

# IDEs should put the headers in a nice place
source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "Header Files"
    FILES ${EXEC_VIS_HEADERS}
)

# IDEs should put the sources in a nice place
source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "Source Files"
    FILES ${EXEC_VIS_SOURCES}
)


# Deploying on windows requires adding extra DLLs
if(WIN32)
    include(../config/windeployqt.cmake)
    windeployqt(${EXEC_VIS})
endif()
