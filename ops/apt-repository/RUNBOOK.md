# User-run WineGUI APT repository runbook

Every command in this document is for the server owner to review and run. Codex must never execute these commands on `ubuntu-server`. Replace every `REPLACE_*` value first, retain an authenticated shell for rollback, and take a current backup before changing an existing service.

Account creation, the pinned engine, directory layout, ACLs, signing material,
and systemd installation are one-time host setup. Normal tagged releases are
automatic afterward. Repeat these steps only for a restore or host migration;
key rotation and publisher/engine upgrades use their own reviewed procedures.

## 1. Pre-production gates

On a disposable local machine, run the contract suite and the real pinned-engine proof:

```sh
./ops/apt-repository/tests/run.sh
./ops/apt-repository/tests/prove-pinned-reprepro.sh
```

Do not install the engine unless the second command proves that both versions are present in `Packages` and visible through `apt-cache policy`. The Dockerfile checks out the official upstream tag and exact commit recorded in `ops/apt-repository/reprepro/version.env`; changing any pin requires review and rerunning the proof.

Ubuntu 24.04's packaged `reprepro` is too old for this repository's `Limit: 0` configuration. The pinned upstream engine supports multiple indexed versions and uses libarchive for current Debian compression formats. The proof imports one gzip-member and one zstd-member DEB.

Build a host binary from the already-proven image without trusting the host's ordinary Ubuntu `reprepro` package:

```sh
sudo apt-get update
sudo apt-get install acl gnupg libarchive13t64 libdb5.3t64 libgpgme11t64 python3

. ops/apt-repository/reprepro/version.env
image="winegui/reprepro:${REPREPRO_VERSION}-${REPREPRO_COMMIT}"
docker create --name winegui-reprepro-extract "$image"
sudo install -d -o root -g root -m 0755 /usr/local/lib/winegui-reprepro/bin
docker cp winegui-reprepro-extract:/opt/winegui-reprepro/bin/reprepro /tmp/winegui-reprepro
docker rm winegui-reprepro-extract
sudo install -o root -g root -m 0755 /tmp/winegui-reprepro /usr/local/lib/winegui-reprepro/bin/reprepro
rm /tmp/winegui-reprepro
/usr/local/lib/winegui-reprepro/bin/reprepro --version
test "$(/usr/local/lib/winegui-reprepro/bin/reprepro --version 2>&1 | sed -n 's/^.*: This is reprepro version //p')" = "$REPREPRO_VERSION"
```

Verify runtime library linkage before continuing:

```sh
ldd /usr/local/lib/winegui-reprepro/bin/reprepro
test -z "$(ldd /usr/local/lib/winegui-reprepro/bin/reprepro | grep 'not found')"
```

If this check reports a missing library, stop and install the named Noble runtime package after review; never silently fall back to Ubuntu's single-version build.

## 2. Account, paths, and permissions

Create a locked service account. Docker on the production server uses
`userns-remap`; container UID/GID values therefore do not equal host values.
Measure the owner created through the actual APT deployer mount before changing
permissions. This probe is preferable to calculating an ID from `/etc/subuid`
because it also verifies the active container and mount.

