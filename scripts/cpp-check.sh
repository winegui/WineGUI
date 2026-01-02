#!/usr/bin/env bash
cppcheck --enable=all,unusedFunction --suppressions-list=suppressions.txt --inline-suppr --check-level=exhaustive --error-exitcode=1 -I ./include/ ./src/
