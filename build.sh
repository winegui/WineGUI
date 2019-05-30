#!/usr/bin/env bash
if [ ! -d "build" ]; then
  echo "Creating build directory..."
  mkdir build
fi

if [ -z "$(ls build)" ]; then
  echo "INFO: Run cmake & ninja"
  cd build
  cmake -GNinja ..
else
  echo "INFO: Only run ninja..."
  cd build
fi
ninja