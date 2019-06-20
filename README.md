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
