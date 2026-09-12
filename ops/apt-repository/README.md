# WineGUI APT repository operations

This tree contains the host-side publisher, static repository configuration, disposable tests, and reviewed examples for `apt.winegui.melroy.org`. It deliberately does not contain a signing key, secret, certificate, generated repository, or production state.

Creating the service account, pinned `reprepro`, signing key, filesystem layout,
and systemd units is a one-time host setup. After that, ordinary tagged releases
flow through GitLab, the durable spool, the publisher, and Angie without copying
the complete repository or running manual publication commands. Repeat host
setup only after a restore, migration, deliberate key rotation, or reviewed
publisher/engine upgrade.

The publisher consumes immutable handoff directories from the artifact deployer's `ready/` spool. It atomically claims a batch, copies it outside the container mount, validates all five DEBs before any import, updates the private `reprepro` database with export disabled, generates indexes and SHA-256 by-hash objects in private storage, signs them, and exposes files in APT-safe order. A host-wide `flock` serializes publication, reconciliation, maintenance, and backups.

On the production host Docker uses user-namespace remapping. The container owns
only `/var/spool/winegui-apt`; its mapped UID/GID must be measured rather than
assumed. The native `winegui-apt` account owns the private state and public web
root. Never chown the public APT root to Docker's mapped identity and never mount
the public root, repository database, or signing home into the deployer.

Start with [RUNBOOK.md](RUNBOOK.md). Run fast local contract tests with:

```sh
./ops/apt-repository/tests/run.sh
```

The much slower, networked Docker proof is separate:

```sh
./ops/apt-repository/tests/prove-pinned-reprepro.sh
```

The fast suite uses a fake `reprepro` and fake signer to test publisher contracts and failure recovery. It does **not** prove that upstream `reprepro`, GPG, APT, Angie, Docker user namespaces, or the production filesystem work together. The Docker proof builds the official upstream release and exact commit recorded in `reprepro/version.env`, proves its version, imports gzip and zstd DEBs, and proves two versions remain indexed and visible to APT in one suite. Both gates are required before production.

## Batch artifact contract

Each deployer `ready/<batch-id>/` directory contains the deployer's root
`batch.json` envelope and one `apt_repository_batch/` directory. The envelope
must identify the `winegui-apt-repository` environment and the same full commit
SHA as the release manifest, and its size/checksum inventory must exactly match
the copied payload.

The payload directory contains only `manifest.json`, `SHA256SUMS`, and five
`WineGUI-v<version>-{noble,plucky,resolute,trixie,forky}.deb` files.
`SHA256SUMS` contains exactly one GNU-style SHA-256 line per DEB plus the
manifest checksum.

`manifest.json` schema 1 has exactly `schema_version`, `release_tag`,
`commit_sha`, `architecture`, and `packages`. `packages` is keyed by the five
suites; each value contains exactly `filename`, `version`, `size`, `sha256`,
and positive `producing_job_id`. Internal versions use one shared positive
rebuild counter for the release batch.
