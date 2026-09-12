# WineGUI APT repository runbook

All `ubuntu-server` commands in this document are run by the server owner.
Codex treats that host as read-only.

The production server already has:

- `winegui-apt-artifact-deployer` listening on `127.0.0.1:3063`;
- a deployment webhook for `winegui-apt-repository`;
- the Angie webhook location `/winegui-apt/gitlab`;
- the Angie vhost for `apt.winegui.melroy.org`;
- `/var/spool/winegui-apt` and the public web-root directory.

Do not recreate those components. Complete the native publisher setup below.

## 1. Test and copy the files

Run from the WineGUI repository on the workstation:

```sh
cd /home/melroy/Documents/projects/winegui
./ops/apt-repository/tests/run.sh
./ops/apt-repository/tests/prove-pinned-reprepro.sh

ssh ubuntu-server 'mkdir -p /home/melroy/winegui-apt-setup'
rsync -av ops/apt-repository/ \
  ubuntu-server:/home/melroy/winegui-apt-setup/
```

The real proof must end with:

```text
PASS: reprepro 5.5.1 accepts gzip/zstd DEBs, indexes both versions, and APT exposes both candidates
```

## 2. Install the repository engine

Run on `ubuntu-server`:

```sh
cd /home/melroy/winegui-apt-setup
sudo apt-get update
sudo apt-get install -y \
  acl gnupg libarchive13t64 libdb5.3t64 libgpgme11t64 python3

./reprepro/build-pinned.sh
. ./reprepro/version.env
image="winegui/reprepro:${REPREPRO_VERSION}-${REPREPRO_COMMIT}"

extract_container=$(docker create "$image")
extract_directory=$(mktemp -d)
docker cp "$extract_container:/opt/winegui-reprepro/bin/reprepro" \
  "$extract_directory/reprepro"
docker rm "$extract_container"

sudo install -d -o root -g root -m 0755 \
  /usr/local/lib/winegui-reprepro/bin
sudo install -o root -g root -m 0755 "$extract_directory/reprepro" \
  /usr/local/lib/winegui-reprepro/bin/reprepro
rm -r "$extract_directory"

installed_version=$(
  /usr/local/lib/winegui-reprepro/bin/reprepro --version 2>&1 |
    sed -n 's/^.*: This is reprepro version //p'
)
test "$installed_version" = "$REPREPRO_VERSION"
test -z "$(ldd /usr/local/lib/winegui-reprepro/bin/reprepro | grep 'not found')"
```

Ubuntu 24.04's packaged `reprepro` does not support this repository's
multi-version `Limit: 0` configuration. Always use the pinned binary above.

## 3. Create the publisher account and paths

The Docker daemon uses user-namespace remapping. Measure the deployer's mapped
host identity; do not hard-code it.

```sh
getent passwd winegui-apt >/dev/null || sudo adduser \
  --system --group --home /var/lib/winegui-apt --no-create-home winegui-apt

probe=".ownership-probe-$$"
docker exec winegui-apt-artifact-deployer mkdir "/app/dest/$probe"
WINEGUI_DEPLOYER_UID=$(stat -c %u "/var/spool/winegui-apt/$probe")
WINEGUI_DEPLOYER_GID=$(stat -c %g "/var/spool/winegui-apt/$probe")
docker exec winegui-apt-artifact-deployer rmdir "/app/dest/$probe"
test "$WINEGUI_DEPLOYER_UID" -gt 0
test "$WINEGUI_DEPLOYER_GID" -gt 0
printf 'mapped deployer identity: %s:%s\n' \
  "$WINEGUI_DEPLOYER_UID" "$WINEGUI_DEPLOYER_GID"

sudo install -d -o winegui-apt -g winegui-apt -m 0750 \
  /var/lib/winegui-apt \
  /var/lib/winegui-apt/{batches,journal,reprepro,reprepro/conf,reprepro/db,work}
sudo install -d -o winegui-apt -g winegui-apt -m 0700 \
  /var/lib/winegui-apt/gnupg

sudo install -d -o "$WINEGUI_DEPLOYER_UID" -g "$WINEGUI_DEPLOYER_GID" -m 0750 \
  /var/spool/winegui-apt \
  /var/spool/winegui-apt/{pending,processing,ready,quarantine,state}
sudo install -d -o winegui-apt -g winegui-apt -m 0750 \
  /var/spool/winegui-apt/{publisher-processing,publisher-archive,publisher-quarantine}

sudo install -d -o winegui-apt -g www-data -m 0755 \
  /var/www/apt.winegui.melroy.org \
  /var/www/apt.winegui.melroy.org/html

sudo setfacl -m u:winegui-apt:--x /var/spool/winegui-apt
sudo setfacl -m u:winegui-apt:rwx /var/spool/winegui-apt/ready
sudo setfacl -m d:u:winegui-apt:rwx /var/spool/winegui-apt/pending

sudo stat -c '%A %U:%G %u:%g %n' \
  /var/lib/winegui-apt \
  /var/lib/winegui-apt/gnupg \
  /var/spool/winegui-apt \
  /var/spool/winegui-apt/ready \
  /var/spool/winegui-apt/publisher-processing \
  /var/www/apt.winegui.melroy.org/html
sudo getfacl -p \
  /var/spool/winegui-apt \
  /var/spool/winegui-apt/pending \
  /var/spool/winegui-apt/ready
```

