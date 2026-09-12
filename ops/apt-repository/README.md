# WineGUI APT repository

This directory contains the production publisher and supporting configuration
for `https://apt.winegui.melroy.org`. It does not contain generated repository
data, certificates, or OpenPGP keys.

## Publication flow

1. A tagged GitLab pipeline builds one DEB for each supported suite.
2. The `apt-repository` job creates one validated batch artifact.
3. `winegui-apt-artifact-deployer` downloads that exact job artifact into
   `/var/spool/winegui-apt/ready/`.
4. The native `winegui-apt` publisher validates and imports the batch.
5. Angie serves the resulting static repository from
   `/var/www/apt.winegui.melroy.org/html`.

The repository root intentionally returns HTTP 404: directory listings and a
landing index are disabled. Before the first successful release publication,
suite paths such as `/dists/noble/InRelease` also return 404. APT clients use
the signed files under `dists/` and packages under `pool/`; they do not use the
site root.

The deployer can write only its spool. A default ACL inherited from the
deployer-owned `pending/` directory lets the native publisher atomically claim
only completed batch directories from `ready/`. The native publisher owns the
private repository state, signing home, and public web root. A single host lock
covers publication, recovery, maintenance, and backups.

## Directory contents

- `reprepro/`: reproducible build pinned to upstream `reprepro` 5.5.1.
- `publisher/`: batch validation, import, signing, and publication logic.
- `config/`: `reprepro` and publisher configuration.
- `systemd/`: publisher service, path trigger, timer, and lock file setup.
- `compose/`: reference configuration for the artifact deployer.
- `angie/`: reference static vhost and webhook location.
- `monitoring/`: reference Monit checks.
- `tests/`: publisher contract tests and the real multi-version Docker proof.

Run both test suites before installing or upgrading the publisher:

```sh
./ops/apt-repository/tests/run.sh
./ops/apt-repository/tests/prove-pinned-reprepro.sh
```

The Docker proof verifies the exact pinned engine, gzip and zstd DEBs,
`Limit: 0`, duplicate retries, and two versions visible through APT.

See [RUNBOOK.md](RUNBOOK.md) for production setup and operation.

## Required key lifecycle maintenance

Repository signing is not a one-time setup. Monitor both active key expiry
dates and start rotation before the documented deadlines:

- signing subkey expires 2028-09-11; start renewal by 2028-03-15;
- certification primary expires 2031-09-11; start replacement by 2030-09-11.

Follow the complete [signing-key expiry and rotation
procedure](RUNBOOK.md#8-signing-key-expiry-and-rotation). It defines the
old-key-signed bridge releases, minimum overlap, GitLab keyring generations,
server switch, rollback boundary, client tests, and recovery for installations
that miss the transition. Do not switch repository signatures without
completing that procedure.

## Batch contract

Each `ready/<batch-id>/` contains `batch.json` and one
`apt_repository_batch/` directory. The payload contains only `manifest.json`,
`SHA256SUMS`, and five `WineGUI-v<version>-<suite>.deb` files for Noble,
Plucky, Resolute, Trixie, and Forky. The publisher verifies the envelope,
checksums, package metadata, suite-specific Debian versions, producing job IDs,
and the shared rebuild counter before importing anything.