```sh
sudo adduser --system --group --home /var/lib/winegui-apt --no-create-home winegui-apt

docker exec winegui-apt-artifact-deployer mkdir /app/dest/.ownership-probe
WINEGUI_DEPLOYER_UID=$(stat -c %u /var/spool/winegui-apt/.ownership-probe)
WINEGUI_DEPLOYER_GID=$(stat -c %g /var/spool/winegui-apt/.ownership-probe)
docker exec winegui-apt-artifact-deployer rmdir /app/dest/.ownership-probe
test "$WINEGUI_DEPLOYER_UID" -gt 0
test "$WINEGUI_DEPLOYER_GID" -gt 0
printf 'mapped deployer identity: %s:%s\n' "$WINEGUI_DEPLOYER_UID" "$WINEGUI_DEPLOYER_GID"

sudo install -d -o winegui-apt -g winegui-apt -m 0750 \
  /var/lib/winegui-apt /var/lib/winegui-apt/{batches,journal,reprepro,reprepro/conf,reprepro/db,work,gnupg}
sudo install -d -o "$WINEGUI_DEPLOYER_UID" -g "$WINEGUI_DEPLOYER_GID" -m 0750 \
  /var/spool/winegui-apt /var/spool/winegui-apt/{pending,processing,ready,quarantine,state}
sudo install -d -o winegui-apt -g winegui-apt -m 0750 \
  /var/spool/winegui-apt/{publisher-processing,publisher-archive,publisher-quarantine}
sudo install -d -o winegui-apt -g www-data -m 0755 \
  /var/www/apt.winegui.melroy.org /var/www/apt.winegui.melroy.org/html
```

The deployer retains ownership of its five spool directories. Give the native
publisher only traversal on the spool root and permission to claim entries from
`ready/`; it does not need access to `pending/`, `processing/`, `quarantine/`,
or `state/`.

```sh
sudo setfacl -m u:winegui-apt:--x /var/spool/winegui-apt
sudo setfacl -m u:winegui-apt:rwx /var/spool/winegui-apt/ready

docker exec winegui-apt-artifact-deployer \
  mkdir -m 0755 /app/dest/ready/.permission-test
docker exec winegui-apt-artifact-deployer \
  sh -c 'printf test > /app/dest/ready/.permission-test/test.txt'
sudo -u winegui-apt test -r /var/spool/winegui-apt/ready/.permission-test/test.txt
sudo -u winegui-apt mv /var/spool/winegui-apt/ready/.permission-test \
  /var/spool/winegui-apt/publisher-processing/.permission-test
sudo -u winegui-apt rm -r \
  /var/spool/winegui-apt/publisher-processing/.permission-test
```

The public APT root is deliberately owned by the native `winegui-apt` account,
not the remapped Docker identity: only the publisher writes repository output.
Do not mount `/var/lib/winegui-apt`, the public web root, or GnuPG home into the
deployer container.

The existing direct WineGUI download deployer is a separate case. Historical
download directories were created by container UID 1000, while the current
image runs as container UID 0. If `cap_drop: [ALL]` is enabled, normalize only
its writable bind targets to the measured mapped UID/GID once, after a backup:

```sh
sudo chown -R "$WINEGUI_DEPLOYER_UID:$WINEGUI_DEPLOYER_GID" \
  /var/www/winegui.melroy.org/html/downloads
sudo chown "$WINEGUI_DEPLOYER_UID:$WINEGUI_DEPLOYER_GID" \
  /var/www/winegui.melroy.org/html/latest_release.txt
docker exec winegui-artifact-deployer \
  test -w /app/dest/build_prod
docker exec winegui-artifact-deployer \
  test -w /app/dest/release/latest_release.txt
```

Other container-written `/var/www` trees may intentionally use a different
mapped container UID. Do not apply this ownership recursively to `/var/www` or
copy these numeric IDs to another service without measuring that container.

Install this tree's publisher files and unsigned `reprepro` configuration:

```sh
sudo install -d -o root -g root -m 0755 /usr/local/lib/winegui-apt/{bin,publisher}
sudo install -o root -g root -m 0755 ops/apt-repository/bin/winegui-apt-publisher \
  ops/apt-repository/bin/winegui-apt-healthcheck ops/apt-repository/bin/winegui-apt-backup \
  /usr/local/lib/winegui-apt/bin/
sudo install -o root -g root -m 0755 ops/apt-repository/publisher/winegui_apt_publisher.py \
  /usr/local/lib/winegui-apt/publisher/
sudo install -o winegui-apt -g winegui-apt -m 0644 ops/apt-repository/config/reprepro/conf/* \
  /var/lib/winegui-apt/reprepro/conf/
sudo install -o root -g winegui-apt -m 0640 ops/apt-repository/config/publisher.env.example \
  /etc/winegui-apt-publisher.env
```

