#!/usr/bin/env bash
if [ $# -ge 1 ] && [[ $1 == "docker" ]]; then
  docker run -v $(pwd):/home --rm --workdir /home danger89/cmake:trixie-cppcheck-2.19.0 \
    bash -c 'find src/ include/ -iname *.cc -o -iname *.h -o -iname *.h.in | xargs clang-format --dry-run -Werror -style=file -fallback-style=LLVM'
else
  find src/ include/ -iname *.cc -o -iname *.h -o -iname *.h.in | xargs clang-format --dry-run -Werror -style=file -fallback-style=LLVM -assume-filename=../.clang-format
fi
