#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Release production build + create Debian package file (.deb), 
#  RPM [Red Hat] Package Manager (.rpm) and compressed file (.tgz/.tar.gz)
#
# Installs into /usr prefix directory under Linux.

set -euo pipefail

# Required input parameter check (used for the defining the CPack generators)
if [ "${1:-}" == "" ]; then
    echo "Usage: $0 <generator_names>"
    echo
    echo "Example: $0 \"TGZ;DEB;RPM\""
    exit 1
fi

cmake_args=(
    -GNinja
    -DCMAKE_INSTALL_PREFIX:PATH=/usr
    -DDOXYGEN=ON
    -DPACKAGE=ON
    -DCMAKE_BUILD_TYPE=Release
    -DGSETTINGS_COMPILE:BOOL=FALSE
)

if [[ "${WINEGUI_APT_ENABLED:-OFF}" == "ON" ]]; then
    : "${WINEGUI_APT_PUBLIC_KEY_FILE:?Set WINEGUI_APT_PUBLIC_KEY_FILE to a protected GitLab file variable}"
    : "${WINEGUI_APT_KEY_FINGERPRINTS:?Set WINEGUI_APT_KEY_FINGERPRINTS to the full primary fingerprint(s)}"
    : "${WINEGUI_APT_KEY_GENERATION:?Set WINEGUI_APT_KEY_GENERATION to a positive integer}"
    cmake_args+=(
        -DWINEGUI_APT_ENABLED=ON
        "-DWINEGUI_APT_KEY_FILE=${WINEGUI_APT_PUBLIC_KEY_FILE}"
        "-DWINEGUI_APT_KEY_FINGERPRINTS=${WINEGUI_APT_KEY_FINGERPRINTS}"
        "-DWINEGUI_APT_KEY_GENERATION=${WINEGUI_APT_KEY_GENERATION}"
    )
fi
cmake_args+=("-DWINEGUI_DEB_REBUILD=${WINEGUI_DEB_REBUILD:-1}")

rm -rf build_prod
cmake "${cmake_args[@]}" -B build_prod
cmake --build ./build_prod --config Release
echo "INFO: Building packages..."
cd build_prod
cpack -C Release -G "$1"

# Check if deb is in the generator names
if [[ "$1" == *"DEB"* ]]; then
    echo "Debian package found in generator names, renaming..."
    # Load os-release
    . /etc/os-release
    # Use the version codename for the new file name prefix.
    # Make the package file name unique, by renaming WineGUI-*.deb to WineGUI-*-trixie.deb for example.
    # Basically adding a postfix to the deb file name
    shopt -s nullglob
    debs=(WineGUI-*.deb)
    if [[ ${#debs[@]} -ne 1 ]]; then
        echo "ERROR: Expected exactly one Debian package, found ${#debs[@]}" >&2
        exit 1
    fi
    deb="${debs[0]}"
    renamed="${deb%.deb}-${VERSION_CODENAME}.deb"
    mv -- "$deb" "$renamed"
    upstream_version="${CI_COMMIT_TAG:-}"
    upstream_version="${upstream_version#v}"
    if [[ -z "$upstream_version" && "$renamed" =~ ^WineGUI-v([0-9]+\.[0-9]+\.[0-9]+)-${VERSION_CODENAME}\.deb$ ]]; then
        upstream_version="${BASH_REMATCH[1]}"
    fi
    python3 ../packaging/debian/verify-package.py \
        --deb "$renamed" \
        --suite "$VERSION_CODENAME" \
        --upstream-version "$upstream_version" \
        --expected-version "$(<debian-control/version.txt)" \
        --tag "${CI_COMMIT_TAG:-}" \
        --job-id "${CI_JOB_ID:-}" \
        --output "debian-package-${VERSION_CODENAME}.json"
    echo "Debian package renamed to $renamed and its control metadata was verified"
fi
