#!/usr/bin/env python3
"""Validate immutable WineGUI release batches and publish a static APT repository."""

from __future__ import annotations

import argparse
import errno
import hashlib
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import time
from pathlib import Path

SUITES = {
    "noble": "ubuntu24.04",
    "plucky": "ubuntu25.04",
    "resolute": "ubuntu26.04",
    "trixie": "debian13",
    "forky": "debian14",
}
EXPECTED_FILES = {"manifest.json", "SHA256SUMS"}
PAYLOAD_DIRECTORY = "apt_repository_batch"
PUBLISHER_PROCESSING = "publisher-processing"
PUBLISHER_ARCHIVE = "publisher-archive"
PUBLISHER_QUARANTINE = "publisher-quarantine"
BATCH_ID_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$")
SHA_RE = re.compile(r"^[0-9a-f]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-f]{40}([0-9a-f]{24})?$")
VERSION_RE = re.compile(r"^[0-9]+(?:\.[0-9]+){1,3}(?:[+~.-][0-9A-Za-z.+~:-]+)?$")
CONTROL_ARCHIVE_RE = re.compile(r"^control\.tar(?:\.(?:gz|xz|zst|bz2|lzma))?$")


class PublishError(RuntimeError):
    pass


class InjectedFailure(RuntimeError):
    """Test-only crash point; deliberately bypasses normal failure handling."""


