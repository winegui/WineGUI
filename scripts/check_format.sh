#!/usr/bin/env bash
find src/ include/ -iname *.cc -o -iname *.h -o -iname *.h.in | xargs clang-format --dry-run -Werror -style=file -fallback-style=LLVM
