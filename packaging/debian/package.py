#!/usr/bin/env python3
"""Build-time Debian metadata and maintainer-script generation (stdlib only)."""

import argparse
import base64
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


# Actual distribution releases, not arbitrary ordering ranks. Development Debian
# images may omit VERSION_ID, so their codename provides the release identity.
SUITES = {
    "trixie": ("debian", "13"),
    "forky": ("debian", "14"),
    "noble": ("ubuntu", "24.04"),
    "plucky": ("ubuntu", "25.04"),
    "resolute": ("ubuntu", "26.04"),
}
HERE = Path(__file__).resolve().parent


def run(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT).strip()


def os_release(path):
    values = {}
    for line in path.read_text().splitlines():
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        parts = shlex.split(value, comments=True)
        values[key] = parts[0] if len(parts) == 1 else ""
    return values


def positive(value):
    if not re.fullmatch(r"[1-9][0-9]{0,8}", str(value)):
        raise ValueError("generation/rebuild must be a positive integer of at most 9 digits")
    return int(value)


def debian_version(upstream, suite, rebuild="1", release=None):
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", upstream):
        raise ValueError("expected a numeric major.minor.patch application version")
    revision = positive(rebuild)
    if suite in SUITES:
        family, version = SUITES[suite]
        if release and release.get("ID") != family:
            raise ValueError("suite does not belong to the build distribution")
    else:
        release = release or {}
        family, version = release.get("ID", "linux"), release.get("VERSION_ID", "0")
        if not re.fullmatch(r"[a-z][a-z0-9]*", family):
            raise ValueError("invalid distribution ID")
        if not re.fullmatch(r"[0-9]+(?:\.[0-9]+)*", version):
            raise ValueError("unsupported distribution VERSION_ID")
        # Local derivative builds retain a distinct, usable package identity.
        version += "." + suite
    return f"{upstream}-1~{family}{version}.{revision}"


def public_key(path, expected):
    """Normalize only an explicitly fingerprint-pinned public OpenPGP export."""
    fingerprints = sorted(part.strip().upper() for part in expected.split(","))
    if not fingerprints or any(not re.fullmatch(r"[A-F0-9]{40}|[A-F0-9]{64}", f) for f in fingerprints):
        raise ValueError("provide comma-separated full primary key fingerprints")
    if len(set(fingerprints)) != len(fingerprints):
        raise ValueError("duplicate expected key fingerprint")
    if not path.is_file() or path.is_symlink() or path.stat().st_size > 1024 * 1024:
        raise ValueError("public key input must be a regular export smaller than 1 MiB")
    with tempfile.TemporaryDirectory(prefix="winegui-key-check-") as directory:
        command = ["gpg", "--batch", "--no-options", "--homedir", directory]
        listing = run(*command, "--with-colons", "--import-options", "show-only", "--import", str(path))
        found, signing = [], {}
        current_primary, primary_fingerprint, primary_can_sign = None, False, False
        for line in listing.splitlines():
            fields = line.split(":")
            if fields[0] in ("sec", "ssb"):
                raise ValueError("secret key material must never be supplied to package builds")
            if fields[0] in ("pub", "sub"):
                if fields[1] in ("r", "e", "d"):
                    raise ValueError("revoked, expired or disabled public key")
                can_sign = len(fields) > 11 and "s" in fields[11].lower()
                if fields[0] == "pub":
                    current_primary = None
                    primary_can_sign = can_sign
                    primary_fingerprint = True
                elif current_primary is not None:
                    signing[current_primary] = signing[current_primary] or can_sign
            elif fields[0] == "fpr" and primary_fingerprint:
                current_primary = fields[9]
                found.append(current_primary)
                signing[current_primary] = primary_can_sign
                primary_fingerprint = False
        if sorted(found) != fingerprints or not all(signing.get(fingerprint, False) for fingerprint in found):
            raise ValueError("public signing key fingerprints do not match the explicit allowlist")
        subprocess.run([*command, "--import", str(path)], check=True, capture_output=True)
        # Also reject private packets in otherwise public-looking input.
        if run(*command, "--with-colons", "--list-secret-keys"):
            raise ValueError("secret key material must never be supplied to package builds")
        key = subprocess.check_output([*command, "--export-options", "export-minimal", "--export", *fingerprints])
    if not key:
        raise ValueError("empty public key export")
    return key, fingerprints


def prepare(args):
    release = os_release(args.os_release)
    suite = release.get("VERSION_CODENAME", "unknown")
    if not re.fullmatch(r"[a-z][a-z0-9]*", suite):
        raise ValueError("invalid VERSION_CODENAME")
    version = debian_version(args.version, suite, args.rebuild, release)
    if args.tag and args.tag != f"v{args.version}":
        raise ValueError("release tag and actual application version disagree")
    key, fingerprints, generation = b"", [], 0
    if args.enabled == "1":
        if suite not in SUITES:
            raise ValueError("repository-enabled packages require a supported build suite")
        if not args.key_file or not args.fingerprints or not args.generation:
            raise ValueError("APT enabled: public key file, fingerprints and generation are required")
        generation = positive(args.generation)
        key, fingerprints = public_key(Path(args.key_file), args.fingerprints)
    args.output.mkdir(parents=True, exist_ok=True)
    common = (HERE / "lifecycle.sh").read_text()
    substitutions = {
        "@APT_ENABLED@": args.enabled,
        "@KEY_GENERATION@": str(generation),
        "@KEY_BASE64@": base64.b64encode(key).decode(),
        "@KEY_SHA256@": hashlib.sha256(key).hexdigest() if key else "",
    }
    for name in ("postinst", "postrm"):
        content = "#!/bin/sh\nset -eu\n" + common + (HERE / f"{name}.in").read_text()
        for pattern, value in substitutions.items():
            content = content.replace(pattern, value)
        destination = args.output / name
        destination.write_text(content)
        destination.chmod(0o755)
    (args.output / "version.txt").write_text(version)
    (args.output / "suite.txt").write_text(suite)
    (args.output / "winegui-build.json").write_text(json.dumps({
        "schema_version": 1, "suite": suite, "version": version,
        "apt_enabled": args.enabled == "1", "key_generation": generation,
        "key_sha256": hashlib.sha256(key).hexdigest() if key else None,
        "key_fingerprints": fingerprints,
    }, sort_keys=True) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--os-release", type=Path, default=Path("/etc/os-release"))
    parser.add_argument("--rebuild", default="1")
    parser.add_argument("--enabled", choices=("0", "1"), default="0")
    parser.add_argument("--tag", default="")
    parser.add_argument("--key-file", default="")
    parser.add_argument("--fingerprints", default="")
    parser.add_argument("--generation", default="")
    try:
        prepare(parser.parse_args())
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"WineGUI Debian packaging: {error}\n")


if __name__ == "__main__":
    main()
