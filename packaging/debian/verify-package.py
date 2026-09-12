#!/usr/bin/env python3
"""Validate a built WineGUI DEB and emit producer metadata."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def field(deb, name):
    return subprocess.check_output(
        ["dpkg-deb", "--field", str(deb), name], text=True
    ).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--deb", required=True, type=Path)
    parser.add_argument("--suite", required=True)
    parser.add_argument("--upstream-version", required=True)
    parser.add_argument("--expected-version", required=True)
    parser.add_argument("--tag", default="")
    parser.add_argument("--job-id", default="")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    if not re.fullmatch(r"[a-z][a-z0-9]*", args.suite):
        parser.error("suite must be a lowercase distribution codename")
    if not args.deb.is_file() or args.deb.is_symlink():
        parser.error("DEB must be a regular, non-symlink file")
    expected_name = f"WineGUI-v{args.upstream_version}-{args.suite}.deb"
    if args.deb.name != expected_name:
        parser.error(f"expected filename {expected_name}, got {args.deb.name}")
    actual = {
        "package": field(args.deb, "Package"),
        "version": field(args.deb, "Version"),
        "architecture": field(args.deb, "Architecture"),
    }
    expected = {"package": "winegui", "version": args.expected_version, "architecture": "amd64"}
    if actual != expected:
        parser.error(f"control metadata mismatch: expected {expected}, got {actual}")
    if args.tag and args.tag != f"v{args.upstream_version}":
        parser.error("release tag and upstream version disagree")
    if args.job_id and not re.fullmatch(r"[1-9][0-9]*", args.job_id):
        parser.error("job ID must be a positive integer")

    data = args.deb.read_bytes()
    args.output.write_text(json.dumps({
        "schema_version": 1,
        "suite": args.suite,
        "filename": args.deb.name,
        "package": actual["package"],
        "version": actual["version"],
        "architecture": actual["architecture"],
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "tag": args.tag or None,
        "producing_job_id": int(args.job_id) if args.job_id else None,
    }, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
