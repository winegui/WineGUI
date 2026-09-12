#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
COMMIT=3afde91f87342b473bb624f3bf3c5cc0341b75e8
IMAGE=${WINEGUI_REPREPRO_IMAGE:-winegui/reprepro:${COMMIT}}
TEST_ROOT=$(mktemp -d)
trap 'rm -rf "$TEST_ROOT"' EXIT HUP INT TERM

"$SCRIPT_DIR/../reprepro/build-pinned.sh"
mkdir -p "$TEST_ROOT/repository/conf" "$TEST_ROOT/repository/db" "$TEST_ROOT/repository/out"
cat > "$TEST_ROOT/repository/conf/distributions" <<'EOF'
Origin: WineGUI proof
Label: WineGUI proof
Suite: noble
Codename: noble
Architectures: amd64
Components: main
Limit: 0
EOF

build_deb() {
  version=$1
  tree="$TEST_ROOT/package-$version"
  mkdir -p "$tree/DEBIAN"
  cat > "$tree/DEBIAN/control" <<EOF
Package: winegui
Version: $version
Architecture: amd64
Section: utils
Priority: optional
Maintainer: WineGUI proof <noreply@example.invalid>
Description: Disposable multi-version proof package
EOF
  # This older reviewed fork predates control.tar.zst support; WineGUI's CPack
  # artifacts use gzip members, so the proof package must match production.
  dpkg-deb -Zgzip --build "$tree" "$TEST_ROOT/winegui_${version}_amd64.deb" >/dev/null
}

build_deb '1.0-1~ubuntu24.04.1'
build_deb '1.1-1~ubuntu24.04.1'

run_reprepro() {
  docker run --rm --user "$(id -u):$(id -g)" \
    --volume "$TEST_ROOT/repository:/repo" \
    --volume "$TEST_ROOT:/input:ro" \
    "$IMAGE" --basedir /repo --confdir /repo/conf --dbdir /repo/db --outdir /repo/out "$@"
}

run_reprepro includedeb noble /input/winegui_1.0-1~ubuntu24.04.1_amd64.deb
run_reprepro includedeb noble /input/winegui_1.1-1~ubuntu24.04.1_amd64.deb
# Exact retries must be harmless and must not add a third index entry.
run_reprepro includedeb noble /input/winegui_1.0-1~ubuntu24.04.1_amd64.deb

PACKAGES="$TEST_ROOT/repository/out/dists/noble/main/binary-amd64/Packages.gz"
test "$(gzip -dc "$PACKAGES" | grep -c '^Package: winegui$')" -eq 2
gzip -dc "$PACKAGES" | grep -q '^Version: 1.0-1~ubuntu24.04.1$'
gzip -dc "$PACKAGES" | grep -q '^Version: 1.1-1~ubuntu24.04.1$'

mkdir -p \
  "$TEST_ROOT/apt/etc/apt" \
  "$TEST_ROOT/apt/etc/apt/preferences.d" \
  "$TEST_ROOT/apt/var/lib/apt/lists/partial" \
  "$TEST_ROOT/apt/var/cache/apt/archives/partial"
printf 'deb [trusted=yes] file:%s noble main\n' "$TEST_ROOT/repository/out" > "$TEST_ROOT/apt/etc/apt/sources.list"
apt-get \
  -o "Dir=$TEST_ROOT/apt" \
  -o "Dir::Etc::sourcelist=$TEST_ROOT/apt/etc/apt/sources.list" \
  -o "Dir::Etc::sourceparts=-" \
  -o APT::Get::List-Cleanup=0 update >/dev/null
POLICY=$(apt-cache \
  -o "Dir=$TEST_ROOT/apt" \
  -o "Dir::Etc::sourcelist=$TEST_ROOT/apt/etc/apt/sources.list" \
  -o "Dir::Etc::sourceparts=-" policy winegui)
printf '%s\n' "$POLICY" | grep -q '1.0-1~ubuntu24.04.1'
printf '%s\n' "$POLICY" | grep -q '1.1-1~ubuntu24.04.1'
echo "PASS: pinned reprepro indexes both versions and APT exposes both candidates"
