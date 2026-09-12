#!/usr/bin/env python3

import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


DEBIAN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(DEBIAN_DIR))
from package import SUITES, debian_version  # noqa: E402


class PackagingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.keys = Path(tempfile.mkdtemp(prefix="winegui-test-keys-"))
        os.chmod(cls.keys, 0o700)
        subprocess.run([
            "gpg", "--batch", "--no-options", "--homedir", str(cls.keys),
            "--pinentry-mode", "loopback", "--passphrase", "",
            "--quick-generate-key", "WineGUI Packaging Test <test@invalid.example>",
            "ed25519", "cert", "1d",
        ], check=True, capture_output=True)
        listing = subprocess.check_output([
            "gpg", "--batch", "--no-options", "--homedir", str(cls.keys),
            "--with-colons", "--list-keys",
        ], text=True)
        cls.fingerprint = next(
            line.split(":")[9] for line in listing.splitlines() if line.startswith("fpr:")
        )
        subprocess.run([
            "gpg", "--batch", "--no-options", "--homedir", str(cls.keys),
            "--pinentry-mode", "loopback", "--passphrase", "",
            "--quick-add-key", cls.fingerprint, "ed25519", "sign", "1d",
        ], check=True, capture_output=True)
        cls.public_key = cls.keys / "public.gpg"
        with cls.public_key.open("wb") as output:
            subprocess.run([
                "gpg", "--batch", "--no-options", "--homedir", str(cls.keys),
                "--export", cls.fingerprint,
            ], check=True, stdout=output)
        cls.secret_key = cls.keys / "secret.gpg"
        with cls.secret_key.open("wb") as output:
            subprocess.run([
                "gpg", "--batch", "--no-options", "--homedir", str(cls.keys),
                "--pinentry-mode", "loopback", "--passphrase", "",
                "--export-secret-keys", cls.fingerprint,
            ], check=True, stdout=output)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.keys)

    def setUp(self):
        self.temporary = Path(tempfile.mkdtemp(prefix="winegui-packaging-test-"))

    def tearDown(self):
        shutil.rmtree(self.temporary)

    def os_release(self, suite):
        family, version = SUITES[suite]
        root = self.temporary / f"root-{suite}"
        (root / "etc").mkdir(parents=True, exist_ok=True)
        (root / "etc/os-release").write_text(
            f"ID={family}\nVERSION_ID=\"{version}\"\nVERSION_CODENAME={suite}\n"
        )
        return root

    def runtime_os_release(self, name, content):
        root = self.temporary / f"runtime-{name}"
        (root / "etc").mkdir(parents=True, exist_ok=True)
        (root / "etc/os-release").write_text(content)
        return root

    def generate(self, suite="noble", generation=1, enabled=True, output_name=None):
        root = self.os_release(suite)
        output = self.temporary / (output_name or f"control-{suite}-{generation}")
        command = [
            sys.executable, str(DEBIAN_DIR / "package.py"),
            "--version", "4.3.2", "--output", str(output),
            "--os-release", str(root / "etc/os-release"), "--rebuild", "1",
            "--enabled", "1" if enabled else "0", "--tag", "v4.3.2",
        ]
        if enabled:
            command += [
                "--key-file", str(self.public_key),
                "--fingerprints", self.fingerprint,
                "--generation", str(generation),
            ]
        subprocess.run(command, check=True)
        return root, output

    def run_script(self, script, root, action):
        environment = os.environ.copy()
        environment["WINEGUI_MAINTAINER_ROOT"] = str(root)
        return subprocess.run(
            [str(script), action], env=environment, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True,
        )

    def test_versions_are_suite_specific_without_rank_table(self):
        expected = {
            "noble": "4.3.2-1~ubuntu24.04.1",
            "plucky": "4.3.2-1~ubuntu25.04.1",
            "resolute": "4.3.2-1~ubuntu26.04.1",
            "trixie": "4.3.2-1~debian13.1",
            "forky": "4.3.2-1~debian14.1",
        }
        self.assertEqual({suite: debian_version("4.3.2", suite) for suite in SUITES}, expected)
        self.assertEqual(len(set(expected.values())), len(SUITES))
        subprocess.run([
            "dpkg", "--compare-versions",
            debian_version("4.3.2", "noble", "2"), "gt",
            debian_version("4.3.2", "noble", "1"),
        ], check=True)

    def test_repository_build_fails_closed_without_key(self):
        root = self.os_release("noble")
        result = subprocess.run([
            sys.executable, str(DEBIAN_DIR / "package.py"),
            "--version", "4.3.2", "--output", str(self.temporary / "missing-key"),
            "--os-release", str(root / "etc/os-release"), "--enabled", "1",
        ], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("public key file, fingerprints and generation are required", result.stderr)

    def test_repository_build_rejects_secret_key_material(self):
        root = self.os_release("noble")
        result = subprocess.run([
            sys.executable, str(DEBIAN_DIR / "package.py"),
            "--version", "4.3.2", "--output", str(self.temporary / "secret-key"),
            "--os-release", str(root / "etc/os-release"), "--enabled", "1",
            "--key-file", str(self.secret_key), "--fingerprints", self.fingerprint,
            "--generation", "1",
        ], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("secret key material", result.stderr)

    def test_non_repository_package_is_usable_without_key(self):
        _, output = self.generate(enabled=False)
        self.assertEqual((output / "version.txt").read_text(), "4.3.2-1~ubuntu24.04.1")
        self.assertIn("APT_ENABLED='0'", (output / "postinst").read_text())

    def test_managed_source_opt_out_and_purge(self):
        root, output = self.generate()
        postinst, postrm = output / "postinst", output / "postrm"
        self.run_script(postinst, root, "configure")
        source = root / "etc/apt/sources.list.d/winegui.sources"
        key = root / "usr/share/keyrings/winegui-archive-keyring.gpg"
        self.assertIn("Suites: noble", source.read_text())
        self.assertEqual(source.stat().st_mode & 0o777, 0o644)
        self.assertEqual(key.stat().st_mode & 0o777, 0o644)
        source.unlink()
        self.run_script(postinst, root, "configure")
        self.assertFalse(source.exists(), "manual deletion must remain an opt-out")
        self.run_script(postrm, root, "purge")
        self.assertFalse(key.exists())

    def test_unmodified_managed_source_is_removed_on_purge(self):
        root, output = self.generate()
        self.run_script(output / "postinst", root, "configure")
        source = root / "etc/apt/sources.list.d/winegui.sources"
        key = root / "usr/share/keyrings/winegui-archive-keyring.gpg"
        self.run_script(output / "postrm", root, "purge")
        self.assertFalse(source.exists())
        self.assertFalse(key.exists())
        self.assertFalse((root / "var/lib/winegui").exists())

    def test_managed_source_tracks_supported_os_upgrade(self):
        root, output = self.generate()
        self.run_script(output / "postinst", root, "configure")
        (root / "etc/os-release").write_text(
            "ID=debian\nVERSION_ID=13\nVERSION_CODENAME=trixie\n"
        )
        self.run_script(output / "postinst", root, "configure")
        source = root / "etc/apt/sources.list.d/winegui.sources"
        self.assertIn("Suites: trixie", source.read_text())

    def test_supported_ubuntu_and_debian_derivatives_use_upstream_suite(self):
        _, output = self.generate(output_name="derivative-script")
        fixtures = {
            "linux-mint": (
                'ID=linuxmint\nID_LIKE="ubuntu debian"\nVERSION_CODENAME=zena\n'
                'UBUNTU_CODENAME=noble\n',
                "noble",
            ),
            "anduinos-resolute": (
                'ID=anduinos\nID_LIKE=debian\nVERSION_CODENAME=anduinos\n'
                'UBUNTU_CODENAME=resolute\n',
                "resolute",
            ),
            "mx-linux": (
                'ID=debian\nVERSION_ID="13"\nVERSION_CODENAME=trixie\n',
                "trixie",
            ),
        }
        for name, (release, expected_suite) in fixtures.items():
            with self.subTest(name=name):
                root = self.runtime_os_release(name, release)
                self.run_script(output / "postinst", root, "configure")
                source = root / "etc/apt/sources.list.d/winegui.sources"
                self.assertIn(f"Suites: {expected_suite}", source.read_text())

    def test_derivative_detection_fails_closed(self):
        _, output = self.generate(output_name="unsupported-derivative-script")
        fixtures = {
            "debian-claiming-ubuntu-suite": 'ID=debian\nVERSION_CODENAME=noble\n',
            "unsupported-ubuntu-base": (
                'ID=anduinos\nID_LIKE="ubuntu debian"\nVERSION_CODENAME=questing\n'
                'UBUNTU_CODENAME=questing\n'
            ),
            "id-like-substring": 'ID=other\nID_LIKE=notubuntu\nVERSION_CODENAME=noble\n',
        }
        for name, release in fixtures.items():
            with self.subTest(name=name):
                root = self.runtime_os_release(name, release)
                result = self.run_script(output / "postinst", root, "configure")
                self.assertIn("unsupported", result.stderr)
                self.assertFalse((root / "etc/apt/sources.list.d/winegui.sources").exists())

    def test_derivative_upgrade_recovers_key_only_installation(self):
        _, output = self.generate(output_name="recovery-script")
        root = self.runtime_os_release(
            "recovery",
            'ID=linuxmint\nID_LIKE="ubuntu debian"\nVERSION_CODENAME=zena\n',
        )
        self.run_script(output / "postinst", root, "configure")
        key = root / "usr/share/keyrings/winegui-archive-keyring.gpg"
        generation = root / "var/lib/winegui/key.generation"
        key_hash = hashlib.sha256(key.read_bytes()).hexdigest()
        self.assertTrue(generation.exists())
        self.assertFalse((root / "var/lib/winegui/repository.setup").exists())

        (root / "etc/os-release").write_text(
            'ID=linuxmint\nID_LIKE="ubuntu debian"\nVERSION_CODENAME=zena\n'
            'UBUNTU_CODENAME=noble\n'
        )
        self.run_script(output / "postinst", root, "configure")
        source = root / "etc/apt/sources.list.d/winegui.sources"
        self.assertIn("Suites: noble", source.read_text())
        self.assertEqual(hashlib.sha256(key.read_bytes()).hexdigest(), key_hash)
        self.assertEqual(generation.read_text(), "1\n")

    def test_edited_source_preserves_referenced_key_on_purge(self):
        root, output = self.generate()
        self.run_script(output / "postinst", root, "configure")
        source = root / "etc/apt/sources.list.d/winegui.sources"
        source.write_text(source.read_text() + "Enabled: no\n")
        self.run_script(output / "postrm", root, "purge")
        self.assertTrue(source.exists())
        self.assertTrue((root / "usr/share/keyrings/winegui-archive-keyring.gpg").exists())
        self.assertTrue((root / "var/lib/winegui/key.generation").exists())
        self.assertFalse((root / "var/lib/winegui/repository.setup").exists())

    def test_preexisting_source_is_never_replaced(self):
        root, output = self.generate()
        source = root / "etc/apt/sources.list.d/winegui.sources"
        source.parent.mkdir(parents=True)
        source.write_text("Types: deb\nURIs: https://admin.invalid\n")
        self.run_script(output / "postinst", root, "configure")
        self.assertIn("admin.invalid", source.read_text())
        self.run_script(output / "postinst", root, "configure")
        self.assertIn("admin.invalid", source.read_text())

    def test_older_package_cannot_downgrade_or_purge_newer_trust(self):
        root, newer = self.generate(generation=2, output_name="newer")
        self.run_script(newer / "postinst", root, "configure")
        key = root / "usr/share/keyrings/winegui-archive-keyring.gpg"
        key_hash = hashlib.sha256(key.read_bytes()).hexdigest()
        _, older = self.generate(generation=1, output_name="older")
        self.run_script(older / "postinst", root, "configure")
        self.assertEqual((root / "var/lib/winegui/key.generation").read_text(), "2\n")
        self.assertEqual(hashlib.sha256(key.read_bytes()).hexdigest(), key_hash)
        self.run_script(older / "postrm", root, "purge")
        self.assertTrue(key.exists())
        self.assertTrue((root / "etc/apt/sources.list.d/winegui.sources").exists())

    def test_unsupported_suite_does_not_create_source(self):
        root = self.temporary / "unsupported"
        (root / "etc").mkdir(parents=True)
        (root / "etc/os-release").write_text("ID=ubuntu\nVERSION_ID=99.04\nVERSION_CODENAME=unknown\n")
        _, output = self.generate(output_name="supported-script")
        result = self.run_script(output / "postinst", root, "configure")
        self.assertIn("unsupported", result.stderr)
        self.assertFalse((root / "etc/apt/sources.list.d/winegui.sources").exists())
        self.assertTrue((root / "usr/share/keyrings/winegui-archive-keyring.gpg").exists())


if __name__ == "__main__":
    unittest.main()
