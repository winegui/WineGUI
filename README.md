# Wine GUI
Finally, an user-interface friendly [WINE](https://www.winehq.org/) (A compatibility layer capable of running Windows applications under Linux) Manager.

User-interface is created by using GTK+ (gtkmm c++-interface) and C++ code.
An helper Windows application for fetching Wine bottle details is created by WineGUI developers, in order to give additional information regarding each bottle.

## Development

### Requirements

Dependencies should be met before build:

* gcc/g++ (v8.3 or later)
* cmake (v3.13 or later)
* ninja-build
* libgtkmm-3.0-dev (implicit dependency with libgtk-3-dev obviously)
* pkg-config
* doxygen
* graphviz

**Hint:** You could execute `./deps.sh` script for Debian based systems (incl. Ubuntu and Linux Mint) in order to get all the dependencies installed automatically.

### Build

Run: `./build.sh`

### Run

Execute:
`./build/bin/winegui`

Or go to the `build` directory and execute:

```
ninja run
```

### Rebuild

Cmake is only needed once, after that you can often use:

`ninja`

Clean the build via: `ninja clean`

*Hint:* Run `ninja help` for all available targets.

## Coding standard

We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

## How-to develop on a remote server (Cloud IDE)

Execute the following command to copy the binary from the Cloud IDE (Coder) to your local machine & execute WineGUI:

```
scp melroy@server.melroy.org:/media/data/coder_projects/project/winegui/build/bin/winegui ~ && ~/winegui
```

*Note #1:* Copy your local ~/.ssh/id_ed25519.pub and add it to the remote machine file: `/home/melroy/.ssh/authorized_keys`
*Note #2:* Requires the `libgtkmm-3.0-1v5` library to be installed on the local machine.