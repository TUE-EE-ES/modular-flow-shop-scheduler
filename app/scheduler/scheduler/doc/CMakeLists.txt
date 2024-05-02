# Download doxygen-awesome-css
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
    cmake_policy(SET CMP0135 NEW)
endif()
include(ExternalProject)
ExternalProject_Add(doxygen-awesome-css
    URL "https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.2.1.tar.gz"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL YES
)
ExternalProject_Get_Property(doxygen-awesome-css SOURCE_DIR)
set(DOXYGEN_AWESOME_SOURCE_DIR "${SOURCE_DIR}")
    

# Documentation
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE  "${PROJECT_SOURCE_DIR}/../README.md")
set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_EXTRACT_PRIVATE YES)
set(DOXYGEN_USE_MATHJAX YES)
set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
set(DOXYGEN_EXCLUDE_PATTERNS "${PROJECT_SOURCE_DIR}/include/rapidxml/*")
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
set(DOXYGEN_CALLER_GRAPH YES)
set(DOXYGEN_CALL_GRAPH YES)
set(DOXYGEN_MARKDOWN_SUPPORT YES)
set(DOXYGEN_DOT_IMAGE_FORMAT "svg")
set(DOXYGEN_DOT_TRANSPARENT YES)
set(DOXYGEN_DOT_MULTI_TARGETS YES)
set(DOXYGEN_INTERACTIVE_SVG YES)
set(DOXYGEN_CREATE_SUBDIRS YES)
set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_SOURCE_BROWSER YES)
set(DOXYGEN_DISABLE_INDEX NO)
set(DOXYGEN_FULL_SIDEBAR NO)
set(DOXYGEN_NUM_PROC_THREADS 0)
set(DOXYGEN_TOC_INCLUDE_HEADINGS 5)
set(DOXYGEN_OUTPUT_LANGUAGE "English")
set(DOXYGEN_HTML_COLORSTYLE "LIGHT")
set(DOXYGEN_TAGFILES "tagfiles/cppreference-doxygen-web.tag.xml=https://en.cppreference.com/w/")
set(DOXYGEN_HTML_HEADER "doxygen/header.html")
set(DOXYGEN_HTML_EXTRA_FILES 
    "${DOXYGEN_AWESOME_SOURCE_DIR}/doxygen-awesome-fragment-copy-button.js" 
    "${DOXYGEN_AWESOME_SOURCE_DIR}/doxygen-awesome-darkmode-toggle.js" 
    "${DOXYGEN_AWESOME_SOURCE_DIR}/doxygen-awesome-interactive-toc.js")
set(DOXYGEN_HTML_EXTRA_STYLESHEET 
    "${DOXYGEN_AWESOME_SOURCE_DIR}/doxygen-awesome.css"
    "${DOXYGEN_AWESOME_SOURCE_DIR}/doxygen-awesome-sidebar-only.css"
    "${DOXYGEN_AWESOME_SOURCE_DIR}/doxygen-awesome-sidebar-only-darkmode-toggle.css")
doxygen_add_docs(doc
    "${PROJECT_SOURCE_DIR}/include" "${DOXYGEN_USE_MDFILE_AS_MAINPAGE}"
)
add_dependencies(doc doxygen-awesome-css)