Only the deployer owns its five spool directories. Only `winegui-apt` owns
private state, publisher spool directories, and the public repository. Never
mount private state, the signing home, or the public web root into the deployer.

The numeric deployer identity may be displayed as `UNKNOWN:UNKNOWN`; the UID
and GID must match the measured values. The default ACL on `pending/` is
inherited by new batch directories but does not grant `winegui-apt` access to
`pending/` itself. The deployer finalizes each ready batch root as `0770`, which
makes the inherited ACL writable for the atomic claim. The `sudo` on these
checks is required because the private parent directories deliberately deny
traversal to the normal login user.

Verify that the publisher can claim a completed batch:

```sh
permission_test=".permission-test-$$"
docker exec winegui-apt-artifact-deployer \
  mkdir -m 0755 "/app/dest/pending/$permission_test"
docker exec winegui-apt-artifact-deployer \
  chmod 0770 "/app/dest/pending/$permission_test"
docker exec winegui-apt-artifact-deployer \
  touch "/app/dest/pending/$permission_test/test.txt"
docker exec winegui-apt-artifact-deployer \
  mv "/app/dest/pending/$permission_test" "/app/dest/ready/$permission_test"
sudo -u winegui-apt test -r \
  "/var/spool/winegui-apt/ready/$permission_test/test.txt"
sudo -u winegui-apt mv "/var/spool/winegui-apt/ready/$permission_test" \
  "/var/spool/winegui-apt/publisher-processing/$permission_test"
sudo -u winegui-apt rm -r \
  "/var/spool/winegui-apt/publisher-processing/$permission_test"
```

## 4. Install the publisher

Run from `/home/melroy/winegui-apt-setup`:

```sh
sudo install -d -o root -g root -m 0755 \
  /usr/local/lib/winegui-apt/{bin,publisher}
sudo install -o root -g root -m 0755 \
  bin/winegui-apt-publisher bin/winegui-apt-healthcheck bin/winegui-apt-backup \
  /usr/local/lib/winegui-apt/bin/
sudo install -o root -g root -m 0755 publisher/winegui_apt_publisher.py \
  /usr/local/lib/winegui-apt/publisher/
sudo install -o winegui-apt -g winegui-apt -m 0644 config/reprepro/conf/* \
  /var/lib/winegui-apt/reprepro/conf/
if [ ! -e /etc/winegui-apt-publisher.env ]; then
  sudo install -o root -g winegui-apt -m 0640 config/publisher.env.example \
    /etc/winegui-apt-publisher.env
fi

sudo install -o root -g root -m 0644 \
  systemd/winegui-apt-publisher.service \
  systemd/winegui-apt-publisher.path \
  systemd/winegui-apt-publisher.timer \
  /etc/systemd/system/
sudo install -o root -g root -m 0644 \
  systemd/winegui-apt-publisher.tmpfiles.conf \
  /etc/tmpfiles.d/winegui-apt-publisher.conf
sudo systemd-tmpfiles --create \
  /etc/tmpfiles.d/winegui-apt-publisher.conf
sudo systemctl daemon-reload
```

Do not enable the service until signing works unattended.

## 5. Install signing material

Create the certifying primary key offline and add a dedicated signing subkey.
Keep the primary secret key offline. Export:

- only the secret signing subkey for the server;
- the complete public keyring for WineGUI packages;
- the full primary and signing-subkey fingerprints.

After securely transferring the secret subkey, copy it into the protected GPG
home and import it:

