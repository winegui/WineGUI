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

The initial two export files are:

- `winegui-apt-secret-subkeys.gpg`: server import containing a disabled primary
  stub and the secret signing subkey;
- `winegui-apt-public.asc`: public key supplied to package builds.

The initial secret-subkey export retains the primary key's passphrase. Prepare
a separate operational copy on the workstation, remove the passphrase from only
that copy's signing subkey, and prove it signs non-interactively. Do not alter
the master keyring.

```sh
WINEGUI_SERVER_KEY_HOME="$HOME/.local/share/winegui-apt-server-key"
install -d -m 0700 "$WINEGUI_SERVER_KEY_HOME"
gpg --homedir "$WINEGUI_SERVER_KEY_HOME" --batch \
  --import "$WINEGUI_KEY_EXPORT/winegui-apt-secret-subkeys.gpg"

SERVER_SIGNING_KEYGRIP=$(
  gpg --homedir "$WINEGUI_SERVER_KEY_HOME" \
    --with-colons --with-keygrip --list-secret-keys "$SIGNING_FPR" |
    awk -F: '$1 == "ssb" { subkey = 1; next }
      subkey && $1 == "grp" { print $10; exit }'
)
test -n "$SERVER_SIGNING_KEYGRIP"
printf 'Server signing keygrip: %s\n' "$SERVER_SIGNING_KEYGRIP"

export GNUPGHOME="$WINEGUI_SERVER_KEY_HOME"
export GPG_TTY="$(tty)"
gpg-connect-agent updatestartuptty /bye
gpg-connect-agent "PASSWD $SERVER_SIGNING_KEYGRIP" /bye
unset GNUPGHOME GPG_TTY
```

Enter the existing master-key passphrase. Leave both new-passphrase fields
empty and confirm the warning. This modifies only the isolated operational
copy. Verify that it signs without a passphrase or cached agent state:

```sh
gpgconf --homedir "$WINEGUI_SERVER_KEY_HOME" --kill gpg-agent
SIGN_TEST=$(mktemp -d)
printf 'WineGUI signing test\n' > "$SIGN_TEST/Release"
gpg --homedir "$WINEGUI_SERVER_KEY_HOME" --batch --yes \
  --local-user "$SIGNING_FPR!" \
  --output "$SIGN_TEST/Release.gpg" --detach-sign "$SIGN_TEST/Release"
gpg --homedir "$WINEGUI_SERVER_KEY_HOME" \
  --verify "$SIGN_TEST/Release.gpg" "$SIGN_TEST/Release"
rm -r "$SIGN_TEST"

gpg --homedir "$WINEGUI_SERVER_KEY_HOME" --batch --yes \
  --output "$WINEGUI_KEY_EXPORT/winegui-apt-server-signing.gpg" \
  --export-secret-subkeys "$PRIMARY_FPR"
chmod 0600 "$WINEGUI_KEY_EXPORT/winegui-apt-server-signing.gpg"
```

Copy the complete `$WINEGUI_KEY_HOME` directory to encrypted offline storage
before deleting the workstation copy. Preserve the printed primary and signing
fingerprints with that backup. The primary secret key must never be copied to
`ubuntu-server` or GitLab.

### 5.2 Install the signing subkey on the server

First transfer only the passphrase-free operational export from the workstation
as the normal workstation user; do not use `sudo`:

```sh
scp "$WINEGUI_KEY_EXPORT/winegui-apt-server-signing.gpg" \
  ubuntu-server:/home/melroy/
```

Then run the following commands on `ubuntu-server`. These commands require
`sudo` because they install into protected system paths:

```sh
sudo install -o winegui-apt -g winegui-apt -m 0600 \
  /home/melroy/winegui-apt-server-signing.gpg \
  /var/lib/winegui-apt/gnupg/import.gpg
SIGNING_FPR="420A8A144B755A988BCDBF616B093B573BDACEF1"
if sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --with-colons --list-secret-keys "$SIGNING_FPR" | grep -q '^ssb:'; then
  sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
    gpg --batch --yes --delete-secret-keys "$SIGNING_FPR!"
fi
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --batch --import /var/lib/winegui-apt/gnupg/import.gpg
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --with-colons --list-secret-keys --fingerprint --fingerprint
sudo rm /var/lib/winegui-apt/gnupg/import.gpg
rm /home/melroy/winegui-apt-server-signing.gpg
```

The `FINGERPRINT!` selector removes only a previously imported copy of that
secret subkey before replacement; it does not delete the public certificate or
touch the offline master key. The server account and mode `0700` GPG home
protect the passphrase-free operational copy.

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

