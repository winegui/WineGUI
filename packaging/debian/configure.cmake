# Build-time input only. No production key is stored in this repository.
option(WINEGUI_APT_ENABLED "Embed and configure the WineGUI APT repository" OFF)
set(WINEGUI_APT_KEY_FILE "" CACHE FILEPATH "Binary or armored public OpenPGP export")
set(WINEGUI_APT_KEY_FINGERPRINTS "" CACHE STRING "Comma-separated full primary fingerprints")
set(WINEGUI_APT_KEY_GENERATION "" CACHE STRING "Monotonically increasing keyring generation")
set(WINEGUI_DEB_REBUILD "1" CACHE STRING "Debian packaging rebuild counter")
set(WINEGUI_OS_RELEASE "/etc/os-release" CACHE FILEPATH "Target distribution identity")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(_apt_enabled 0)
if(WINEGUI_APT_ENABLED)
    set(_apt_enabled 1)
endif()
set(_debian_output "${CMAKE_CURRENT_BINARY_DIR}/debian-control")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/package.py"
        --version "${PROJECT_VERSION}" --output "${_debian_output}"
        --os-release "${WINEGUI_OS_RELEASE}" --rebuild "${WINEGUI_DEB_REBUILD}"
        --enabled "${_apt_enabled}" --tag "$ENV{CI_COMMIT_TAG}"
        --key-file "${WINEGUI_APT_KEY_FILE}"
        --fingerprints "${WINEGUI_APT_KEY_FINGERPRINTS}"
        --generation "${WINEGUI_APT_KEY_GENERATION}"
    RESULT_VARIABLE _debian_result
    ERROR_VARIABLE _debian_error)
if(NOT _debian_result EQUAL 0)
    string(STRIP "${_debian_error}" _debian_error)
    message(FATAL_ERROR "WineGUI Debian metadata/key validation failed: ${_debian_error}")
endif()
file(READ "${_debian_output}/version.txt" CPACK_DEBIAN_PACKAGE_VERSION)
string(STRIP "${CPACK_DEBIAN_PACKAGE_VERSION}" CPACK_DEBIAN_PACKAGE_VERSION)
set(CPACK_DEBIAN_PACKAGE_NAME winegui)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${_debian_output}/postinst;${_debian_output}/postrm")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
if(WINEGUI_APT_ENABLED)
    string(APPEND CPACK_DEBIAN_PACKAGE_DEPENDS ", ca-certificates")
endif()
