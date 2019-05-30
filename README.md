# Wine GUI
Finally, an user-interface friendly WINE manager.

## Development

### Requirements

Dependencies should be met before build:

* cmake
* GTK3.0+
* pkg-config


### Build

Run: `./build.sh`

### Run

Execute:
`./build/bin/winegui`

Or go to `build` directory and execute:

```
ninja run
```

### Rebuild

Cmake is only needed once, after that you can often use:

`ninja`

Clean the build via: `ninja clean`

*Hint:* Run `ninja help` for all available targets.