After the server passes this test, remove only the passphrase-free operational
staging copies from the workstation. Keep the protected master keyring and its
encrypted backups. Keep the public export for GitLab and package verification:

```sh
rm -r "$HOME/.local/share/winegui-apt-server-key"
rm "$HOME/.local/share/winegui-apt-key-export/winegui-apt-server-signing.gpg"
```

The protected `winegui-apt-secret-subkeys.gpg` export is redundant once the
master keyring is backed up and may also be removed. It can always be recreated
from the protected master keyring.

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
publisher when a completed batch arrives; the hourly timer is a fallback for
missed path events, restart recovery, and interrupted work.

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

## 8. Signing-key expiry and rotation

Do not wait for a key to expire. The current key schedule is:

| Key | Fingerprint | Expires | Start rotation no later than |
| --- | --- | --- | --- |
| Signing subkey | `420A8A144B755A988BCDBF616B093B573BDACEF1` | 2028-09-11 | 2028-03-15 |
| Certification primary | `DC1522C22AC2908D6AF1D0A54AD49EDC219C57F7` | 2031-09-11 | 2030-09-11 |

Monitor both dates. A normal signing-subkey renewal keeps the same primary
fingerprint. A primary-key rotation introduces a new trust identity and needs a
longer overlap. Recalculate the rotation date for every replacement signing
subkey: begin at least 180 days before its expiration.

### 8.1 Rules shared by every rotation

1. Take and verify a consistent repository backup under the publisher lock.
2. Restore the protected master keyring from encrypted offline storage onto a
   disconnected workstation. Never copy the primary secret key to the server or
   GitLab.
3. Keep the repository signed by the old operational subkey while distributing
   packages containing the expanded public keyring.
4. Increment `WINEGUI_APT_KEY_GENERATION` whenever the public keyring embedded
   in packages changes. Never reuse or decrease a generation.
5. Publish at least two normal releases and maintain at least 180 days of
   overlap before switching repository signatures.
6. Test the new operational subkey without cached agent state before installing
   it on the server.
7. Keep the old server signing subkey until a new-signed repository has been
   published and verified from clean clients. It is the rollback key during the
   switch.
8. Keep a current DEB and its checksum available independently from APT for
   clients that miss the complete overlap.

The GitLab variables involved are:

- `WINEGUI_APT_PUBLIC_KEY_FILE`: complete public export for the current
  transition;
- `WINEGUI_APT_KEY_FINGERPRINTS`: comma-separated primary fingerprints present
  in that export;
- `WINEGUI_APT_KEY_GENERATION`: monotonically increasing keyring generation.

The server setting involved is `WINEGUI_APT_SIGNING_KEY` in
`/etc/winegui-apt-publisher.env`. It always contains one full operational
signing-subkey fingerprint.

### 8.2 Renew the signing subkey under the current primary

Use this procedure before 2028-03-15. Run as the normal workstation user with
the restored master keyring; do not use `sudo`:

```sh
WINEGUI_KEY_HOME="$HOME/.local/share/winegui-apt-master-key"
WINEGUI_KEY_EXPORT="$HOME/.local/share/winegui-apt-key-export"
PRIMARY_FPR="DC1522C22AC2908D6AF1D0A54AD49EDC219C57F7"
OLD_SIGNING_FPR="420A8A144B755A988BCDBF616B093B573BDACEF1"

gpg --homedir "$WINEGUI_KEY_HOME" \
  --quick-add-key "$PRIMARY_FPR" ed25519 sign 2y
gpg --homedir "$WINEGUI_KEY_HOME" \
  --with-subkey-fingerprints --list-secret-keys "$PRIMARY_FPR"

NEW_SIGNING_FPR=$(
  gpg --homedir "$WINEGUI_KEY_HOME" \
    --with-colons --list-secret-keys "$PRIMARY_FPR" |
    awk -F: '$1 == "ssb" { subkey = 1; next }
      subkey && $1 == "fpr" { newest = $10; subkey = 0 }
      END { print newest }'
)
test -n "$NEW_SIGNING_FPR"
test "$NEW_SIGNING_FPR" != "$OLD_SIGNING_FPR"
printf 'New signing fingerprint: %s\n' "$NEW_SIGNING_FPR"
```

Record the new signing-subkey fingerprint. Export the updated public
certificate containing both old and new subkeys:

```sh
gpg --homedir "$WINEGUI_KEY_HOME" --batch --yes --armor \
  --export-options export-minimal \
  --output "$WINEGUI_KEY_EXPORT/winegui-apt-public.asc" \
  --export "$PRIMARY_FPR"
```

