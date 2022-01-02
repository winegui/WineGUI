#!/usr/bin/env bash
find src/ include/ -iname *.cc -o -iname *.h -o -iname *.h.in | xargs clang-format -i -style=file -fallback-style=LLVM -assume-filename=../.clang-format
