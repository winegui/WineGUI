#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Release production build + create an AppImage (.AppImage).
#  A single self-contained, executable file that runs on most Linux
#  distributions without installation or root.
#
# The AppImage bundles the GTK stack, its gio-launch-desktop helper and a Python
# runtime used by WineGUI's automatically managed GE-Proton support. Wine itself
# remains a host dependency.
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

# Patch the AppDir copy of the desktop file (never misc/winegui.desktop, which the deb/rpm
# packages ship as-is). Two AppImage-only changes:
#
#   1. The installed file uses an absolute 'Exec=/usr/bin/winegui', correct for a system
#      install; a relocatable AppImage needs a bare 'Exec=winegui' so linuxdeploy can symlink
#      the AppRun target.
#   2. Rename it from winegui.desktop to org.melroy.winegui.desktop (matching the AppStream
#      component <id>). appimagetool derives the expected metainfo filename from the desktop
#      basename, so this makes it look for org.melroy.winegui.appdata.xml (staged below) rather
#      than winegui.appdata.xml. Without the rename appimagetool's bundled appstreamcli fails
#      the build with metainfo-filename-cid-mismatch. The rDNS basename is also the modern
#      Desktop-Entry recommendation. We pass this renamed copy to linuxdeploy below.
APPIMAGE_DESKTOP="${APPDIR}/usr/share/applications/org.melroy.winegui.desktop"
mv "${APPDIR}/usr/share/applications/winegui.desktop" "${APPIMAGE_DESKTOP}"
sed -i 's#^Exec=/usr/bin/winegui#Exec=winegui#' "${APPIMAGE_DESKTOP}"

# GTK resolves the window icon through its icon theme, which only scans a hicolor tree
# that has a valid index.theme at its root. On a normal system install the icon lands in
# /usr/share/icons/hicolor, which already ships an index.theme (hicolor-icon-theme pkg).
# Inside the AppImage the GTK plugin prepends $APPDIR/usr/share to XDG_DATA_DIRS, but
# linuxdeploy never synthesizes an index.theme there, so GTK cannot find the 'winegui'
# icon and the running app shows a blank window icon. Write a minimal one ourselves.
HICOLOR_DIR="${APPDIR}/usr/share/icons/hicolor"
cat > "${HICOLOR_DIR}/index.theme" <<'EOF'
[Icon Theme]
Name=Hicolor
Comment=Fallback icon theme
Directories=48x48/apps,scalable/apps

[48x48/apps]
Size=48
Context=Applications
Type=Fixed

[scalable/apps]
MinSize=1
Size=48
MaxSize=512
Context=Applications
Type=Scalable
EOF

# Ship AppStream metadata inside the AppImage only. The deb/rpm (CPack) packages do not
# generate or install a metainfo file, so we keep this out of the CMake install() rules and
# stage it directly into the AppDir. The file name matters because two checks disagree:
#
#   * appimagetool's presence check only recognises the legacy .appdata.xml suffix and derives
#     the expected basename from the desktop file; without a matching *.appdata.xml it warns
#     "AppStream upstream metadata is missing". (Our canonical source uses the modern
#     .metainfo.xml suffix, which this check ignores.)
#   * appimagetool then runs `appstreamcli validate`, which fails the build (exit 3) on any
#     warning, e.g. metainfo-filename-cid-mismatch when the basename != component <id>.
#
# Since we renamed the AppDir desktop file to org.melroy.winegui.desktop above, appimagetool
# looks for org.melroy.winegui.appdata.xml. That basename equals the rDNS <id>, so it is both
# found and validates cleanly (only non-fatal infos remain). The canonical source keeps its
# .metainfo.xml name for appstreamcli and the deb/rpm packages.
METAINFO_DIR="${APPDIR}/usr/share/metainfo"
mkdir -p "${METAINFO_DIR}"
cp "${PWD}/misc/org.melroy.winegui.metainfo.xml" "${METAINFO_DIR}/org.melroy.winegui.appdata.xml"

# Keep the AppDir metainfo internally consistent with the renamed desktop file: its
# <launchable> must reference org.melroy.winegui.desktop, not the original winegui.desktop.
# Patched in the AppDir copy only; the canonical misc/ file is unchanged for the deb/rpm packages.
sed -i 's#<launchable type="desktop-id">winegui.desktop</launchable>#<launchable type="desktop-id">org.melroy.winegui.desktop</launchable>#' \
    "${METAINFO_DIR}/org.melroy.winegui.appdata.xml"

# Locate the downloaded tools (paths defined in cmake/appimage.cmake).
TOOLS_DIR="${PWD}/${BUILD_DIR}/appimage-tools"
LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool-x86_64.AppImage"

# Gio launches non-D-Bus desktop applications through gio-launch-desktop. The
# GTK plugin bundles libgio but not this helper, while libgio first looks in a
# distro-specific path compiled into the library. Bundle the helper from the same
# GLib installation so the AppImage does not depend on the target distro using
# Debian's multiarch path, openSUSE's /usr/libexec path, or exposing either on PATH.
GLIB_LIBDIR="$(pkg-config --variable=libdir gio-2.0)"
GLIB_PREFIX="$(pkg-config --variable=prefix gio-2.0)"
GIO_LAUNCH_DESKTOP_EXECUTABLE=""
GIO_LAUNCH_DESKTOP_CANDIDATES=(
    "${GLIB_LIBDIR}/glib-2.0/gio-launch-desktop"
    "${GLIB_PREFIX}/libexec/gio-launch-desktop"
    "${GLIB_PREFIX}/lib/glib-2.0/gio-launch-desktop"
)

