#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Build & run unit-tests
if [ -z "$(ls build_test)" ]; then
  echo "INFO: Run cmake & ninja"
  cmake -GNinja -DUNITTEST:BOOL=TRUE -B build_test
else
  echo "INFO: Only run ninja..."
fi
# Build & run unit-tests
cmake --build ./build_test --target tests
