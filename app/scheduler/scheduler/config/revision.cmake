execute_process(COMMAND git rev-parse --abbrev-ref HEAD
                OUTPUT_VARIABLE GIT_BRANCH)

execute_process(COMMAND git rev-list HEAD --count
                OUTPUT_VARIABLE GIT_COUNT
                ERROR_QUIET)
execute_process(COMMAND git log -n 1 "--format=%H Commited at %cd"
                OUTPUT_VARIABLE GIT_REV_DATE
                ERROR_QUIET)

string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
string(STRIP "${GIT_COUNT}" GIT_COUNT)
string(STRIP "${GIT_REV_DATE}" GIT_REV_DATE)

set(VERSION 
"#include \"fms/versioning.hpp\"

constexpr const std::string_view VERSION = \"${GIT_BRANCH}-${GIT_COUNT}-${GIT_REV_DATE}\";
")

if(EXISTS ${VERSION_FILE_NAME})
    file(READ ${VERSION_FILE_NAME} VERSION_)
else()
    set(VERSION_ "")
endif()

if (NOT "${VERSION}" STREQUAL "${VERSION_}")
    file(WRITE ${VERSION_FILE_NAME} "${VERSION}")
endif()

