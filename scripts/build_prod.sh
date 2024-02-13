#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Release production build + create Debian package file (.deb), 
#   RPM [Red Hat] Package Manager (.rpm) and compressed file (.tgz/.tar.gz)
rm -rf build_prod
cmake -GNinja -DCMAKE_INSTALL_PREFIX:PATH=/usr -Ddoc=ON -Dpackage=ON -DCMAKE_BUILD_TYPE=Release -B build_prod
cmake --build ./build_prod --config Release
echo "INFO: Building packages..."
cd build_prod
# Requires root
cpack -C Release -G "TGZ;DEB;RPM"
