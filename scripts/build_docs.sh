#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Only build the documentation

mkdir build_docs
cd build_docs
echo "INFO: Build Doxygen...";
cmake -G Ninja -Ddoc=ON ..
ninja Doxygen