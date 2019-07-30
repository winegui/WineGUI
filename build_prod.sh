#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Production build + create Debian package file (.deb)

if [ ! -d "build" ]; then
  echo "Creating build directory..."
  mkdir build
fi

if [ -z "$(ls build)" ]; then
  echo "INFO: Run cmake & ninja for production"
  cd build
  cmake -GNinja -Ddoc=ON -Dprod=ON ..
else
  echo "INFO: Only run ninja..."
  cd build
fi
ninja
cpack -G DEB