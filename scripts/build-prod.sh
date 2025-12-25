#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Release production build + create Debian package file (.deb), 
#   RPM [Red Hat] Package Manager (.rpm) and compressed file (.tgz/.tar.gz)
#
#  Installs into /usr prefix directory under Linux.

# Required input parameter check (used for the defining the CPack generators)
if [ "$1" == "" ]; then
    echo "Usage: $0 <generator_names>"
    echo
    echo "Example: $0 \"TGZ;DEB;RPM\""
    exit 1
fi

rm -rf build_prod
cmake -GNinja -DCMAKE_INSTALL_PREFIX:PATH=/usr -DDOXYGEN=ON -DPACKAGE=ON -DCMAKE_BUILD_TYPE=Release -B build_prod
cmake --build ./build_prod --config Release
echo "INFO: Building packages..."
cd build_prod
# Requires root
cpack -C Release -G "$1"
