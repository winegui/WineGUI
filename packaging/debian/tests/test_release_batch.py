#!/usr/bin/env python3

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


DEBIAN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(DEBIAN_DIR))
from package import SUITES, debian_version  # noqa: E402


class ReleaseBatchTests(unittest.TestCase):
    def setUp(self):
        self.temporary = Path(tempfile.mkdtemp(prefix="winegui-batch-test-"))
        self.input = self.temporary / "input"
        self.input.mkdir()
        self.output = self.temporary / "batch"
        for job_id, suite in enumerate(sorted(SUITES), start=101):
            package_root = self.temporary / f"package-{suite}"
            (package_root / "DEBIAN").mkdir(parents=True)
            version = debian_version("4.3.2", suite)
            (package_root / "DEBIAN/control").write_text(
                "Package: winegui\n"
                f"Version: {version}\n"
                "Architecture: amd64\n"
                "Maintainer: WineGUI Test <test@invalid.example>\n"
                "Description: WineGUI packaging fixture\n"
            )
            deb = self.input / f"WineGUI-v4.3.2-{suite}.deb"
            subprocess.run(["dpkg-deb", "--build", str(package_root), str(deb)], check=True, capture_output=True)
            subprocess.run([
                sys.executable, str(DEBIAN_DIR / "verify-package.py"),
                "--deb", str(deb), "--suite", suite,
                "--upstream-version", "4.3.2", "--expected-version", version,
                "--tag", "v4.3.2",
                "--job-id", str(job_id),
                "--output", str(self.input / f"debian-package-{suite}.json"),
            ], check=True)

    def tearDown(self):
        shutil.rmtree(self.temporary)

    def create(self, expect_success=True):
        result = subprocess.run([
            sys.executable, str(DEBIAN_DIR / "create-release-batch.py"),
            "--input", str(self.input), "--output", str(self.output),
            "--tag", "v4.3.2", "--commit", "a" * 40,
        ], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if expect_success and result.returncode != 0:
            self.fail(result.stderr)
        return result

    def test_batch_has_exact_payload_and_manifest(self):
        self.create()
        self.assertEqual(
            {path.name for path in self.output.iterdir()},
            {f"WineGUI-v4.3.2-{suite}.deb" for suite in SUITES} | {"manifest.json", "SHA256SUMS"},
        )
        manifest = json.loads((self.output / "manifest.json").read_text())
        self.assertEqual(manifest["release_tag"], "v4.3.2")
        self.assertEqual(manifest["commit_sha"], "a" * 40)
        self.assertEqual(set(manifest["packages"]), set(SUITES))
        sums = {}
        for line in (self.output / "SHA256SUMS").read_text().splitlines():
            digest, filename = line.split("  ", 1)
            sums[filename] = digest
        self.assertEqual(set(sums), {path.name for path in self.output.iterdir()} - {"SHA256SUMS"})
        for filename, digest in sums.items():
            self.assertEqual(hashlib.sha256((self.output / filename).read_bytes()).hexdigest(), digest)

    def test_tampered_deb_is_rejected_before_output(self):
        with (self.input / "WineGUI-v4.3.2-noble.deb").open("ab") as stream:
            stream.write(b"tamper")
        result = self.create(expect_success=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("size/checksum mismatch", result.stderr)
        self.assertFalse(self.output.exists())

    def test_missing_producer_metadata_is_rejected(self):
        (self.input / "debian-package-noble.json").unlink()
        result = self.create(expect_success=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("producer metadata", result.stderr)


if __name__ == "__main__":
    unittest.main()
