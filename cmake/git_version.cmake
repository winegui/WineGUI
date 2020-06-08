find_package(Git QUIET REQUIRED)
if(GIT_FOUND)
    set_property(GLOBAL APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/.git/index")

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --always HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE GIT_HEAD_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    # For debug reasons use:
    #    message("GIT_HEAD_TAG = ${GIT_HEAD_TAG}")

    if("${GIT_HEAD_TAG}" MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+).*$")
        string(REGEX REPLACE "^v([0-9]+)\\.([0-9]+)\\.([0-9]+).*$"
        "\\1;\\2;\\3" _ver_parts "${GIT_HEAD_TAG}")
        list(GET _ver_parts 0 GIT_TAG_VERSION_MAJOR)
        list(GET _ver_parts 1 GIT_TAG_VERSION_MINOR)
        list(GET _ver_parts 2 GIT_TAG_VERSION_PATCH)
    else()
        set(GIT_TAG_VERSION_MAJOR "0")
        set(GIT_TAG_VERSION_MINOR "0")
        set(GIT_TAG_VERSION_PATCH "0")
    endif()

    set(GIT_TAG_VERSION "${GIT_TAG_VERSION_MAJOR}.${GIT_TAG_VERSION_MINOR}.${GIT_TAG_VERSION_PATCH}")
else(GIT_FOUND)
    message("GIT needs to be installed to generate GIT versioning.")
endif(GIT_FOUND)