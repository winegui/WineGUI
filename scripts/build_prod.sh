#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Release production build + create Debian package file (.deb), 
#   RPM [Red Hat] Package Manager (.rpm) and compressed file (.tgz/.tar.gz)
rm -rf build_prod
mkdir build_prod
cd build_prod
cmake -GNinja -Ddoc=ON -DCMAKE_BUILD_TYPE=Release ..
ninja && 
echo "INFO: Building packages...";
cpack -G "TGZ;DEB;RPM"
