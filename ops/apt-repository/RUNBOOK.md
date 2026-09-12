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

### 5.1 Create the master key on the workstation

Run this subsection as the normal workstation user. Do not use `sudo`. The
working keyring is created below the user's home directory so every command is
directly executable. Disconnect the workstation from the network while creating
the key if practical.

```sh
umask 077
WINEGUI_KEY_HOME="$HOME/.local/share/winegui-apt-master-key"
install -d -m 0700 "$WINEGUI_KEY_HOME"

gpg --homedir "$WINEGUI_KEY_HOME" \
  --quick-generate-key \
  "WineGUI APT Repository <melroy@melroy.org>" \
  ed25519 cert 5y

PRIMARY_FPR=$(
  gpg --homedir "$WINEGUI_KEY_HOME" \
    --with-colons --list-secret-keys |
    awk -F: '$1 == "fpr" { print $10; exit }'
)
test -n "$PRIMARY_FPR"
printf 'Primary fingerprint: %s\n' "$PRIMARY_FPR"

gpg --homedir "$WINEGUI_KEY_HOME" \
  --quick-add-key "$PRIMARY_FPR" ed25519 sign 2y

gpg --homedir "$WINEGUI_KEY_HOME" \
  --with-subkey-fingerprints --list-secret-keys "$PRIMARY_FPR"

SIGNING_FPR=$(
  gpg --homedir "$WINEGUI_KEY_HOME" \
    --with-colons --list-secret-keys "$PRIMARY_FPR" |
    awk -F: '$1 == "ssb" { subkey = 1; next }
      subkey && $1 == "fpr" { print $10; exit }'
)
test -n "$SIGNING_FPR"
printf 'Signing fingerprint: %s\n' "$SIGNING_FPR"

WINEGUI_KEY_EXPORT="$HOME/.local/share/winegui-apt-key-export"
install -d -m 0700 "$WINEGUI_KEY_EXPORT"
gpg --homedir "$WINEGUI_KEY_HOME" --armor --export-options export-minimal \
  --output "$WINEGUI_KEY_EXPORT/winegui-apt-public.asc" \
  --export "$PRIMARY_FPR"
gpg --homedir "$WINEGUI_KEY_HOME" \
  --output "$WINEGUI_KEY_EXPORT/winegui-apt-secret-subkeys.gpg" \
  --export-secret-subkeys "$PRIMARY_FPR"
chmod 0600 "$WINEGUI_KEY_EXPORT"/*
```

The first GPG command asks for a new passphrase; it is not requesting an
existing system password. Store the new passphrase in a password manager. The
listing must show a certification-only primary key (`[C]`) and a signing subkey
(`[S]`).

The two export files are:

- `winegui-apt-secret-subkeys.gpg`: server import containing a disabled primary
  stub and the secret signing subkey;
- `winegui-apt-public.asc`: public key supplied to package builds.

Copy the complete `$WINEGUI_KEY_HOME` directory to encrypted offline storage
before deleting the workstation copy. Preserve the printed primary and signing
fingerprints with that backup. The primary secret key must never be copied to
`ubuntu-server` or GitLab.

### 5.2 Install the signing subkey on the server

First transfer only the signing-subkey export from the workstation as the normal
workstation user; do not use `sudo`:

```sh
scp "$WINEGUI_KEY_EXPORT/winegui-apt-secret-subkeys.gpg" \
  ubuntu-server:/home/melroy/
```

Then run the following commands on `ubuntu-server`. These commands require
`sudo` because they install into protected system paths:

```sh
sudo install -o winegui-apt -g winegui-apt -m 0600 \
  /home/melroy/winegui-apt-secret-subkeys.gpg \
  /var/lib/winegui-apt/gnupg/import.gpg
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --batch --import /var/lib/winegui-apt/gnupg/import.gpg
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --with-colons --list-secret-keys --fingerprint --fingerprint
sudo rm /var/lib/winegui-apt/gnupg/import.gpg
rm /home/melroy/winegui-apt-secret-subkeys.gpg
```

The signing subkey export retains the master-key passphrase. The publisher must
sign unattended after reboot, so remove the passphrase from only the imported
server-side signing subkey. The server account and mode `0700` GPG home become
the protection for this operational copy; the offline primary remains
passphrase-protected.

```sh
SIGNING_FPR=$(
  sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
    gpg --with-colons --list-secret-keys |
    awk -F: '$1 == "ssb" { subkey = 1; next }
      subkey && $1 == "fpr" { print $10; exit }'
)
SIGNING_KEYGRIP=$(
  sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
    gpg --with-colons --with-keygrip --list-secret-keys "$SIGNING_FPR" |
    awk -F: '$1 == "ssb" { subkey = 1; next }
      subkey && $1 == "grp" { print $10; exit }'
)
test -n "$SIGNING_FPR"
test -n "$SIGNING_KEYGRIP"
printf 'Signing fingerprint: %s\nSigning keygrip: %s\n' \
  "$SIGNING_FPR" "$SIGNING_KEYGRIP"

SERVER_TTY=$(tty)
sudo setfacl -m u:winegui-apt:rw- "$SERVER_TTY"
remove_tty_acl() {
  sudo setfacl -x u:winegui-apt "$SERVER_TTY"
}
trap remove_tty_acl EXIT HUP INT TERM

sudo -u winegui-apt env \
  GNUPGHOME=/var/lib/winegui-apt/gnupg \
  GPG_TTY="$SERVER_TTY" \
  gpg-connect-agent updatestartuptty /bye
sudo -u winegui-apt env \
  GNUPGHOME=/var/lib/winegui-apt/gnupg \
  GPG_TTY="$SERVER_TTY" \
  gpg-connect-agent "PASSWD $SIGNING_KEYGRIP" /bye

remove_tty_acl
trap - EXIT HUP INT TERM
```

Enter the existing master-key passphrase when prompted. For the new passphrase,
leave both entries empty and confirm the warning. This changes only the signing
subkey identified by its keygrip. The temporary terminal ACL is necessary
because Pinentry runs as `winegui-apt` while the SSH terminal belongs to the
login user; the trap removes it on success, failure, or interruption.

### 5.3 Configure the publisher and GitLab

Run on `ubuntu-server`:

```sh
sudoedit /etc/winegui-apt-publisher.env
```

Set `WINEGUI_APT_SIGNING_KEY` to the full signing-subkey fingerprint. Configure
these protected GitLab CI/CD variables:

- `WINEGUI_APT_PUBLIC_KEY_FILE`: file variable containing the public export;
- `WINEGUI_APT_KEY_FINGERPRINTS`: full primary fingerprint;
- `WINEGUI_APT_KEY_GENERATION`: `1` for the first key generation.

### 5.4 Verify unattended signing

Run on `ubuntu-server`. Killing the service account's GPG agent first proves
that the test does not depend on a cached passphrase:

```sh
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpgconf --kill gpg-agent
```

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
