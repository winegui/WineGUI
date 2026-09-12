#!/usr/bin/env python3

import gzip
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

MODULE_PATH = Path(__file__).parents[1] / "publisher" / "winegui_apt_publisher.py"
SPEC = importlib.util.spec_from_file_location("publisher", MODULE_PATH)
publisher = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(publisher)


class PublisherTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.state = self.root / "state"
        self.spool = self.root / "spool"
        self.public = self.root / "public"
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.reprepro = self.bin / "reprepro"
        self.gpg = self.bin / "gpg"
        self._write_fakes()
        self.environment = {
            "WINEGUI_APT_STATE_ROOT": str(self.state),
            "WINEGUI_APT_SPOOL_ROOT": str(self.spool),
            "WINEGUI_APT_PUBLIC_ROOT": str(self.public),
            "WINEGUI_APT_REPREPRO_CONFIG": str(Path(__file__).parents[1] / "config/reprepro/conf"),
            "WINEGUI_APT_GNUPGHOME": str(self.root / "gnupg"),
            "WINEGUI_APT_SIGNING_KEY": "TEST-FINGERPRINT",
            "WINEGUI_APT_REPREPRO": str(self.reprepro),
            "WINEGUI_APT_GPG": str(self.gpg),
            "WINEGUI_APT_DPKG_DEB": shutil.which("dpkg-deb"),
        }

    def tearDown(self):
        self.temporary.cleanup()

    def _write_fakes(self):
        self.reprepro.write_text(
            """#!/bin/sh
set -eu
out=
while [ $# -gt 0 ]; do
  case "$1" in
    --outdir) out=$2; shift 2 ;;
    --basedir|--confdir|--dbdir) shift 2 ;;
    --export=never) shift ;;
    includedeb)
      suite=$2; deb=$3
      mkdir -p "$out/pool/main/w/winegui"
      cp "$deb" "$out/pool/main/w/winegui/$(dpkg-deb -f "$deb" Package)_$(dpkg-deb -f "$deb" Version)_amd64.deb"
      exit 0 ;;
    export)
      shift
      for suite in "$@"; do
        dir="$out/dists/$suite/main/binary-amd64"
        mkdir -p "$dir"
        printf 'Package: winegui\\nSuite: %s\\n' "$suite" > "$dir/Packages"
        gzip -n -c "$dir/Packages" > "$dir/Packages.gz"
        printf 'Suite: %s\\nSHA256:\\n' "$suite" > "$out/dists/$suite/Release"
      done
      exit 0 ;;
  esac
done
exit 2
""",
            encoding="utf-8",
        )
        self.gpg.write_text(
            """#!/bin/sh
set -eu
output=
input=
while [ $# -gt 0 ]; do
  case "$1" in
    --output) output=$2; shift 2 ;;
    --detach-sign|--clearsign) shift; input=$1; shift ;;
    --local-user) shift 2 ;;
    --batch|--yes) shift ;;
    *) shift ;;
  esac
done
printf 'fake signature for %s\\n' "$input" > "$output"
""",
            encoding="utf-8",
        )
        self.reprepro.chmod(0o755)
        self.gpg.chmod(0o755)

    def build_deb(self, path, version):
        tree = self.root / f"deb-{version}"
        (tree / "DEBIAN").mkdir(parents=True)
        (tree / "DEBIAN/control").write_text(
            f"Package: winegui\nVersion: {version}\nArchitecture: amd64\nMaintainer: Test <test@example.invalid>\nDescription: Test\n",
            encoding="utf-8",
        )
        subprocess.run(["dpkg-deb", "-Zgzip", "--build", str(tree), str(path)], check=True, capture_output=True)

    def make_batch(self, batch_id="batch-1"):
        ready = self.spool / "ready" / batch_id
        payload = ready / publisher.PAYLOAD_DIRECTORY
        payload.mkdir(parents=True)
        packages = {}
        checksums = []
        for index, (suite, suffix) in enumerate(publisher.SUITES.items(), 1):
            filename = f"WineGUI-v4.3.2-{suite}.deb"
            version = f"4.3.2-1~{suffix}.1"
            deb = payload / filename
            self.build_deb(deb, version)
            digest = hashlib.sha256(deb.read_bytes()).hexdigest()
            packages[suite] = {
                    "filename": filename,
                    "version": version,
                    "size": deb.stat().st_size,
                    "sha256": digest,
                    "producing_job_id": 100 + index,
                }
            checksums.append(f"{digest}  {filename}")
        manifest = {
            "schema_version": 1,
            "release_tag": "v4.3.2",
            "commit_sha": "a" * 40,
            "architecture": "amd64",
            "packages": packages,
        }
        (payload / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
        checksums.append(
            f"{hashlib.sha256((payload / 'manifest.json').read_bytes()).hexdigest()}  manifest.json"
        )
        (payload / "SHA256SUMS").write_text("\n".join(checksums) + "\n", encoding="ascii")
        self.refresh_envelope(ready, batch_id)
        return ready

    def refresh_envelope(self, ready, batch_id="batch-1"):
        payload = ready / publisher.PAYLOAD_DIRECTORY
        inventory = []
        extracted = 0
        for path in sorted(payload.iterdir()):
            size = path.stat().st_size
            extracted += size
            inventory.append({
                "path": f"{publisher.PAYLOAD_DIRECTORY}/{path.name}",
                "bytes": size,
                "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            })
        envelope = {
            "schema_version": 1,
            "batch_id": batch_id,
            "project_id": 66,
            "deployment_id": 200,
            "job_id": 300,
            "environment": "winegui-apt-repository",
            "sha": "a" * 40,
            "completed_at": "2026-09-12T12:00:00Z",
            "artifact": {"sha256": "b" * 64, "bytes": extracted + 100},
            "extracted_bytes": extracted,
            "files": inventory,
        }
        (ready / "batch.json").write_text(json.dumps(envelope), encoding="utf-8")

    def instance(self):
        with mock.patch.dict(os.environ, self.environment, clear=False):
            return publisher.Publisher()

    def test_complete_publication_and_by_hash(self):
        self.make_batch()
        instance = self.instance()
        instance.run_all()
        journal = json.loads((self.state / "journal/batch-1.json").read_text())
        self.assertEqual("complete", journal["stage"])
        self.assertTrue((self.spool / publisher.PUBLISHER_ARCHIVE / "batch-1").is_dir())
        for suite in publisher.SUITES:
            release = (self.public / f"dists/{suite}/Release").read_text()
            self.assertTrue(release.startswith("Acquire-By-Hash: yes\n"))
            package_index = self.public / f"dists/{suite}/main/binary-amd64/Packages.gz"
            digest = hashlib.sha256(package_index.read_bytes()).hexdigest()
            self.assertTrue((package_index.parent / f"by-hash/SHA256/{digest}").is_file())
            self.assertTrue((self.public / f"dists/{suite}/InRelease").is_file())

    def test_invalid_checksum_is_quarantined_before_import(self):
        ready = self.make_batch()
        payload = ready / publisher.PAYLOAD_DIRECTORY
        (payload / "SHA256SUMS").write_text("0" * 64 + "  broken.deb\n", encoding="ascii")
        self.refresh_envelope(ready)
        instance = self.instance()
        instance.run_all()
        self.assertFalse((self.public / "pool").exists())
        self.assertTrue((self.spool / publisher.PUBLISHER_QUARANTINE / "batch-1/QUARANTINE_REASON.txt").is_file())
        self.assertEqual("quarantined", json.loads((self.state / "journal/batch-1.json").read_text())["stage"])

    def test_symlink_is_rejected_before_private_copy(self):
        ready = self.make_batch()
        victim = next((ready / publisher.PAYLOAD_DIRECTORY).glob("*.deb"))
        victim.unlink()
        victim.symlink_to("/etc/passwd")
        instance = self.instance()
        instance.run_all()
        self.assertFalse((self.public / "pool").exists())

    def test_deployer_environment_mismatch_is_quarantined(self):
        ready = self.make_batch()
        envelope_path = ready / "batch.json"
        envelope = json.loads(envelope_path.read_text())
        envelope["environment"] = "production"
        envelope_path.write_text(json.dumps(envelope), encoding="utf-8")
        self.instance().run_all()
        self.assertFalse((self.public / "pool").exists())
        reason = (self.spool / publisher.PUBLISHER_QUARANTINE / "batch-1/QUARANTINE_REASON.txt").read_text()
        self.assertIn("environment mismatch", reason)

    def test_deployer_inventory_mismatch_is_quarantined(self):
        ready = self.make_batch()
        payload = ready / publisher.PAYLOAD_DIRECTORY
        with (payload / "manifest.json").open("ab") as stream:
            stream.write(b"\n")
        self.instance().run_all()
        self.assertFalse((self.public / "pool").exists())
        reason = (self.spool / publisher.PUBLISHER_QUARANTINE / "batch-1/QUARANTINE_REASON.txt").read_text()
        self.assertIn("inventory does not match", reason)

    def test_completed_identical_retry_is_archived_without_republish(self):
        original = self.make_batch()
        snapshot = self.root / "snapshot"
        shutil.copytree(original, snapshot)
        instance = self.instance()
        instance.run_all()
        shutil.copytree(snapshot, self.spool / "ready/batch-1")
        instance.run_all()
        retries = list((self.spool / publisher.PUBLISHER_ARCHIVE).glob("batch-1.retry*"))
        self.assertEqual(1, len(retries))

    def test_signing_failure_recovers_from_journal(self):
        self.make_batch()
        environment = dict(self.environment)
        environment["WINEGUI_APT_TEST_FAILPOINT"] = "signing"
        with mock.patch.dict(os.environ, environment, clear=False):
            first = publisher.Publisher()
        with self.assertRaises(publisher.InjectedFailure):
            first.run_all()
        second = self.instance()
        second.run_all()
        self.assertEqual("complete", json.loads((self.state / "journal/batch-1.json").read_text())["stage"])

    def test_failure_after_publication_recovers_without_reimport(self):
        self.make_batch()
        environment = dict(self.environment)
        environment["WINEGUI_APT_TEST_FAILPOINT"] = "published"
        with mock.patch.dict(os.environ, environment, clear=False):
            first = publisher.Publisher()
        with self.assertRaises(publisher.InjectedFailure):
            first.run_all()
        second = self.instance()
        second.run_all()
        self.assertEqual("complete", json.loads((self.state / "journal/batch-1.json").read_text())["stage"])

    def test_zstd_control_member_is_quarantined_for_pinned_engine(self):
        ready = self.make_batch()
        payload = ready / publisher.PAYLOAD_DIRECTORY
        manifest = json.loads((payload / "manifest.json").read_text())
        package = manifest["packages"]["noble"]
        deb = payload / package["filename"]
        tree = self.root / "zstd-deb"
        (tree / "DEBIAN").mkdir(parents=True)
        (tree / "DEBIAN/control").write_text(
            f"Package: winegui\nVersion: {package['version']}\nArchitecture: amd64\nMaintainer: Test <test@example.invalid>\nDescription: Test\n",
            encoding="utf-8",
        )
        subprocess.run(["dpkg-deb", "-Zzstd", "--build", str(tree), str(deb)], check=True, capture_output=True)
        package["size"] = deb.stat().st_size
        package["sha256"] = hashlib.sha256(deb.read_bytes()).hexdigest()
        (payload / "manifest.json").write_text(json.dumps(manifest), encoding="utf-8")
        checksums = [f"{item['sha256']}  {item['filename']}" for item in manifest["packages"].values()]
        checksums.append(
            f"{hashlib.sha256((payload / 'manifest.json').read_bytes()).hexdigest()}  manifest.json"
        )
        (payload / "SHA256SUMS").write_text("\n".join(checksums) + "\n", encoding="ascii")
        self.refresh_envelope(ready)
        instance = self.instance()
        instance.run_all()
        reason = (self.spool / publisher.PUBLISHER_QUARANTINE / "batch-1/QUARANTINE_REASON.txt").read_text()
        self.assertIn("control.tar.gz", reason)

    def test_restart_after_claim_rename_recovers(self):
        self.make_batch()
        environment = dict(self.environment)
        environment["WINEGUI_APT_TEST_FAILPOINT"] = "renamed"
        with mock.patch.dict(os.environ, environment, clear=False):
            first = publisher.Publisher()
        with self.assertRaises(publisher.InjectedFailure):
            first.run_all()
        self.assertTrue((self.spool / publisher.PUBLISHER_PROCESSING / "batch-1").is_dir())
        second = self.instance()
        second.run_all()
        self.assertEqual("complete", json.loads((self.state / "journal/batch-1.json").read_text())["stage"])

    def test_non_default_rebuild_counter_is_accepted(self):
        ready = self.make_batch()
        payload = ready / publisher.PAYLOAD_DIRECTORY
        manifest_path = payload / "manifest.json"
        manifest = json.loads(manifest_path.read_text())
        checksums = []
        for package in manifest["packages"].values():
            deb = payload / package["filename"]
            version = package["version"].rsplit(".", 1)[0] + ".2"
            self.build_deb(deb, version)
            package["version"] = version
            package["size"] = deb.stat().st_size
            package["sha256"] = hashlib.sha256(deb.read_bytes()).hexdigest()
            checksums.append(f"{package['sha256']}  {package['filename']}")
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        checksums.append(f"{hashlib.sha256(manifest_path.read_bytes()).hexdigest()}  manifest.json")
        (payload / "SHA256SUMS").write_text("\n".join(checksums) + "\n", encoding="ascii")
        self.refresh_envelope(ready)
        self.instance().run_all()
        self.assertTrue((self.spool / publisher.PUBLISHER_ARCHIVE / "batch-1").is_dir())


if __name__ == "__main__":
    unittest.main()
