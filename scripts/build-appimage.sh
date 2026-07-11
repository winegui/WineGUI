#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Release production build + create an AppImage (.AppImage).
#  A single self-contained, executable file that runs on most Linux
#  distributions without installation or root.
#
# The AppImage bundles only the GTK stack (via linuxdeploy-plugin-gtk).
# Wine v9+ and winetricks remain host run-time dependencies (found via PATH).
#
# The linuxdeploy / linuxdeploy-plugin-gtk / appimagetool tools are downloaded
# by CMake (see cmake/appimage.cmake) when configured with -DAPPIMAGE=ON, so no
# system-wide installation of those tools is required.

set -e

BUILD_DIR="build_appimage"
APPDIR="${PWD}/${BUILD_DIR}/AppDir"

rm -rf "${BUILD_DIR}"

# Configure + build. -DAPPIMAGE=ON downloads the AppImage tooling into the build tree.
# GSETTINGS_COMPILE is OFF because the GTK plugin compiles the schema inside the AppDir.
cmake -GNinja \
    -DCMAKE_INSTALL_PREFIX:PATH=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DAPPIMAGE=ON \
    -DGSETTINGS_COMPILE:BOOL=FALSE \
    -B "${BUILD_DIR}"
cmake --build "./${BUILD_DIR}" --config Release

# Stage the install tree into the AppDir (AppDir/usr/...).
echo "INFO: Staging AppDir..."
DESTDIR="${APPDIR}" cmake --install "${BUILD_DIR}"

# The installed desktop file uses an absolute 'Exec=/usr/bin/winegui', which is
# correct for a normal system install. Inside a relocatable AppImage, however,
# linuxdeploy needs a bare 'Exec=winegui' to symlink the AppRun target. Instead
# of changing the shipped misc/winegui.desktop, we patch only the AppDir copy
# (and pass that same copy to linuxdeploy below).
APPIMAGE_DESKTOP="${APPDIR}/usr/share/applications/winegui.desktop"
sed -i 's#^Exec=/usr/bin/winegui#Exec=winegui#' "${APPIMAGE_DESKTOP}"

# Locate the downloaded tools (paths defined in cmake/appimage.cmake).
TOOLS_DIR="${PWD}/${BUILD_DIR}/appimage-tools"
LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool-x86_64.AppImage"

# linuxdeploy discovers plugins named 'linuxdeploy-plugin-<name>' on PATH.
export PATH="${TOOLS_DIR}:${PATH}"

# Version string (e.g. 'v3.1.0'), written by cmake/appimage.cmake. Used by
# linuxdeploy to name the output WineGUI-<VERSION>-x86_64.AppImage, matching the
# CPack naming and the release asset links.
export VERSION="$(cat "${PWD}/${BUILD_DIR}/appimage_version")"

# Point linuxdeploy's appimage output plugin at our pinned appimagetool.
export APPIMAGETOOL

echo "INFO: Building AppImage (version ${VERSION})..."
"${LINUXDEPLOY}" \
    --appdir "${APPDIR}" \
    --plugin gtk \
    --output appimage \
    --desktop-file "${APPIMAGE_DESKTOP}" \
    --icon-file "${PWD}/misc/winegui.svg"

# Collect the artifact alongside the other release packages (build_prod/), so the
# CI 'gh release create build_prod/WineGUI-*' glob and the release links pick it up.
# linuxdeploy writes the AppImage into the current directory using $VERSION.
APPIMAGE_FILE="WineGUI-${VERSION}-x86_64.AppImage"
mkdir -p build_prod
mv "${APPIMAGE_FILE}" build_prod/
echo "INFO: AppImage created: build_prod/${APPIMAGE_FILE}"

# Fallback: if the produced AppImage fails to run on target systems (GTK4 bundling
# issues with linuxdeploy-plugin-gtk, which is officially GTK2/3), consider switching
# to sharun / quick-sharun (https://github.com/VHSgunzo/sharun). See README.
