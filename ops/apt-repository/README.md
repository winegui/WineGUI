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

The deployer can write only its spool. The native publisher owns the private
repository state, signing home, and public web root. A single host lock covers
publication, recovery, maintenance, and backups.

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

## Batch contract

Each `ready/<batch-id>/` contains `batch.json` and one
`apt_repository_batch/` directory. The payload contains only `manifest.json`,
`SHA256SUMS`, and five `WineGUI-v<version>-<suite>.deb` files for Noble,
Plucky, Resolute, Trixie, and Forky. The publisher verifies the envelope,
checksums, package metadata, suite-specific Debian versions, producing job IDs,
and the shared rebuild counter before importing anything.