def atomic_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump(value, stream, sort_keys=True, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
        directory_fd = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def sync_directory(path: Path) -> None:
    directory_fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(directory_fd)
    finally:
        os.close(directory_fd)


def run(command: list[str], *, env: dict[str, str] | None = None) -> str:
    try:
        result = subprocess.run(
            command,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=env,
        )
    except subprocess.CalledProcessError as exc:
        detail = (exc.stderr or exc.stdout or str(exc)).strip()
        raise PublishError(f"command failed ({' '.join(command)}): {detail}") from exc
    return result.stdout.strip()


def require_plain_directory(path: Path) -> None:
    info = path.lstat()
    if not stat.S_ISDIR(info.st_mode) or stat.S_ISLNK(info.st_mode):
        raise PublishError(f"not a plain directory: {path}")


def list_plain_files(path: Path) -> dict[str, Path]:
    require_plain_directory(path)
    result: dict[str, Path] = {}
    with os.scandir(path) as entries:
        for entry in entries:
            if entry.name in (".", "..") or "/" in entry.name or "\x00" in entry.name:
                raise PublishError(f"unsafe input name: {entry.name!r}")
            if not entry.is_file(follow_symlinks=False):
                raise PublishError(f"input must contain regular files only: {entry.name}")
            result[entry.name] = Path(entry.path)
    return result


def copy_batch(source: Path, destination: Path) -> None:
    if destination.exists():
        require_plain_directory(destination)
        return
    temporary = destination.with_name(f".{destination.name}.copying")
    if temporary.exists():
        if temporary.is_symlink() or not temporary.is_dir():
            raise PublishError(f"unsafe interrupted private copy: {temporary}")
        shutil.rmtree(temporary)
    temporary.mkdir(parents=True, mode=0o700)

    def copy_directory(source_dir: Path, target_dir: Path) -> None:
        require_plain_directory(source_dir)
        with os.scandir(source_dir) as entries:
            for entry in entries:
                if entry.name in (".", "..") or "/" in entry.name or "\x00" in entry.name:
                    raise PublishError(f"unsafe input name: {entry.name!r}")
                source_path = Path(entry.path)
                target_path = target_dir / entry.name
                if entry.is_dir(follow_symlinks=False):
                    target_path.mkdir(mode=0o700)
                    copy_directory(source_path, target_path)
                    sync_directory(target_path)
                    continue
                if not entry.is_file(follow_symlinks=False):
                    raise PublishError(f"input must contain regular files/directories only: {entry.name}")
                source_fd = os.open(source_path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
                try:
                    before = os.fstat(source_fd)
                    if not stat.S_ISREG(before.st_mode):
                        raise PublishError(f"input changed type while copying: {entry.name}")
                    target_fd = os.open(target_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
                    try:
                        with os.fdopen(os.dup(source_fd), "rb") as source_stream, os.fdopen(
                            target_fd, "wb"
                        ) as target_stream:
                            shutil.copyfileobj(source_stream, target_stream, 1024 * 1024)
                            target_stream.flush()
                            os.fsync(target_stream.fileno())
                    except Exception:
                        try:
                            os.close(target_fd)
                        except OSError:
                            pass
                        raise
                    after = os.fstat(source_fd)
                    if (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) != (
                        after.st_dev,
                        after.st_ino,
                        after.st_size,
                        after.st_mtime_ns,
                    ):
                        raise PublishError(f"input changed while copying: {entry.name}")
                finally:
                    os.close(source_fd)

    try:
        copy_directory(source, temporary)
        sync_directory(temporary)
        os.replace(temporary, destination)
        sync_directory(destination.parent)
    except Exception:
        shutil.rmtree(temporary, ignore_errors=True)
        raise


def load_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise PublishError(f"invalid {path.name}: {exc}") from exc
    if not isinstance(value, dict):
        raise PublishError(f"{path.name} must contain a JSON object")
    return value


def parse_checksums(path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    try:
        lines = path.read_text(encoding="ascii").splitlines()
    except (OSError, UnicodeDecodeError) as exc:
        raise PublishError(f"invalid SHA256SUMS: {exc}") from exc
    for line in lines:
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9][A-Za-z0-9._+-]*)", line)
        if not match or match.group(2) in result:
            raise PublishError(f"invalid or duplicate SHA256SUMS line: {line!r}")
        result[match.group(2)] = match.group(1)
    return result


def dpkg_field(deb: Path, field: str, dpkg_deb: str) -> str:
    return run([dpkg_deb, "-f", str(deb), field])


def deb_members(path: Path) -> set[str]:
    members = set()
    with path.open("rb") as stream:
        if stream.read(8) != b"!<arch>\n":
            raise PublishError(f"not a Debian ar archive: {path.name}")
        while True:
            header = stream.read(60)
            if not header:
                break
            if len(header) != 60 or header[58:60] != b"`\n":
                raise PublishError(f"invalid ar member header: {path.name}")
            try:
                name = header[:16].decode("ascii").strip().rstrip("/")
                size = int(header[48:58].decode("ascii").strip())
            except (UnicodeDecodeError, ValueError) as exc:
                raise PublishError(f"invalid ar metadata: {path.name}") from exc
            if not name or name.startswith("#1/"):
                raise PublishError(f"unsupported ar member naming: {path.name}")
            members.add(name)
            stream.seek(size + (size % 2), os.SEEK_CUR)
    return members


def validate_batch(batch: Path, dpkg_deb: str) -> tuple[dict, list[dict], str]:
    files = list_plain_files(batch)
    if set(files) < EXPECTED_FILES:
        raise PublishError("batch must contain manifest.json and SHA256SUMS")
    manifest = load_json(files["manifest.json"])
    required_top = {"schema_version", "release_tag", "commit_sha", "architecture", "packages"}
    if set(manifest) != required_top:
        raise PublishError(f"manifest keys must be exactly {sorted(required_top)}")
    if manifest["schema_version"] != 1:
        raise PublishError("unsupported manifest schema_version")
    tag = manifest["release_tag"]
    if not isinstance(tag, str) or not re.fullmatch(r"v[0-9]+(?:\.[0-9]+){1,3}", tag):
        raise PublishError("release_tag must be a simple v-prefixed release version")
    if manifest["architecture"] != "amd64":
        raise PublishError("manifest architecture must be amd64")
    if not isinstance(manifest["commit_sha"], str) or not COMMIT_RE.fullmatch(manifest["commit_sha"]):
        raise PublishError("commit_sha must be a lowercase full Git commit hash")
    packages = manifest["packages"]
    if not isinstance(packages, dict) or len(packages) != len(SUITES):
        raise PublishError("manifest must describe exactly five suite-keyed packages")

    package_keys = {
        "filename",
        "version",
        "size",
        "sha256",
        "producing_job_id",
    }
    expected_names = set(EXPECTED_FILES)
    normalized: list[dict] = []
    rebuilds: set[str] = set()
    upstream = tag[1:]
    for suite, package in packages.items():
        if suite not in SUITES:
            raise PublishError(f"invalid suite: {suite!r}")
        if not isinstance(package, dict) or set(package) != package_keys:
            raise PublishError(f"each package must have exactly {sorted(package_keys)}")
        expected_filename = f"WineGUI-{tag}-{suite}.deb"
        if package["filename"] != expected_filename:
            raise PublishError(f"unexpected filename for {suite}: {package['filename']!r}")
        if Path(package["filename"]).name != package["filename"]:
            raise PublishError("package filename must not contain a path")
        expected_names.add(package["filename"])
        version_match = re.fullmatch(
            rf"{re.escape(upstream)}-1~{re.escape(SUITES[suite])}\.([1-9][0-9]{{0,8}})",
            package["version"],
        )
        if not version_match or not VERSION_RE.fullmatch(package["version"]):
            raise PublishError(f"unexpected internal version for {suite}: {package['version']!r}")
        rebuilds.add(version_match.group(1))
        if not isinstance(package["size"], int) or package["size"] <= 0:
            raise PublishError(f"invalid size for {suite}")
        if not isinstance(package["sha256"], str) or not SHA_RE.fullmatch(package["sha256"]):
            raise PublishError(f"invalid SHA-256 for {suite}")
        job_id = package["producing_job_id"]
        if not isinstance(job_id, int) or isinstance(job_id, bool) or job_id <= 0:
            raise PublishError(f"invalid producing job ID for {suite}")
        deb = files.get(package["filename"])
        if deb is None:
            raise PublishError(f"missing package: {package['filename']}")
        if deb.stat().st_size != package["size"] or sha256(deb) != package["sha256"]:
            raise PublishError(f"size or checksum mismatch: {package['filename']}")
        members = deb_members(deb)
        control_archives = {member for member in members if CONTROL_ARCHIVE_RE.fullmatch(member)}
        if "debian-binary" not in members or len(control_archives) != 1:
            raise PublishError(
                f"{package['filename']} must contain exactly one supported control.tar archive"
            )
        actual = {
            field: dpkg_field(deb, field, dpkg_deb)
            for field in ("Package", "Version", "Architecture")
        }
        if actual != {"Package": "winegui", "Version": package["version"], "Architecture": "amd64"}:
            raise PublishError(f"DEB control metadata mismatch for {suite}: {actual}")
        normalized.append({
            **package,
            "suite": suite,
            "package": "winegui",
            "architecture": "amd64",
            "job_id": job_id,
        })

    if set(files) != expected_names:
        raise PublishError(f"unexpected batch files: {sorted(set(files) - expected_names)}")
    if len(rebuilds) != 1:
        raise PublishError("all packages in a release batch must use the same rebuild counter")
    checksums = parse_checksums(files["SHA256SUMS"])
    expected_checksums = {package["filename"]: package["sha256"] for package in normalized}
    expected_checksums["manifest.json"] = sha256(files["manifest.json"])
    if checksums != expected_checksums:
        raise PublishError("SHA256SUMS does not exactly match the manifest and five packages")
    digest = hashlib.sha256(files["manifest.json"].read_bytes() + files["SHA256SUMS"].read_bytes()).hexdigest()
    return manifest, sorted(normalized, key=lambda item: item["suite"]), digest


def validate_deployer_batch(
    batch: Path, batch_id: str, dpkg_deb: str
) -> tuple[Path, dict, list[dict], str]:
    require_plain_directory(batch)
    with os.scandir(batch) as batch_entries:
        entries = {entry.name: entry for entry in batch_entries}
    if set(entries) != {"batch.json", PAYLOAD_DIRECTORY}:
        raise PublishError("deployer batch must contain only batch.json and apt_repository_batch/")
    if not entries["batch.json"].is_file(follow_symlinks=False):
        raise PublishError("deployer batch.json must be a regular file")
    payload = batch / PAYLOAD_DIRECTORY
    require_plain_directory(payload)

    envelope = load_json(batch / "batch.json")
    required = {
        "schema_version", "batch_id", "project_id", "deployment_id", "job_id",
        "environment", "sha", "completed_at", "artifact", "extracted_bytes", "files",
    }
    if not required <= set(envelope):
        raise PublishError("deployer batch.json is missing required identity or inventory fields")
    if envelope["schema_version"] != 1 or envelope["batch_id"] != batch_id:
        raise PublishError("deployer batch identity mismatch")
    if envelope["environment"] != "winegui-apt-repository":
        raise PublishError("deployer batch environment mismatch")
    for field in ("project_id", "deployment_id", "job_id"):
        if not isinstance(envelope[field], int) or isinstance(envelope[field], bool) or envelope[field] <= 0:
            raise PublishError(f"invalid deployer {field}")
    if not isinstance(envelope["sha"], str) or not COMMIT_RE.fullmatch(envelope["sha"]):
        raise PublishError("deployer batch sha must be a lowercase full Git commit hash")
    artifact = envelope["artifact"]
    if (
        not isinstance(artifact, dict)
        or set(artifact) != {"sha256", "bytes"}
        or not isinstance(artifact["sha256"], str)
        or not SHA_RE.fullmatch(artifact["sha256"])
        or not isinstance(artifact["bytes"], int)
        or isinstance(artifact["bytes"], bool)
        or artifact["bytes"] <= 0
    ):
        raise PublishError("invalid deployer artifact metadata")

    payload_files = list_plain_files(payload)
    inventory = envelope["files"]
    if not isinstance(inventory, list):
        raise PublishError("deployer file inventory must be an array")
    expected_inventory: dict[str, tuple[int, str]] = {}
    for name, path in payload_files.items():
        expected_inventory[f"{PAYLOAD_DIRECTORY}/{name}"] = (path.stat().st_size, sha256(path))
    actual_inventory: dict[str, tuple[int, str]] = {}
    for item in inventory:
        if not isinstance(item, dict) or set(item) != {"path", "bytes", "sha256"}:
            raise PublishError("invalid deployer file inventory entry")
        item_path = item["path"]
        if not isinstance(item_path, str) or item_path in actual_inventory:
            raise PublishError("invalid or duplicate deployer inventory path")
        if (
            not isinstance(item["bytes"], int)
            or isinstance(item["bytes"], bool)
            or item["bytes"] < 0
            or not isinstance(item["sha256"], str)
            or not SHA_RE.fullmatch(item["sha256"])
        ):
            raise PublishError("invalid deployer inventory size or checksum")
        actual_inventory[item_path] = (item["bytes"], item["sha256"])
    if actual_inventory != expected_inventory:
        raise PublishError("deployer inventory does not match its private copied payload")
    if envelope["extracted_bytes"] != sum(size for size, _ in expected_inventory.values()):
        raise PublishError("deployer extracted byte count mismatch")

    manifest, packages, payload_digest = validate_batch(payload, dpkg_deb)
    if manifest["commit_sha"] != envelope["sha"]:
        raise PublishError("deployer and release manifest commit SHAs disagree")
    digest = hashlib.sha256(
        (batch / "batch.json").read_bytes() + payload_digest.encode("ascii")
    ).hexdigest()
    return payload, manifest, packages, digest


class Publisher:
    def __init__(self) -> None:
        env = os.environ
        self.state = Path(env.get("WINEGUI_APT_STATE_ROOT", "/var/lib/winegui-apt"))
        self.spool = Path(env.get("WINEGUI_APT_SPOOL_ROOT", "/var/spool/winegui-apt"))
        self.public = Path(env.get("WINEGUI_APT_PUBLIC_ROOT", "/var/www/apt.winegui.melroy.org/html"))
        self.config = Path(env.get("WINEGUI_APT_REPREPRO_CONFIG", str(self.state / "reprepro/conf")))
        self.gnupg = Path(env.get("WINEGUI_APT_GNUPGHOME", str(self.state / "gnupg")))
        self.signing_key = env.get("WINEGUI_APT_SIGNING_KEY", "")
        self.reprepro = env.get("WINEGUI_APT_REPREPRO", "reprepro")
        self.dpkg_deb = env.get("WINEGUI_APT_DPKG_DEB", "dpkg-deb")
        self.gpg = env.get("WINEGUI_APT_GPG", "gpg")
        self.failpoint = env.get("WINEGUI_APT_TEST_FAILPOINT", "")
        self.journals = self.state / "journal"
        self.batches = self.state / "batches"
        self.work = self.state / "work"
        self.health_file = self.state / "health.json"

    def initialize(self) -> None:
        for path in (
            self.spool / "ready",
            self.spool / PUBLISHER_PROCESSING,
            self.spool / PUBLISHER_ARCHIVE,
            self.spool / PUBLISHER_QUARANTINE,
            self.journals,
            self.batches,
            self.work,
            self.public,
        ):
            path.mkdir(parents=True, exist_ok=True)

    def write_health(self, status: str, **extra: object) -> None:
        atomic_json(self.health_file, {"status": status, "timestamp": int(time.time()), **extra})

    def checkpoint(self, journal: dict, stage: str, **extra: object) -> None:
        journal.update(stage=stage, updated_at=int(time.time()), **extra)
        atomic_json(self.journals / f"{journal['batch_id']}.json", journal)
        if self.failpoint == stage:
            raise InjectedFailure(f"test failpoint after stage {stage}")

    def claim_ready(self) -> None:
        for source in sorted((self.spool / "ready").iterdir()):
            batch_id = source.name
            if not BATCH_ID_RE.fullmatch(batch_id):
                self.quarantine_unclaimed(source, "unsafe batch ID")
                continue
            try:
                require_plain_directory(source)
            except (OSError, PublishError):
                self.quarantine_unclaimed(source, "batch must be a plain directory")
                continue
            destination = self.spool / PUBLISHER_PROCESSING / batch_id
            journal_path = self.journals / f"{batch_id}.json"
            if journal_path.exists():
                journal = load_json(journal_path)
            else:
                journal = {"batch_id": batch_id, "created_at": int(time.time())}
                self.checkpoint(journal, "claiming")
            if destination.exists():
                self.quarantine_unclaimed(source, "duplicate active batch ID")
                continue
            try:
                os.rename(source, destination)
            except OSError as exc:
                if exc.errno in (errno.EEXIST, errno.ENOTEMPTY):
                    self.quarantine_unclaimed(source, "duplicate active batch ID")
                    continue
                raise
            if self.failpoint == "renamed":
                raise InjectedFailure("test failpoint after atomic ready-to-processing rename")
            if journal.get("stage") == "complete":
                self.handle_completed_retry(destination, journal)
                continue
            self.checkpoint(journal, "claimed")

    def quarantine_unclaimed(self, source: Path, reason: str) -> None:
        target = self.unique_destination(self.spool / PUBLISHER_QUARANTINE, source.name)
        os.rename(source, target)
        target_mode = target.lstat().st_mode
        reason_path = (
            target / "QUARANTINE_REASON.txt"
            if stat.S_ISDIR(target_mode) and not stat.S_ISLNK(target_mode)
            else target.with_name(target.name + ".reason.txt")
        )
        reason_path.write_text(reason + "\n", encoding="utf-8")

    @staticmethod
    def unique_destination(parent: Path, name: str) -> Path:
        candidate = parent / name
        counter = 1
        while candidate.exists():
            candidate = parent / f"{name}.{counter}"
            counter += 1
        return candidate

    def handle_completed_retry(self, source: Path, journal: dict) -> None:
        retry = self.batches / f"{source.name}.retry"
        if retry.exists():
            shutil.rmtree(retry)
        copy_batch(source, retry)
        try:
            _, _, _, digest = validate_deployer_batch(retry, source.name, self.dpkg_deb)
            if digest != journal.get("batch_digest"):
                raise PublishError("completed batch ID was reused with different content")
            target = self.unique_destination(self.spool / PUBLISHER_ARCHIVE, f"{source.name}.retry")
            os.rename(source, target)
        except PublishError as exc:
            self.quarantine(source.name, str(exc))
        finally:
            shutil.rmtree(retry, ignore_errors=True)

    def quarantine(self, batch_id: str, reason: str) -> None:
        source = self.spool / PUBLISHER_PROCESSING / batch_id
        if source.exists():
            target = self.unique_destination(self.spool / PUBLISHER_QUARANTINE, batch_id)
            os.rename(source, target)
            (target / "QUARANTINE_REASON.txt").write_text(reason + "\n", encoding="utf-8")
        journal_path = self.journals / f"{batch_id}.json"
        journal = load_json(journal_path) if journal_path.exists() else {"batch_id": batch_id}
        self.checkpoint(journal, "quarantined", error=reason)

    def process(self, batch_id: str) -> None:
        source = self.spool / PUBLISHER_PROCESSING / batch_id
        journal_path = self.journals / f"{batch_id}.json"
        journal = load_json(journal_path)
        private = self.batches / batch_id
        try:
            if journal["stage"] == "claimed":
                copy_batch(source, private)
                self.checkpoint(journal, "copied")
            payload, manifest, packages, digest = validate_deployer_batch(
                private, batch_id, self.dpkg_deb
            )
            if journal["stage"] in ("copied", "validated"):
                self.checkpoint(journal, "validated", batch_digest=digest, release_tag=manifest["release_tag"])
        except PublishError as exc:
            self.quarantine(batch_id, str(exc))
            return
        try:
            workspace = self.work / batch_id / "repository"
            if journal["stage"] in ("validated", "importing"):
                workspace.mkdir(parents=True, exist_ok=True)
                self.checkpoint(journal, "importing")
                self.import_packages(payload, packages, workspace)
                self.checkpoint(journal, "imported")
            if journal["stage"] in ("imported", "exporting"):
                self.checkpoint(journal, "exporting")
                self.export_and_prepare(workspace)
                self.checkpoint(journal, "prepared")
            if journal["stage"] in ("prepared", "signing"):
                self.checkpoint(journal, "signing")
                self.sign(workspace)
                self.checkpoint(journal, "signed")
            if journal["stage"] in ("signed", "publishing"):
                self.checkpoint(journal, "publishing")
                self.publish(workspace)
                self.checkpoint(journal, "published")
            if journal["stage"] == "published":
                archive = self.unique_destination(self.spool / PUBLISHER_ARCHIVE, batch_id)
                os.rename(source, archive)
                self.checkpoint(journal, "complete", completed_at=int(time.time()))
        except PublishError as exc:
            self.checkpoint(journal, "failed", error=str(exc), resume_stage=journal["stage"])
            raise

    def command_prefix(self, workspace: Path) -> list[str]:
        return [
            self.reprepro,
            "--basedir",
            str(self.state / "reprepro"),
            "--confdir",
            str(self.config),
            "--dbdir",
            str(self.state / "reprepro/db"),
            "--outdir",
            str(workspace),
        ]

    def find_existing(self, package: dict, pool: Path) -> Path | None:
        if not pool.exists():
            return None
        for path in pool.rglob("*.deb"):
            if (
                dpkg_field(path, "Package", self.dpkg_deb) == package["package"]
                and dpkg_field(path, "Version", self.dpkg_deb) == package["version"]
                and dpkg_field(path, "Architecture", self.dpkg_deb) == package["architecture"]
            ):
                return path
        return None

    def import_packages(self, private: Path, packages: list[dict], workspace: Path) -> None:
        for package in packages:
            staged = self.find_existing(package, workspace / "pool")
            if staged and sha256(staged) != package["sha256"]:
                raise PublishError(
                    f"changed bytes for staged identity winegui/{package['version']}/amd64"
                )
            existing = self.find_existing(package, self.public / "pool")
            if existing:
                if sha256(existing) != package["sha256"]:
                    raise PublishError(
                        f"changed bytes for existing identity winegui/{package['version']}/amd64"
                    )
                continue
            # Re-run inclusion after a crash even if the pool object was staged:
            # the pinned engine treats an already-committed identical version as
            # a successful no-op, while a pre-commit crash still needs the DB write.
            run(
                self.command_prefix(workspace)
                + ["--export=never", "includedeb", package["suite"], str(private / package["filename"])]
            )

    def export_and_prepare(self, workspace: Path) -> None:
        run(self.command_prefix(workspace) + ["export"] + sorted(SUITES))
        for suite in SUITES:
            release = workspace / "dists" / suite / "Release"
            if not release.is_file():
                raise PublishError(f"reprepro did not generate {release}")
            text = release.read_text(encoding="utf-8")
            lines = [line for line in text.splitlines() if not line.startswith("Acquire-By-Hash:")]
            lines.insert(0, "Acquire-By-Hash: yes")
            release.write_text("\n".join(lines) + "\n", encoding="utf-8")
            for index in (workspace / "dists" / suite).rglob("*"):
                if not index.is_file() or "by-hash" in index.parts or index.name in {
                    "Release",
                    "Release.gpg",
                    "InRelease",
                }:
                    continue
                digest = sha256(index)
                target = index.parent / "by-hash" / "SHA256" / digest
                target.parent.mkdir(parents=True, exist_ok=True)
                if not target.exists():
                    shutil.copyfile(index, target)

    def sign(self, workspace: Path) -> None:
        if not self.signing_key:
            raise PublishError("WINEGUI_APT_SIGNING_KEY is required")
        env = os.environ.copy()
        env["GNUPGHOME"] = str(self.gnupg)
        for suite in SUITES:
            directory = workspace / "dists" / suite
            release = directory / "Release"
            run(
                [
                    self.gpg,
                    "--batch",
                    "--yes",
                    "--local-user",
                    self.signing_key,
                    "--output",
                    str(directory / "Release.gpg"),
                    "--detach-sign",
                    str(release),
                ],
                env=env,
            )
            run(
                [
                    self.gpg,
                    "--batch",
                    "--yes",
                    "--local-user",
                    self.signing_key,
                    "--output",
                    str(directory / "InRelease"),
                    "--clearsign",
                    str(release),
                ],
                env=env,
            )

    @staticmethod
    def install_immutable(source: Path, target: Path) -> None:
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            if sha256(source) != sha256(target):
                raise PublishError(f"immutable publication conflict: {target}")
            return
        temporary = target.with_name(f".{target.name}.{os.getpid()}.tmp")
        shutil.copyfile(source, temporary)
        os.chmod(temporary, 0o644)
        os.replace(temporary, target)

    @staticmethod
    def replace_file(source: Path, target: Path) -> None:
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = target.with_name(f".{target.name}.{os.getpid()}.tmp")
        shutil.copyfile(source, temporary)
        os.chmod(temporary, 0o644)
        os.replace(temporary, target)

    def publish(self, workspace: Path) -> None:
        # Immutable package objects and by-hash indexes become visible first.
        pool = workspace / "pool"
        if pool.exists():
            for source in sorted(path for path in pool.rglob("*") if path.is_file()):
                self.install_immutable(source, self.public / source.relative_to(workspace))
        by_hash = sorted(
            path for path in (workspace / "dists").rglob("*") if path.is_file() and "by-hash" in path.parts
        )
        for source in by_hash:
            self.install_immutable(source, self.public / source.relative_to(workspace))

        # Canonical indexes may change. Release signatures are deliberately last.
        ordinary = []
        releases = []
        detached = []
        inline = []
        for source in sorted(path for path in (workspace / "dists").rglob("*") if path.is_file()):
            if "by-hash" in source.parts:
                continue
            if source.name == "InRelease":
                inline.append(source)
            elif source.name == "Release.gpg":
                detached.append(source)
            elif source.name == "Release":
                releases.append(source)
            else:
                ordinary.append(source)
        for group in (ordinary, releases, detached, inline):
            for source in group:
                self.replace_file(source, self.public / source.relative_to(workspace))

    def run_all(self) -> None:
        self.initialize()
        self.reconcile_claims()
        self.reconcile_archived()
        self.claim_ready()
        failed = []
        for source in sorted((self.spool / PUBLISHER_PROCESSING).iterdir()):
            if not source.is_dir() or not BATCH_ID_RE.fullmatch(source.name):
                continue
            journal_path = self.journals / f"{source.name}.json"
            if not journal_path.exists():
                self.quarantine(source.name, "processing batch has no journal")
                continue
            journal = load_json(journal_path)
            if journal.get("stage") in ("quarantined", "complete"):
                continue
            if journal.get("stage") == "failed":
                # Resume from the last durable operation according to available artifacts.
                previous = journal.get("resume_stage") or self.infer_resume_stage(source.name)
                self.checkpoint(journal, previous, error=None)
            try:
                self.process(source.name)
            except PublishError as exc:
                failed.append({"batch_id": source.name, "error": str(exc)})
        if failed:
            self.write_health("failed", failures=failed)
            raise PublishError(f"{len(failed)} batch(es) failed")
        self.write_health("ok")

    def reconcile_claims(self) -> None:
        for journal_path in self.journals.glob("*.json"):
            journal = load_json(journal_path)
            if journal.get("stage") != "claiming":
                continue
            batch_id = journal.get("batch_id", "")
            if not BATCH_ID_RE.fullmatch(batch_id):
                continue
            if (self.spool / PUBLISHER_PROCESSING / batch_id).is_dir():
                self.checkpoint(journal, "claimed")

    def reconcile_archived(self) -> None:
        for journal_path in self.journals.glob("*.json"):
            journal = load_json(journal_path)
            if journal.get("stage") != "published":
                continue
            batch_id = journal.get("batch_id", "")
            if BATCH_ID_RE.fullmatch(batch_id) and (self.spool / PUBLISHER_ARCHIVE / batch_id).is_dir():
                self.checkpoint(journal, "complete", completed_at=int(time.time()))

    def infer_resume_stage(self, batch_id: str) -> str:
        workspace = self.work / batch_id / "repository"
        private = self.batches / batch_id
        if any((workspace / "dists" / suite / "InRelease").exists() for suite in SUITES):
            return "signed"
        if any((workspace / "dists" / suite / "Release").exists() for suite in SUITES):
            return "imported"
        if private.exists():
            return "copied"
        return "claimed"

    def health(self, max_age: int) -> int:
        problems = []
        now = time.time()
        for area in ("ready", PUBLISHER_PROCESSING):
            directory = self.spool / area
            if directory.exists():
                for entry in directory.iterdir():
                    if now - entry.stat().st_mtime > max_age:
                        problems.append(f"stale {area} batch: {entry.name}")
        quarantine = self.spool / PUBLISHER_QUARANTINE
        if quarantine.exists() and any(quarantine.iterdir()):
            problems.append("quarantine is not empty")
        if self.health_file.exists():
            status = load_json(self.health_file)
            if status.get("status") != "ok":
                problems.append("last publisher run failed")
        else:
            problems.append("publisher health file is absent")
        for problem in problems:
            print(problem, file=sys.stderr)
        return 1 if problems else 0


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("run")
    health = subparsers.add_parser("health")
    health.add_argument("--max-age", type=int, default=3600)
    args = parser.parse_args()
    publisher = Publisher()
    try:
        if args.command == "run":
            publisher.run_all()
            return 0
        return publisher.health(args.max_age)
    except (OSError, PublishError) as exc:
        print(f"winegui-apt-publisher: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
