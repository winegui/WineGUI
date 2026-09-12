# WineGUI Debian packaging

WineGUI DEBs always receive a distribution-specific internal version. A release
build can additionally embed the public WineGUI repository key and safely manage
`/etc/apt/sources.list.d/winegui.sources`.

Repository support is opt-in. Set `WINEGUI_APT_ENABLED=ON` and provide all of:

- `WINEGUI_APT_PUBLIC_KEY_FILE`: path supplied by a protected GitLab file-type
  variable containing a public OpenPGP export. Secret key packets are rejected.
- `WINEGUI_APT_KEY_FINGERPRINTS`: comma-separated full primary fingerprints in
  that export.
- `WINEGUI_APT_KEY_GENERATION`: positive, monotonically increasing integer.

Release CI enables this mode for canonical tags and fails before compilation if
any input is absent or invalid. Ordinary local and non-release Trixie builds do
not need a repository key and remain installable without configuring APT.

The maintainer scripts support Noble, Plucky, Resolute, Trixie, and Forky,
including compatible derivatives that identify their upstream through `ID`,
`ID_LIKE`, and `UBUNTU_CODENAME` in `/etc/os-release`. This includes Linux Mint,
elementary OS, Zorin OS, Pop!_OS, compatible AnduinOS releases, and MX Linux when
their upstream suite is published by WineGUI. Unsupported upstream suites such
as Questing are never redirected to a different suite; the package installs
normally but does not create a source. After an OS upgrade,
`sudo dpkg-reconfigure winegui` updates a source that still matches WineGUI's
last generated content. An administrator-edited source is preserved; update its
`Suites:` field manually when switching to a different suite. Deleting the
source is a persistent opt-out.

Increasing the key generation allows a new package to install an overlapping
public keyring. Installing or purging an older retained DEB cannot replace or
remove trust state owned by a newer generation. A purge removes unmodified
WineGUI-managed files, but retains an edited source and any WineGUI keyring it
still references.

Run focused checks with:

```sh
python3 -m unittest discover -s packaging/debian/tests -v
```
