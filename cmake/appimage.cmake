# AppImage build tooling.
#
# When -DAPPIMAGE=ON is set, this module downloads the tools required to build an
# AppImage (linuxdeploy, its GTK plugin and appimagetool) into the build tree, so
# a developer does not need to install them system-wide. scripts/build-appimage.sh
# then drives these tools to produce a WineGUI-v<version>-x86_64.AppImage.
#
# The AppImage itself only bundles the GTK stack (via linuxdeploy-plugin-gtk);
# Wine v9+ and winetricks remain host run-time dependencies (resolved from PATH).
#
# NOTE: The pinned versions/hashes below occasionally need bumping. To update:
#   1. Pick a new tag (linuxdeploy / appimagetool) or commit (gtk plugin).
#   2. Download the asset and run `sha256sum` on it.
#   3. Update the URL + EXPECTED_HASH pair here.

# Where the downloaded tools land, referenced by scripts/build-appimage.sh
set(APPIMAGE_TOOLS_DIR "${CMAKE_BINARY_DIR}/appimage-tools")

# linuxdeploy (AppDir maintenance + AppImage output). Stable tagged release.
set(LINUXDEPLOY_URL
  "https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage")
set(LINUXDEPLOY_SHA256 "c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d")

# linuxdeploy GTK plugin (bundles GLib schemas, GDK-Pixbuf loaders, Pango, theme).
# No tagged releases exist; pin to an immutable commit on master.
set(LINUXDEPLOY_GTK_URL
  "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/7a3fbc31a9e5075073ff8790f26effbac5f84453/linuxdeploy-plugin-gtk.sh")
set(LINUXDEPLOY_GTK_SHA256 "b0f4cbc684a0103a9651f0955b635eaea0096b3a66c0f5a2c2aa337960375171")

# appimagetool (turns the AppDir into the final .AppImage). Stable tagged release.
set(APPIMAGETOOL_URL
  "https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage")
set(APPIMAGETOOL_SHA256 "ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0")

# Download helper: fetch to a destination, verify checksum and make it executable.
function(_appimage_download name url sha256 dest)
  if(EXISTS "${dest}")
    file(SHA256 "${dest}" _existing_hash)
    if("${_existing_hash}" STREQUAL "${sha256}")
      message(STATUS "AppImage tool ${name} already present and verified.")
      return()
    endif()
  endif()
  message(STATUS "Downloading AppImage tool ${name}...")
  file(DOWNLOAD "${url}" "${dest}"
    EXPECTED_HASH SHA256=${sha256}
    TLS_VERIFY ON
    STATUS _dl_status)
  list(GET _dl_status 0 _dl_code)
  if(NOT _dl_code EQUAL 0)
    list(GET _dl_status 1 _dl_msg)
    message(FATAL_ERROR "Failed to download ${name} from ${url}: ${_dl_msg}")
  endif()
  file(CHMOD "${dest}" PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)
endfunction()

file(MAKE_DIRECTORY "${APPIMAGE_TOOLS_DIR}")

_appimage_download("linuxdeploy" "${LINUXDEPLOY_URL}" "${LINUXDEPLOY_SHA256}"
  "${APPIMAGE_TOOLS_DIR}/linuxdeploy-x86_64.AppImage")
_appimage_download("linuxdeploy-plugin-gtk" "${LINUXDEPLOY_GTK_URL}" "${LINUXDEPLOY_GTK_SHA256}"
  "${APPIMAGE_TOOLS_DIR}/linuxdeploy-plugin-gtk.sh")
_appimage_download("appimagetool" "${APPIMAGETOOL_URL}" "${APPIMAGETOOL_SHA256}"
  "${APPIMAGE_TOOLS_DIR}/appimagetool-x86_64.AppImage")

# Single source of truth for the AppImage version string. We prefix with 'v' to
# match the CPack naming (WineGUI-v<version>) and the release asset links, so the
# produced file name is byte-identical to what scripts/create-release-links.sh
# points at. build-appimage.sh reads this file.
file(WRITE "${CMAKE_BINARY_DIR}/appimage_version" "v${PROJECT_VERSION}")

message(STATUS "AppImage tooling ready in ${APPIMAGE_TOOLS_DIR} (version v${PROJECT_VERSION}).")
