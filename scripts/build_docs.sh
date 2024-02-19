#!/usr/bin/env bash
# By: Melroy van den Berg
# Description: Only build the documentation
cmake -GNinja -DDOXYGEN=ON -B build_docs
cmake --build ./build_docs --target Doxygen
