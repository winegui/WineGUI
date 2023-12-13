#!/usr/bin/env bash
if [ $# -ge 1 ] && [[ $1 == "docker" ]]; then
  docker run -v $(pwd):/home --rm --workdir /home danger89/cmake:5.0 \
    bash -c 'find src/ include/ -iname *.cc -o -iname *.h -o -iname *.h.in | xargs clang-format -i -style=file -fallback-style=LLVM'
else
  find src/ include/ -iname *.cc -o -iname *.h -o -iname *.h.in | xargs clang-format -i -style=file -fallback-style=LLVM -assume-filename=../.clang-format
fi
