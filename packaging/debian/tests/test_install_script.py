import base64
import hashlib
import re
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
INSTALLER = ROOT / "scripts" / "install.sh"


class InstallScriptTests(unittest.TestCase):
    def detect(self, content):
        with tempfile.NamedTemporaryFile("w", delete=False) as release:
            release.write(content)
            path = release.name
        try:
            return subprocess.run(
                ["bash", str(INSTALLER), "--print-suite", path],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        finally:
            Path(path).unlink()

    def test_detects_supported_derivatives(self):
        fixtures = {
            "noble": 'ID=linuxmint\nID_LIKE="ubuntu debian"\nVERSION_CODENAME=zena\nUBUNTU_CODENAME=noble\n',
            "trixie": 'ID=mx\nID_LIKE="debian"\nVERSION_CODENAME=trixie\n',
        }
        for expected, content in fixtures.items():
            with self.subTest(expected=expected):
                result = self.detect(content)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.strip(), expected)

    def test_unsupported_derivative_fails_closed(self):
        result = self.detect(
            'ID=anduinos\nID_LIKE="ubuntu debian"\nVERSION_CODENAME=questing\nUBUNTU_CODENAME=questing\n'
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("does not report a supported", result.stderr)

    def test_embedded_key_matches_hash_and_primary_fingerprint(self):
        script = INSTALLER.read_text()
        encoded = re.search(r"readonly WINEGUI_KEY_BASE64='([^']+)'", script).group(1)
        expected_hash = re.search(r'readonly WINEGUI_KEY_SHA256="([0-9a-f]+)"', script).group(1)
        expected_fingerprints = re.search(r'readonly WINEGUI_KEY_FINGERPRINTS="([A-F0-9,]+)"', script).group(1)
        key = base64.b64decode(encoded, validate=True)

        self.assertEqual(hashlib.sha256(key).hexdigest(), expected_hash)
        with tempfile.TemporaryDirectory() as home, tempfile.NamedTemporaryFile() as key_file:
            Path(home).chmod(0o700)
            key_file.write(key)
            key_file.flush()
            result = subprocess.run(
                [
                    "gpg", "--homedir", home, "--batch", "--show-keys",
                    "--with-colons", "--with-fingerprint", key_file.name,
                ],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=True,
            )

        primary_fingerprints = []
        want_fingerprint = False
        for line in result.stdout.splitlines():
            fields = line.split(":")
            if fields[0] == "pub":
                want_fingerprint = True
            elif want_fingerprint and fields[0] == "fpr":
                primary_fingerprints.append(fields[9])
                want_fingerprint = False

        self.assertEqual(",".join(primary_fingerprints), expected_fingerprints)


if __name__ == "__main__":
    unittest.main()
