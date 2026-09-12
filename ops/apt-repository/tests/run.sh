#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest -v "$SCRIPT_DIR/test_publisher.py"
PYTHONDONTWRITEBYTECODE=1 python3 -c \
  'import pathlib; source=pathlib.Path(__import__("sys").argv[1]); compile(source.read_text(), str(source), "exec")' \
  "$SCRIPT_DIR/../publisher/winegui_apt_publisher.py"
for script in "$SCRIPT_DIR"/../bin/* "$SCRIPT_DIR"/../reprepro/*.sh "$SCRIPT_DIR"/*.sh; do
  sh -n "$script"
done
