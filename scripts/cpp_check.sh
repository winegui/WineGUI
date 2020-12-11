#!/usr/bin/env bash
cppcheck --enable=all --suppressions-list=suppressions.txt --error-exitcode=1 -I ./include/ ./src/