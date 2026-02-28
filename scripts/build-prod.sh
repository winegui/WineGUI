#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Release production build + create Debian package file (.deb), 
#  RPM [Red Hat] Package Manager (.rpm) and compressed file (.tgz/.tar.gz)
#
# Installs into /usr prefix directory under Linux.

set -e

# Required input parameter check (used for the defining the CPack generators)
if [ "$1" == "" ]; then
    echo "Usage: $0 <generator_names>"
    echo
    echo "Example: $0 \"TGZ;DEB;RPM\""
    exit 1
fi

rm -rf build_prod
cmake -GNinja -DCMAKE_INSTALL_PREFIX:PATH=/usr -DDOXYGEN=ON -DPACKAGE=ON -DCMAKE_BUILD_TYPE=Release -DGSETTINGS_COMPILE:BOOL=FALSE -B build_prod
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
    # Make the package file name unique, by renaming WineGUI-*.deb to WineGUI-*-bookworm.deb for example.
    # Basically adding a postfix to the deb file name
    mv WineGUI-*.deb "$(echo WineGUI-*.deb | sed "s/\.deb$/-${VERSION_CODENAME}.deb/")"
    echo "Debian package renamed to $(echo WineGUI-*.deb)"
fi
