# Wine GUI

Finally, a user-interface friendly [WINE](https://www.winehq.org/) (A compatibility layer capable of running Windows applications under Linux) Manager.

![WineGUI](misc/winegui_screenshot.png)

User-interface is created by using GTK+ (gtkmm c++-interface) and C++ code.

## Download

You can find the latest version on the [Releases page](https://gitlab.melroy.org/melroy/winegui/-/releases).

Download the WineGUI package you require for your Linux distribution (we provided `.deb`, `.rpm` and `.tar.gz` files). Typically you should use `.deb` file for Ubuntu and Linux Mint distros.

Install the package and you are ready to go! WineGUI should be listed in your menu.

## Development

### Requirements

Dependencies should be met before build:

* gcc/g++ (v8.3 or later)
* cmake (v3.13 or later)
* ninja-build
* libgtkmm-3.0-dev (implicit dependency with libgtk-3-dev obviously)
* pkg-config

Optionally:

* doxygen
* graphviz
* rpm
* cppcheck

**Hint:** You could execute `./deps.sh` script for Debian based systems (incl. Ubuntu and Linux Mint) in order to get all the dependencies installed automatically.

### Build

Run: `./build.sh`

Or execute: `mkdir -p build && cd build && cmake -GNinja .. && ninja`

### Run

Execute within the build directory: `ninja run`

Or:

```sh
./build/bin/winegui
```

### Rebuild

Cmake is only needed once, after that you can often use:

`ninja`

Clean the build via: `ninja clean`

*Hint:* Run `ninja help` for all available targets.

### Debug

You can use the helper script: `./build_debug.sh`

Start debugging in [GDB (GNU Debugger)](https://cs.brown.edu/courses/cs033/docs/guides/gdb.pdf):

```sh
cd build_debug
gdb -ex=run bin/winegui
```

### Production

For production build and DEB file package, you can run: `./build_prod.sh`

Or use the `cmake -DCMAKE_BUILD_TYPE=Release`.

### Releasing

Before you can make a new release, align the version number in WineGUI with the version you want to release.
Then create a new tagged version in Gitlab with the same version name.

*Note:* Only the `master` branch will trigger the publish task.


## Coding standard

We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

## CI

For Contious Integration on the [Gitlab server](https://gitlab.melroy.org), we use our [Dockerfile](Dockerfile) as input for `danger89/gtk3-docker-cmake-ninja` image.
This image is hosted on [Dockerhub](https://hub.docker.com/r/danger89/gtk3-docker-cmake-ninja), and managed by [danger89](https://hub.docker.com/u/danger89).

A helper script can be used: `./upload_docker_image.sh` from within the `misc` directory, after successfully identified via `docker login --username=<username>`.

## Coding in the Cloud

### Cloud IDE

You may use our [Cloud IDE](https://ide.melroy.org) for developing remotely.

### How-to develop on a remote server

Execute the following command to copy the binary from the Cloud IDE (Coder) to your local machine & execute WineGUI:

```sh
scp melroy@server.melroy.org:/media/data/coder_projects/project/winegui/build/bin/winegui ~ && ~/winegui
```

*Note #1:* Copy your local ~/.ssh/id_ed25519.pub and add it to the remote machine file: `/home/melroy/.ssh/authorized_keys`
*Note #2:* Requires the `libgtkmm-3.0-1v5` library to be installed on the local machine.
