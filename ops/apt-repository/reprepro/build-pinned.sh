#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# shellcheck source=version.env
. "$SCRIPT_DIR/version.env"
IMAGE=${WINEGUI_REPREPRO_IMAGE:-winegui/reprepro:${REPREPRO_VERSION}-${REPREPRO_COMMIT}}

exec docker build \
  --build-arg "REPREPRO_VERSION=$REPREPRO_VERSION" \
  --build-arg "REPREPRO_TAG=$REPREPRO_TAG" \
  --build-arg "REPREPRO_COMMIT=$REPREPRO_COMMIT" \
  --tag "$IMAGE" \
  "$SCRIPT_DIR"
