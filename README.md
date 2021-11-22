## Golioth Arduino SDK

Access [Golioth](https://golioth.io) device service via Arduino SDK.

### How to build PlatformIO based project

- [Install PlatformIO IDE](https://docs.platformio.org/en/latest/integration/ide/pioide.html)
- Clone/Download this project and open our your favorite IDE with PlatformIO installed
- Change target on the `platformio.ini` file.
- Add `https://github.com/goliothlabs/arduino-sdk.git` as a dependency on `lib_deps`

```
lib_deps =
  https://github.com/goliothlabs/arduino-sdk.git
```
