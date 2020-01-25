
# Example: https://github.com/MariaDB/server/tree/10.5/cmake
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WineGUI is a user-friendly WINE graphical interface")
set(CPACK_PACKAGE_VENDOR "Melroy van den Berg")
set(CPACK_PACKAGE_CONTACT "Melroy van den Berg <melroy@melroy.org>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://winegui.melroy.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_TARGET}-${CPACK_PACKAGE_VERSION}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_RPM_PACKAGE_GROUP      "Applications/Productivity")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-v${CPACK_PACKAGE_VERSION}") # Without '-Linux' suffix

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND EXISTS "/etc/os-release")
    execute_process (
        COMMAND grep "^NAME=" /etc/os-release
        COMMAND sed -e "s/NAME=//g"
        COMMAND sed -e "s/\"//g"
        RESULT_VARIABLE DIFINE_LINUX_DISTRO_RESULT
        OUTPUT_VARIABLE LINUX_DISTRO
    )
    if (NOT ${DIFINE_LINUX_DISTRO_RESULT} EQUAL 0)
        message (FATAL_ERROR "Linux distro identification error")
    endif ()
endif ()

if(${LINUX_DISTRO} MATCHES "openSUSE")
  # OpenSuse/Leap
  set(CPACK_RPM_PACKAGE_REQUIRES "gtkmm3")
else()
  # Redhat/CentOS/Fedora/etc.
  set(CPACK_RPM_PACKAGE_REQUIRES "gtkmm30")
endif()

# Debian Jessie/Ubuntu Trusty/Mint Qiana (libgtkmm-3.0-1) or 
# Debian Stretch, Buster or newer, Ubuntu Xenial, Artful, Bionic or newer, Linux Mint Sarah, Tessa, Tina or newer (libgtkmm-3.0-1v5)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtkmm-3.0-1 | libgtkmm-3.0-1v5")

# include CPack model once all variables are set
include(CPack)