# WineGUI

At last, a user-interface friendly [Wine](https://www.winehq.org/) (A compatibility layer capable of running Windows applications under Linux) Manager.

[![Pipeline](https://gitlab.melroy.org/melroy/winegui/badges/main/pipeline.svg)](https://gitlab.melroy.org/melroy/winegui/-/pipelines/latest)
[![Telegram](https://img.shields.io/badge/chat-on%20telegram-brightgreen)](https://t.me/winegui)
[![Matrix](https://img.shields.io/badge/chat-on%20matrix-brightgreen)](https://matrix.to/#/#winegui:melroy.org)
[![Release](https://img.shields.io/badge/release-latest-orange)](https://gitlab.melroy.org/melroy/winegui/-/releases)

![WineGUI](misc/winegui_screenshots.gif)

## Download

You can find the latest version on the [Releases page](https://gitlab.melroy.org/melroy/winegui/-/releases) of GitLab.

Download the WineGUI package you require for your Linux distribution (we provide `.deb`, `.rpm` and `.tar.gz` files). Typically you should use `.deb` file for Ubuntu and Linux Mint distros.

Install the package and you are ready to go! WineGUI should be listed in your menu.

## Features

- **Graphical user-interface** on top of [Wine](https://www.winehq.org/)
- Creating a new machine using an **easy step-by-step wizard**
- **Application list** per machine (with _search feature_ and refresh button)
- Editing and removing Windows machines _in a breeze_
- Configure window **installing additional software** with just a single click (like installing DirectX)
- **One-button click** to run a program, open the `C:` drive, simulate a reboot or kill all processes

## GitHub Star History

[![Star History Chart](https://api.star-history.com/svg?repos=winegui/WineGUI&type=Date)](https://star-history.com/#winegui/WineGUI&Date)

---

## Development

WineGUI is created by using [GTK3 toolkit](https://www.gtk.org/) (Gtkmm C++-interface) and C++ code.

Development has been done in VSCcodium, using the following extensions:

- C/C++
- CMake
- CMake Tools
- GitLab Workflow

### Documentation

See latest [WineGUI Developer Docs](https://gitlab.melroy.org/melroy/winegui/-/jobs/artifacts/main/file/doc/doxygen/index.html?job=test-build).

### Requirements

Dependencies should be met before build:

- gcc/g++ (advised: v8 or later)
- cmake (advised: v3.10 or later)
- ninja-build
- libgtkmm-3.0-dev (implicit dependency with libgtk-3-dev)
- libjson-glib-dev
- pkg-config

Optionally:

- doxygen
- graphviz
- rpm
- clang-format (v14)
- cppcheck (v2.10 or higher)

**Hint:** You could execute `./scripts/deps.sh` script for Debian based systems (incl. Ubuntu and Linux Mint) in order to get all the dependencies installed automatically.

### Build

Run: `./scripts/build.sh`

Or execute:

```bash
# Prepare
cmake -GNinja -B build
# Build WineGUI
cmake --build ./build
```

### Run

Execute: `ninja -C build run`

Or execute the binary directly:

```sh
./build/bin/winegui
```

### Rebuild

Configuring the ninja build system via CMake is often only needed once (`cmake -GNinja -B build`), after that just execute:

```bash
cmake --build ./build
```

Or just: `ninja` within the build directory.  
Clean the build via: `ninja clean`.

_Hint:_ Run `ninja help` for all available targets.

### Debug

You can use the helper script: `./scripts/build_debug.sh`

Start debugging in [GDB (GNU Debugger)](https://cs.brown.edu/courses/cs033/docs/guides/gdb.pdf):

```sh
cd build_debug
gdb -ex=run bin/winegui
```

### Memory check

First **build** the (Linux) target including _debug symbols_. Binary should be present in the `build/bin` directory.

Next, check for memory leaks using `valgrind` by executing:

```sh
./scripts/valgrind.sh
```

Or to generate a memory usage plot in [massif format](https://valgrind.org/docs/manual/ms-manual.html), execute:

```sh
./scripts/valgrind_plot.sh
```

### Production

For production build and DEB file package, you can run: `./scripts/build_prod.sh`

Or use:

```sh
cmake -GNinja -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=Release -B build_prod

cmake --build ./build_prod --config Release
```

### Build Doxygen

Or build with generated doxygen files locally:

```sh
cmake -GNinja -Ddoc=ON -B build_docs
cmake --build ./build_docs --target Doxygen
```

### Releasing

Before you can make a new release, align the version number in WineGUI with the version you want to release.
Then create a new tagged version in Gitlab with the same version name.

_Note:_ Only a `release tag` on the `main` branch will trigger the publish task.

### Automated Clang Format

We use our [own Clang LLVM C++ Programming Style Format](.clang-format), using [clang-format](https://clang.llvm.org/docs/ClangFormat.html) command.

To automatically comply to our style format execute following script (inplace edits are performed for you):

```sh
./scripts/fix_format.sh
```

Or depend on the docker image instead of your local `clang-format`:

```sh
./scripts/fix_format.sh docker
```

Check only for errors, run: `./scripts/check_format.sh` (same idea with Docker, run: `./scripts/check_format.sh docker` to not depend on your local `clang-format` tool)

### Guidelines

First we try to use the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) as basis.

Next, we also tend to follow the popular [C++ Core Guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) as much as possible.

### CI/CD

For continuous integration & delivery we use our [Dockerfile](misc/Dockerfile) to create a Docker image.
This image (`danger89/gtk3-docker-cmake-ninja`) is hosted on [Dockerhub](https://hub.docker.com/r/danger89/gtk3-docker-cmake-ninja).

A helper script can be used: `./scripts/build_and_upload_image.sh`
