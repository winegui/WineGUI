#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Debug build
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -B build_debug
cmake --build ./build_debug --config Debug