for candidate in "${GIO_LAUNCH_DESKTOP_CANDIDATES[@]}"; do
    if [[ -x "${candidate}" ]]; then
        GIO_LAUNCH_DESKTOP_EXECUTABLE="${candidate}"
        break
    fi
done

if [[ -z "${GIO_LAUNCH_DESKTOP_EXECUTABLE}" ]]; then
    echo "ERROR: Could not find the gio-launch-desktop helper belonging to gio-2.0." >&2
    printf 'Checked: %s\n' "${GIO_LAUNCH_DESKTOP_CANDIDATES[@]}" >&2
    exit 1
fi

# linuxdeploy turns scripts in AppDir/apprun-hooks into startup hooks sourced by
# the generated AppRun before WineGUI starts. GIO_APPRUN_HOOK_PATH is only the
# build-time path where we write our hook; it is not an environment variable
# interpreted by GLib or linuxdeploy.
#
# At runtime the hook exports GIO_LAUNCH_DESKTOP, which is GLib's explicit helper
# override. This makes the bundled libgio execute the matching helper inside the
# AppImage. Keep the override in a hook rather than prepending AppDir/usr/bin to
# PATH, since WineGUI deliberately invokes host tools such as Wine and UMU.
APPRUN_HOOK_DIR="${APPDIR}/apprun-hooks"
GIO_APPRUN_HOOK_PATH="${APPRUN_HOOK_DIR}/winegui-gio-launch-desktop.sh"
mkdir -p "${APPRUN_HOOK_DIR}"
cat > "${GIO_APPRUN_HOOK_PATH}" <<'EOF'
#!/usr/bin/env bash

# APPDIR is normally supplied by the AppImage runtime. Derive it from AppRun
# when the AppDir has been extracted and started directly.
if [[ -z "${APPDIR:-}" ]]; then
    APPDIR="$(dirname "$(readlink -f "$0")")"
    export APPDIR
fi

# Tell libgio to use the helper bundled from the same GLib installation.
export GIO_LAUNCH_DESKTOP="${APPDIR}/usr/bin/gio-launch-desktop"
EOF
chmod 755 "${GIO_APPRUN_HOOK_PATH}"

# The managed GE-Proton launcher is a Python zipapp. Bundle Python >= 3.10 and its
# standard library so AppImage users never have to install Python themselves.
PYTHON3_EXECUTABLE="$(command -v python3)"
if [[ -z "${PYTHON3_EXECUTABLE}" ]] || ! "${PYTHON3_EXECUTABLE}" -c 'import sys; raise SystemExit(sys.version_info < (3, 10))'; then
    echo "ERROR: Python 3.10 or newer is required to build the AppImage." >&2
    exit 1
fi
PYTHON_STDLIB="$("${PYTHON3_EXECUTABLE}" -c 'import sysconfig; print(sysconfig.get_path("stdlib"))')"
PYTHON_VERSION_DIR="python$("${PYTHON3_EXECUTABLE}" -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')"
mkdir -p "${APPDIR}/usr/lib/${PYTHON_VERSION_DIR}"
cp -a "${PYTHON_STDLIB}/." "${APPDIR}/usr/lib/${PYTHON_VERSION_DIR}/"

# linuxdeploy discovers plugins named 'linuxdeploy-plugin-<name>' on PATH.
export PATH="${TOOLS_DIR}:${PATH}"

# linuxdeploy/appimagetool are AppImages themselves; inside Docker (CI) there
# is no FUSE to mount them, so tell their runtimes to self-extract and run.
export APPIMAGE_EXTRACT_AND_RUN=1

# Version string (e.g. 'v3.1.0'), written by cmake/appimage.cmake. Used by
# linuxdeploy to name the output WineGUI-<VERSION>-x86_64.AppImage, matching the
# CPack naming and the release asset links.
export VERSION="$(cat "${PWD}/${BUILD_DIR}/appimage_version")"

# Point linuxdeploy's appimage output plugin at our pinned appimagetool.
export APPIMAGETOOL

echo "INFO: Building AppImage (version ${VERSION})..."
"${LINUXDEPLOY}" \
    --appdir "${APPDIR}" \
    --executable "${PYTHON3_EXECUTABLE}" \
    --executable "${GIO_LAUNCH_DESKTOP_EXECUTABLE}" \
    --plugin gtk \
    --output appimage \
    --desktop-file "${APPIMAGE_DESKTOP}" \
    --icon-file "${PWD}/misc/winegui.svg"

# Guard against future linuxdeploy/GLib layout changes silently recreating the
# cross-distro launch failure. The helper must be packaged, the generated AppRun
# must source our hook, and the minimal wrapper must be able to exec a host tool.
BUNDLED_GIO_LAUNCH_DESKTOP="${APPDIR}/usr/bin/gio-launch-desktop"
if [[ ! -x "${BUNDLED_GIO_LAUNCH_DESKTOP}" ]]; then
    echo "ERROR: linuxdeploy did not bundle gio-launch-desktop." >&2
    exit 1
fi
if ! grep -Fq "winegui-gio-launch-desktop.sh" "${APPDIR}/AppRun"; then
    echo "ERROR: The generated AppRun does not source the Gio helper hook." >&2
    exit 1
fi
if ! "${BUNDLED_GIO_LAUNCH_DESKTOP}" /bin/true; then
    echo "ERROR: The bundled gio-launch-desktop helper could not execute a host program." >&2
    exit 1
fi

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
