#!/usr/bin/env python3
"""Create the immutable five-package WineGUI APT publication batch."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from package import SUITES, debian_version


def load_metadata(path):
    if not path.is_file() or path.is_symlink():
        raise ValueError(f"metadata is not a regular file: {path}")
    value = json.loads(path.read_text())
    if not isinstance(value, dict):
        raise ValueError(f"metadata must be an object: {path}")
    required = {
        "schema_version", "suite", "filename", "package", "version",
        "architecture", "size", "sha256", "tag", "producing_job_id",
    }
    if set(value) != required or value["schema_version"] != 1:
        raise ValueError(f"invalid metadata schema: {path}")
    return value


def deb_field(path, name):
    return subprocess.check_output(
        ["dpkg-deb", "--field", str(path), name], text=True
    ).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--rebuild", default="1")
    args = parser.parse_args()
    if not re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", args.tag):
        parser.error("tag must be v<major>.<minor>.<patch>")
    if not re.fullmatch(r"[0-9a-fA-F]{40}", args.commit):
        parser.error("commit must be a full 40-character SHA")
    upstream_version = args.tag[1:]

    metadata_paths = sorted(args.input.glob("debian-package-*.json"))
    if len(metadata_paths) != len(SUITES):
        parser.error(f"expected {len(SUITES)} producer metadata files, found {len(metadata_paths)}")
    packages = {}
    for path in metadata_paths:
        item = load_metadata(path)
        suite = item["suite"]
        if suite not in SUITES or suite in packages:
            parser.error(f"invalid or duplicate suite {suite!r}")
        if item["tag"] != args.tag:
            parser.error(f"tag mismatch for {suite}")
        if item["package"] != "winegui" or item["architecture"] != "amd64":
            parser.error(f"package identity mismatch for {suite}")
        expected_filename = f"WineGUI-{args.tag}-{suite}.deb"
        expected_version = debian_version(upstream_version, suite, args.rebuild)
        if item["filename"] != expected_filename or item["version"] != expected_version:
            parser.error(f"filename/version mismatch for {suite}")
        if type(item["producing_job_id"]) is not int or item["producing_job_id"] <= 0:
            parser.error(f"missing producing job ID for {suite}")
        deb = args.input / item["filename"]
        if not deb.is_file() or deb.is_symlink() or deb.parent.resolve() != args.input.resolve():
            parser.error(f"unsafe or missing DEB for {suite}")
        data = deb.read_bytes()
        if len(data) != item["size"] or hashlib.sha256(data).hexdigest() != item["sha256"]:
            parser.error(f"size/checksum mismatch for {suite}")
        actual = {
            "package": deb_field(deb, "Package"),
            "version": deb_field(deb, "Version"),
            "architecture": deb_field(deb, "Architecture"),
        }
        if actual != {
            "package": "winegui", "version": expected_version, "architecture": "amd64"
        }:
            parser.error(f"DEB control metadata mismatch for {suite}")
        packages[suite] = (item, deb)
    if set(packages) != set(SUITES):
        parser.error("batch does not contain all supported suites")

    manifest = {
        "schema_version": 1,
        "release_tag": args.tag,
        "commit_sha": args.commit.lower(),
        "architecture": "amd64",
        "packages": {
            suite: {
                "filename": packages[suite][0]["filename"],
                "version": packages[suite][0]["version"],
                "size": packages[suite][0]["size"],
                "sha256": packages[suite][0]["sha256"],
                "producing_job_id": packages[suite][0]["producing_job_id"],
            }
            for suite in sorted(packages)
        },
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="winegui-apt-batch-", dir=args.output.parent) as temporary:
        staging = Path(temporary)
        for suite in sorted(packages):
            shutil.copyfile(packages[suite][1], staging / packages[suite][0]["filename"])
        manifest_path = staging / "manifest.json"
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        checksum_paths = sorted(staging.glob("*.deb")) + [manifest_path]
        (staging / "SHA256SUMS").write_text("".join(
            f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
            for path in checksum_paths
        ))
        if args.output.exists():
            if args.output.is_symlink() or not args.output.is_dir():
                parser.error("output must be a directory")
            shutil.rmtree(args.output)
        shutil.copytree(staging, args.output)


if __name__ == "__main__":
    try:
        main()
    except (TypeError, ValueError, OSError, json.JSONDecodeError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"WineGUI APT batch: {error}")