The `conf/distributions` file intentionally has no `SignWith`: the publisher must add `Acquire-By-Hash: yes` and immutable hash objects before it signs final Release files itself.

## 3. Signing material

Create the certifying primary key and its revocation certificate on an offline system. Add a dedicated signing-capable subkey, export only that secret subkey for the server, and separately export the complete public keyring for WineGUI packages. Record the full signing-subkey fingerprint and key-generation integer in the offline key register.

Transfer the encrypted subkey export through an authenticated out-of-band path. As the server owner, import it into `/var/lib/winegui-apt/gnupg`, remove the transfer file, set the directory to `0700`, and confirm that `gpg --with-colons --list-secret-keys` shows only the intended online signing material. Put the full fingerprint in `/etc/winegui-apt-publisher.env`. Never put private keys in this repository, GitLab variables/artifacts, Docker images, or container mounts.

Unattended signing requires a server-owner-approved `gpg-agent` passphrase strategy. Verify it after a reboot with a disposable Release file before enabling automatic publication. The publisher invokes `gpg --batch`; it never creates or rotates keys.

Rotation procedure:

1. Ship a package keyring containing old and new public keys while repository signatures remain acceptable to old clients.
2. Keep overlap for at least two WineGUI releases and 180 days.
3. Increment the package key-generation marker; retained old DEBs must not downgrade it.
4. Publish current-DEB and `SHA256SUMS` recovery artifacts through both GitLab and GitHub.
5. Test a client that missed the entire overlap by independently verifying and installing the current DEB before retiring the old key.

## 4. Deployer instance and webhook

Use the published `latest` image containing the reviewed durable batch mode, matching the existing production deployer convention. Copy `compose/deployer.compose.yaml` and create `/home/melroy/docker/.env.deployer.winegui-apt` from the example with mode `0600`. Leave `USE_JOB_NAME`, `PROJECT_ID`, and post-deployment commands unset: batch mode must fetch the exact webhook `deployable_id`.

The new instance filters only `winegui-apt-repository`; configure the existing WineGUI download deployer to filter only `production`. Point the new GitLab deployment-event webhook at the path in `angie/deployer-location.conf` with its own secret. The APT hostname remains static-only.

Before starting, validate and inspect the effective configuration:

```sh
docker compose -f REPLACE_COMPOSE_PATH config
docker compose -f REPLACE_COMPOSE_PATH pull
docker image inspect REPLACE_IMAGE_DIGEST
docker compose -f REPLACE_COMPOSE_PATH up -d
docker compose -f REPLACE_COMPOSE_PATH ps
docker logs --tail 100 winegui-apt-artifact-deployer
```

Send a signed test webhook for the wrong environment and verify that it is ignored, then a disposable matching event and verify the exact `p<project>-d<deployment>-j<job>` directory reaches `ready/` without shared extraction or a job-name lookup.

## 5. Publisher service

Install the systemd files, review the hardening output, and enable both event-driven and periodic reconciliation:

```sh
sudo install -o root -g root -m 0644 \
  ops/apt-repository/systemd/*.service ops/apt-repository/systemd/*.path \
  ops/apt-repository/systemd/*.timer /etc/systemd/system/
sudo install -o root -g root -m 0644 ops/apt-repository/systemd/winegui-apt-publisher.tmpfiles.conf \
  /etc/tmpfiles.d/winegui-apt-publisher.conf
sudo systemd-tmpfiles --create /etc/tmpfiles.d/winegui-apt-publisher.conf
sudo systemctl daemon-reload
sudo systemd-analyze security winegui-apt-publisher.service
sudo systemctl enable --now winegui-apt-publisher.path winegui-apt-publisher.timer
sudo systemctl start winegui-apt-publisher.service
sudo systemctl status winegui-apt-publisher.service winegui-apt-publisher.path winegui-apt-publisher.timer
```

