#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Development build

if [ ! -d "build" ]; then
  echo "Creating build directory..."
  mkdir build
fi

if [ -z "$(ls build)" ]; then
  echo "INFO: Run cmake & ninja"
  cd build
  cmake -GNinja -Ddoc=ON ..
else
  echo "INFO: Only run ninja..."
  cd build
fi
ninja