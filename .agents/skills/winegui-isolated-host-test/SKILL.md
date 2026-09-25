---
name: winegui-isolated-host-test
description: Reproduce WineGUI launch or shortcut behavior on the installed Linux host with a throwaway Wine prefix and separate XDG config/data directories. Use when a unit test is insufficient and the real GTK app, Wine process, or launch arguments must be observed without changing existing bottles.
---

# Isolated WineGUI host test

Use this for a bounded real-host check. Record the exact source revision and binary used for each run. For GTK interaction and screenshots, also follow the repository's `gui-debug` skill.

## Isolation preflight

- Check `DISPLAY`, `wine`, `wineboot`, `wineserver`, `dbus-run-session`, `xdotool`, and `import`. A live X display allows actual windows to be inspected.
- Check whether `~/.winegui` exists. `GeneralConfigFile::config_and_folder_migration()` can move or remove files there even when `XDG_CONFIG_HOME` is changed. If it exists, use a dedicated test OS account or another environment that isolates that home directory; do not launch this fixture under the affected account.
- Create a unique directory with `mktemp -d /tmp/winegui-live-XXXXXX`. Keep its resolved absolute path. Place all fixture config, data, working-directory files, logs, and screenshots beneath it. Do not repurpose `HOME`.
- Check for an already running WineGUI instance. Launch the test app through `dbus-run-session` so GTK's single-instance application ID does not activate a user's existing window.

## Prepare the fixture

Create `<fixture>/config/winegui`, `<fixture>/data/winegui/prefixes`, and `<fixture>/cwd`. Before starting WineGUI, create `<fixture>/config/winegui/config.ini` with:

```ini
[General]
DefaultFolder=<fixture>/data/winegui/prefixes
DisplayDefaultWineMachine=false
EnableLoggingStderr=false
CheckForUpdatesStartup=false
```

Replace `<fixture>` with the actual absolute path. `DisplayDefaultWineMachine=false` prevents the user's `~/.wine` prefix from appearing in the test app. Set both `XDG_CONFIG_HOME=<fixture>/config` and `XDG_DATA_HOME=<fixture>/data` on every WineGUI launch.

Initialize only a throwaway prefix, for example `env WINEPREFIX=<fixture>/data/winegui/prefixes/LiveCase WINEDEBUG=-all wineboot -u`. Create its `winegui.ini` using the current `BottleConfigFile` keys, including an effective `CpuCoreLimit` for affinity tests and `[Application.N]` entries for the commands under test. Keep `BinaryPath` empty for the system Wine runner unless tracing arguments as described below. Use `<fixture>/cwd` as the app's process working directory when testing relative paths.

For a real executable with a spaced relative name, a symlink in `<fixture>/cwd` to a Wine-provided PE program such as `notepad.exe` can make the outcome visible without installing software into a user bottle. Discover the actual PE path on the host; do not assume a distribution-specific location. Confirm it is a test fixture before linking.

## Compare actual launch arguments

When the difference is argument splitting, optionally create an executable `<fixture>/runner-bin/wine` wrapper that records each argument and then executes the real system Wine binary with `"$@"`. Point the test bottle's `BinaryPath` at that directory and provide a `wineserver` symlink to the system wineserver. Keep the log under `<fixture>` and distinguish version probes such as `--version` from application launches. The wrapper must still call real Wine; it is an observer, not a fake runner.

Build baseline and candidate binaries from their exact source trees. A detached temporary worktree is suitable for the baseline when the current checkout has uncommitted candidate changes. Launch one binary at a time from `<fixture>/cwd`, for example:

```text
dbus-run-session -- env XDG_CONFIG_HOME=<fixture>/config XDG_DATA_HOME=<fixture>/data GSETTINGS_SCHEMA_DIR=<binary-build-dir> WINEDEBUG=-all <binary>
```

Find the window by the launched PID (`xdotool search --pid <pid>`), confirm it is viewable, and drive the fixture application row. Capture the Wine arguments and the target window or failure. A missing window alone is inconclusive; pair it with the recorded arguments or another direct observation. Repeat with the candidate binary under the same fixture conditions.

## Finish and report

State which revision produced which arguments and whether the real Windows program opened. Keep unit tests, local build/static checks, and the live host result as separate evidence. The local GUI result does not prove a packaged or installed release works.

Close only fixture windows. Stop only the verified test WineGUI PID, then use `WINEPREFIX=<fixture>/data/winegui/prefixes/LiveCase wineserver -k` if test Wine processes remain. Remove only the temporary worktree and fixture directory that this run created, after preserving any evidence needed for the report. Do not kill unrelated WineGUI or Wine processes.
