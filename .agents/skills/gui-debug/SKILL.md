---
name: gui-debug
description: Launch the WineGUI GTK app on the live X display, drive it with xdotool (click toolbar buttons, open windows), and capture screenshots to visually verify a UI change end-to-end. Use whenever you need to see a GTK/GUI change actually rendered — new buttons, layout changes, dialogs — rather than only checking that it compiles.
---

# GUI debugging & screenshot capture (WineGUI / GTK)

Goal: after building, actually *see* the change in the running app. Compiling is not
verification for a UI change — a button can compile and still be wired to the wrong
row, mislabeled, or invisible. Launch it, drive it, screenshot it.

## Prerequisites (check once)

```bash
echo "DISPLAY=$DISPLAY WAYLAND_DISPLAY=$WAYLAND_DISPLAY"   # need a real X display, e.g. :0
which xdotool import                                       # xdotool drives, ImageMagick `import` captures
```

- A real display (`DISPLAY=:0`) means you can screenshot the actual app. If there is
  no display, `xvfb-run` gives a headless one, but you cannot eyeball those captures —
  prefer a real display when the machine has one.
- Build first, in the **same directory you are working in** (see the pitfall below).

## The one pitfall that will waste your time: worktree vs main tree

If you are in a git **worktree**, your edits and your build **must** target the worktree
copy, not the original checkout. It is easy to edit the main-tree absolute path by
accident, build the worktree, and get a binary with *none* of your changes.

Always confirm your changes are actually in the binary before screenshotting:

```bash
# Pick a unique string you added (a label, tooltip, message) and grep the binary:
strings build_debug/bin/winegui | grep -i "Reinstall Wine Mono"
```

If that prints nothing, the binary is stale or you edited the wrong tree. Fix that
*before* launching — otherwise you will screenshot the old UI and be confused.

## Build

```bash
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -B build_debug
cmake --build ./build_debug --config Debug
```

## Launch (background, capture the log)

Run it in the background and keep stderr — GTK CSS/theme warnings are normal noise and
not your bug.

```bash
DISPLAY=:0 ./build_debug/bin/winegui > /tmp/.../scratchpad/winegui.log 2>&1 &
sleep 4   # let it start and paint
```

## Find the RIGHT window — search by class, not by name

`xdotool search --name "WineGUI"` also matches unrelated windows (e.g. a browser tab
titled "WineGUI · GitLab"), and you will screenshot the wrong thing. Prefer the window
**class**, and cross-check the owning PID:

```bash
WID=$(DISPLAY=:0 xdotool search --class "winegui" | head -1)
DISPLAY=:0 xdotool getwindowpid $WID          # should match the pid you launched
```

## Drive the UI

`windowactivate --sync` before interacting; `mousemove --window` uses window-relative
coordinates (read them off a screenshot of the toolbar).

```bash
DISPLAY=:0 xdotool windowactivate --sync $WID
DISPLAY=:0 xdotool mousemove --window $WID 417 50 click 1   # e.g. the "Configure" toolbar button
sleep 2
```

Opening a dialog spawns a new top-level window — find it, then screenshot *it*:

```bash
CFG=$(DISPLAY=:0 xdotool search --name "Configure machine" | head -1)
DISPLAY=:0 xdotool windowactivate --sync $CFG; sleep 1
```

## Capture

```bash
DISPLAY=:0 import -window $CFG /tmp/.../scratchpad/configure.png
```

Then read the PNG back with the Read tool to actually look at it. Verify the specific
thing you changed: correct label, correct row/position, correct icon, no overlap.

## Clean up

```bash
kill <pid>   # the pid you launched; don't leave stray app instances running
```

## Don't trigger destructive actions against live data

Clicking a button that starts a real install, download, or wineboot mutates the user's
actual Wine prefix / bottle. Verifying the button *renders and is wired* (compiles +
signal connected + appears in the right place) is usually enough. If you must exercise
the full action, do it against a throwaway bottle, and ask first.