Update the GitLab public-key file variable, leave
`WINEGUI_APT_KEY_FINGERPRINTS` set to the same primary fingerprint, and
increment `WINEGUI_APT_KEY_GENERATION`. Publish the bridge releases while the
server still signs with the old subkey.

Prepare a passphrase-free operational copy of only the new signing subkey using
the isolated workstation procedure from section 5.1. Select only that subkey
when making the protected intermediate export:

```sh
gpg --homedir "$WINEGUI_KEY_HOME" \
  --output "$WINEGUI_KEY_EXPORT/new-protected-signing-subkey.gpg" \
  --export-secret-subkeys "${NEW_SIGNING_FPR}!"
```

After the bridge period, import the tested operational export on the server,
change `WINEGUI_APT_SIGNING_KEY` to `NEW_SIGNING_FPR`, kill the service account's
GPG agent, and repeat section 5.4. Start the publisher once manually and verify
all suites from a clean APT client. Only then remove the old server secret
subkey with:

```sh
sudo -u winegui-apt env GNUPGHOME=/var/lib/winegui-apt/gnupg \
  gpg --batch --yes --delete-secret-keys "${OLD_SIGNING_FPR}!"
```

Back up the updated offline master keyring and its revocation material before
removing the working copy from the workstation.

### 8.3 Replace the five-year primary key

Start this procedure by 2030-09-11. Do not merely switch to a new primary key:
clients trust only keys delivered by packages they already trust.

1. Create a separate protected certification-only primary and signing subkey by
   repeating section 5.1 with a new master-key directory. Record both new
   fingerprints and make two encrypted offline backups.
2. Create one public-key file containing complete public exports of both the old
   and new primaries. Set `WINEGUI_APT_KEY_FINGERPRINTS` to
   `OLD_PRIMARY_FPR,NEW_PRIMARY_FPR` and increment
   `WINEGUI_APT_KEY_GENERATION`.
3. Keep signing the repository with the old signing subkey for at least two
   releases and 180 days. Those bridge packages install both public keys on
   clients.
4. Prepare and test a passphrase-free operational export of only the new
   primary's signing subkey. Import it on the server without deleting the old
   signing subkey.
5. Change `WINEGUI_APT_SIGNING_KEY` to the new signing-subkey fingerprint. Kill
   the service account's GPG agent, repeat section 5.4, publish once manually,
   and verify every suite from clean clients.
6. Retain the old public key in package builds through the full overlap. After
   retirement, publish a new-primary-only public export and increment the key
   generation again. Remove the old server secret subkey only after the
   new-signed repository and client recovery path are verified.

Create the two-primary public bundle on the workstation without secret packets:

```sh
gpg --homedir "$OLD_KEY_HOME" --armor --export-options export-minimal \
  --export "$OLD_PRIMARY_FPR" > "$WINEGUI_KEY_EXPORT/old-public.asc"
gpg --homedir "$NEW_KEY_HOME" --armor --export-options export-minimal \
  --export "$NEW_PRIMARY_FPR" > "$WINEGUI_KEY_EXPORT/new-public.asc"
cat "$WINEGUI_KEY_EXPORT/old-public.asc" \
  "$WINEGUI_KEY_EXPORT/new-public.asc" \
  > "$WINEGUI_KEY_EXPORT/winegui-apt-public-transition.asc"
```

Before using the bundle, run the packaging tests with that file and both
primary fingerprints. The build rejects secret packets and mismatched
fingerprints.

### 8.4 Required client tests and missed-overlap recovery

Test each rotation with disposable clients before retiring the old key:

- a client with the previous WineGUI package receives the bridge package, then
  completes `apt update` after the signing switch;
- a fresh client installs the current package and completes `apt update`;
- installing a retained old package after a current package does not downgrade
  the keyring generation;
- a client that missed the entire overlap initially fails verification, then
  recovers by installing the current DEB obtained outside APT and verified
  against its published checksum.

An old retained DEB is not an offline recovery mechanism: a fresh installation
of it may contain only the retired key. Document recovery as downloading the
current release DEB from the official HTTPS download or GitLab Release location,
verifying its checksum independently, installing it with `apt install ./...deb`,
and then running `apt update` again.

If the old key expires before bridge packages have reached clients, do not
silently disable signature checking or extend trust on clients. Restore the old
primary from offline storage only to publish a properly documented emergency
transition while it is still cryptographically usable; otherwise use the
independently verified current-DEB recovery path.