```sh
sudo install -o winegui-apt -g winegui-apt -m 0600 \
  /path/to/secret-signing-subkey.gpg \
  /var/lib/winegui-apt/gnupg/import.gpg
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --batch --import /var/lib/winegui-apt/gnupg/import.gpg
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --with-colons --list-secret-keys --fingerprint --fingerprint
sudo rm /var/lib/winegui-apt/gnupg/import.gpg
sudoedit /etc/winegui-apt-publisher.env
```

Set `WINEGUI_APT_SIGNING_KEY` to the full signing-subkey fingerprint. Configure
these protected GitLab CI/CD variables:

- `WINEGUI_APT_PUBLIC_KEY_FILE`: file variable containing the public export;
- `WINEGUI_APT_KEY_FINGERPRINTS`: full primary fingerprint;
- `WINEGUI_APT_KEY_GENERATION`: `1` for the first key generation.

Test both signature formats as the service account:

```sh
signing_key=$(sudo sed -n 's/^WINEGUI_APT_SIGNING_KEY=//p' \
  /etc/winegui-apt-publisher.env)
sign_test=$(mktemp -d)
printf 'WineGUI signing test\n' > "$sign_test/Release"
sudo chown -R winegui-apt:winegui-apt "$sign_test"

sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --batch --yes --local-user "$signing_key" \
  --output "$sign_test/Release.gpg" --detach-sign "$sign_test/Release"
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --batch --yes --local-user "$signing_key" \
  --output "$sign_test/InRelease" --clearsign "$sign_test/Release"

sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --verify "$sign_test/Release.gpg" "$sign_test/Release"
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --verify "$sign_test/InRelease"
sudo rm -r "$sign_test"
```

The same commands must work unattended after a server reboot. Do not enable
automatic publication while GPG still requires interactive input.

## 6. Enable and verify publication

```sh
sudo systemd-analyze security winegui-apt-publisher.service
sudo systemctl enable --now \
  winegui-apt-publisher.path winegui-apt-publisher.timer
sudo systemctl start winegui-apt-publisher.service
sudo systemctl status \
  winegui-apt-publisher.service \
  winegui-apt-publisher.path \
  winegui-apt-publisher.timer
sudo journalctl -u winegui-apt-publisher.service -n 100 --no-pager
sudo angie -t
```

The first empty run may create publisher health state but publishes no suite.
Create a tagged WineGUI release, then verify:

```sh
curl --fail --silent --show-error --head \
  https://apt.winegui.melroy.org/dists/noble/InRelease
curl --fail --silent --show-error \
  https://apt.winegui.melroy.org/dists/noble/InRelease | \
  sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg gpg --verify

find /var/spool/winegui-apt/publisher-quarantine \
  -mindepth 1 -maxdepth 1 -print
sudo journalctl -u winegui-apt-publisher.service -n 100 --no-pager
```

Test a clean APT client for every supported suite. Confirm that it can install
the latest WineGUI version, select an older retained version, and upgrade again.

## 7. Routine operation

Normal tagged releases require no server command. The path unit starts the
publisher when a completed batch arrives; the timer retries interrupted work.

After a supported client OS upgrade, reconfigure or reinstall the current
WineGUI DEB. Its maintainer script updates an unchanged managed source to the
new OS suite. It preserves an administrator-edited source, which must then be
updated manually before `apt update`.

Never edit a quarantined batch. Diagnose the reason, fix the pipeline, and
publish a new batch. Run all repository maintenance under the publisher lock:

```sh
sudo -u winegui-apt flock /run/lock/winegui-apt-publisher.lock \
  REPLACE_MAINTENANCE_COMMAND
```

Create backups with the supplied wrapper:

```sh
sudo install -d -o root -g root -m 0700 /var/backups/winegui-apt
sudo /usr/local/lib/winegui-apt/bin/winegui-apt-backup \
  /var/backups/winegui-apt
```

Backups contain online signing material. Encrypt them, store them off-host, and
test restoration into disposable paths. Restore private state, spool, and the
public repository from one consistent backup; never mix generations.

Adapt `monitoring/monit.conf` to the server's existing Monit includes. Monitor
publisher health, quarantine contents, disk and inode use, external HTTPS, the
signing-key expiry, and the TLS certificate expiry.

To roll back a package, stop automatic intake, preserve the journal and batch,
remove only the exact package version with the pinned engine under the lock,
republish all metadata, verify APT clients, and then resume. Do not delete files
directly from `pool/`, `dists/`, or `by-hash/`.

For key rotation, ship old and new public keys together for at least two
releases and 180 days, increment `WINEGUI_APT_KEY_GENERATION`, and keep a
current independently verifiable DEB available for clients that miss the
overlap period.
