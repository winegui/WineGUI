
# Example: https://github.com/MariaDB/server/tree/12.2/cmake
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WineGUI is a user-friendly WINE graphical interface")
set(CPACK_PACKAGE_VENDOR "Melroy van den Berg")
set(CPACK_PACKAGE_CONTACT "Melroy van den Berg <melroy@melroy.org>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://winegui.melroy.org")
set(CPACK_RPM_PACKAGE_URL "https://winegui.melroy.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/misc/package_desc.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_TARGET}-${CPACK_PACKAGE_VERSION}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-v${CPACK_PACKAGE_VERSION}") # Without '-Linux' suffix

set(CPACK_RPM_PACKAGE_REQUIRES "(gtkmm4.0 or libgtkmm-4_0-0), cabextract, unzip, 7zip, wget, zenity, tar, xz, python3 >= 3.10")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    "/usr/share/applications"
    "/usr/share/glib-2.0"
    "/usr/share/glib-2.0/schemas"
    "/usr/share/icons"
    "/usr/share/icons/hicolor"
    "/usr/share/icons/hicolor/48x48"
    "/usr/share/icons/hicolor/48x48/apps"
    "/usr/share/icons/hicolor/scalable"
    "/usr/share/icons/hicolor/scalable/apps"
)
# Optional RPM packages
set(CPACK_RPM_PACKAGE_SUGGESTS "vulkan, vulkan-loader")

# Debian trixie, forky, sid, Ubuntu Noble Numbat, Linux Mint 22 (libgtkmm-4.0-0)
# If needed we can add multiple minor versions eg. via libgtkmm-4.0-0 | libgtkmm-4.0-1
# Note: xz-utils is needed to extract the Wine runner tar.xz archives (tar itself is an Essential package)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtkmm-4.0-0, cabextract, unzip, 7zip, wget, zenity, xz-utils, python3 (>= 3.10)")
# Optional deb packages
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "libvulkan1, libvulkan1:i386, mesa-vulkan-drivers, mesa-vulkan-drivers:i386")

# include CPack model once all variables are set
include(CPack)