The path unit reduces latency; the persistent timer recovers missed events and interrupted runs. A deployer download failure remains in its own `quarantine/`; a publisher validation failure moves the claimed batch to `publisher-quarantine/` with a reason and never imports it. Signing, export, disk, or publication failures remain journaled and are retried by reconciliation. Do not edit a quarantined batch in place: diagnose it, fix the producing pipeline, and deliver a new batch identity.

All repository maintenance must share `/run/lock/winegui-apt-publisher.lock`:

```sh
sudo -u winegui-apt flock /run/lock/winegui-apt-publisher.lock REPLACE_MAINTENANCE_COMMAND
```

## 6. Static HTTPS and monitoring

Obtain the certificate using the server's established ACME process, install the reviewed Angie vhost, and validate before reload:

```sh
sudo install -o root -g root -m 0644 ops/apt-repository/angie/apt.winegui.melroy.org.conf \
  /etc/angie/http.d/apt.winegui.melroy.org.conf
sudo angie -t
sudo systemctl reload angie
curl --fail --silent --show-error --head https://apt.winegui.melroy.org/dists/noble/InRelease
```

Adapt the included Monit checks to the server's existing include layout, run `monit -t`, then reload Monit. Alerts must cover publisher failure/staleness, non-empty quarantine, repository space/inodes, signing-key expiry, certificate expiry, and external HTTPS. The sample covers all except key/certificate expiry, which should reuse the host's established certificate/key monitoring rather than introduce a competing mechanism.

## 7. Backup, restore, and rollback

`winegui-apt-backup` takes a tar snapshot of private database/configuration, online signing material, journals/spool, and public repository while holding the same global lock. Store the resulting archive encrypted and off-host; protect it as a private key backup. The script uses `--one-file-system`, so if these roots are separate filesystems, back them up individually under one held lock or use one atomic filesystem snapshot.

Test restore only into disposable paths first:

```sh
mkdir -p /tmp/winegui-apt-restore-test
tar --extract --gzip --numeric-owner --file REPLACE_BACKUP.tar.gz \
  --directory /tmp/winegui-apt-restore-test
find /tmp/winegui-apt-restore-test -maxdepth 4 -type f -print
```

For a real restore: stop the deployer and publisher path/timer, take a final forensic copy, hold the publisher lock, restore private and public roots as one consistent generation, verify ownership/modes, run `reprepro check` against the restored private DB, verify every Release signature and referenced SHA-256 object, then start the publisher and deployer. Never combine a database from one backup with `pool/` or `dists/` from another.

Rollback of a bad release is repository maintenance, not file deletion: stop automatic intake, preserve the batch/journal, use the pinned engine under the lock to remove only the exact package version from each affected suite, regenerate by-hash/signatures through the publisher workflow, and verify APT clients before resuming. Never delete old by-hash objects during rollback.

## 8. Launch acceptance

Before announcing the repository, verify all of the following:

- One valid release publishes exactly five `winegui`/`amd64` packages with expected suite-specific internal versions.
- An identical webhook retry is archived harmlessly; changed bytes for an existing package/version/architecture are rejected.
- Missing, extra, symlinked, malformed, wrong-suite, wrong-version, and checksum-mismatched batches enter quarantine before import.
- Restart/failure tests pass at claim, copy, import, export, signing, publication, and post-publication cleanup boundaries.
- Clean clients for all five suites can `apt update`, install latest, select an older retained version, and upgrade again.
- Repeated `apt update` during publication produces no hash mismatch or missing by-hash object.
- The package-scoped `Signed-By` behavior, OS-suite switch, key overlap, missed-rotation recovery, and fresh old-DEB installation all pass.
- Unattended signing works after reboot, monitoring alerts arrive, and a consistent backup restores successfully into a disposable target.

Only the server owner performs deployment and launch. Keep the previous download-only path active until these checks pass; rollback is disabling the new webhook/path/timer and continuing to serve the last known-good static repository generation.
