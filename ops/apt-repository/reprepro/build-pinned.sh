#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
COMMIT=3afde91f87342b473bb624f3bf3c5cc0341b75e8
IMAGE=${WINEGUI_REPREPRO_IMAGE:-winegui/reprepro:${COMMIT}}

exec docker build \
  --build-arg "REPREPRO_COMMIT=$COMMIT" \
  --tag "$IMAGE" \
  "$SCRIPT_DIR"
