> :warning: This project is considered experimental and is not recommended for production use. Functionality may break at any time.

> :heavy_exclamation_mark: This SDK is currently no longer functioning as a result of the MQTT BETA ending. Migration to CoAP is being evaluated but not currently planned.

## Golioth Arduino SDK

Access [Golioth](https://golioth.io) device service via Arduino SDK.

### How to build PlatformIO based project

- [Install PlatformIO IDE](https://docs.platformio.org/en/latest/integration/ide/pioide.html)
- Create a new project and open our your favorite IDE with PlatformIO installed.
- Change target on the `platformio.ini` file.
- Add `https://github.com/goliothlabs/arduino-sdk.git` as a dependency on `lib_deps`.

```
lib_deps =
  https://github.com/goliothlabs/arduino-sdk.git
```
