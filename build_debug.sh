#!/usr/bin/env bash
if [ ! -d "build_debug" ]; then
  echo "Creating build_debug directory..."
  mkdir build_debug
fi

echo "INFO: Debug build"
cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ../
make
