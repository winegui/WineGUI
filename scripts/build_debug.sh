#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Debugging build

if [ ! -d "build_debug" ]; then
  echo "Creating build_debug directory..."
  mkdir build_debug
fi

cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ../ &&
make
