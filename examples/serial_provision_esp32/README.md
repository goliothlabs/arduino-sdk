### Provisioning device credentials over USB/Serial

Example showing how to remove Golioth credentials from the firmware code and setting them up over USB/Serial. This is useful for having the same firmware for your whole fleet of devices and being able to set up them later with they specific credentials.

> Configuration is saved on ESP32 NVS Store.

Add the following to libraries to your `platformio.ini` file:

```
lib_deps =
  https://github.com/goliothlabs/arduino-sdk.git
  SimpleCLI
  SimpleSerialShell
```

This removes some warnings when entries doesn't exist yet on NVS Store.

```
build_flags =
    -D CORE_DEBUG_LEVEL=0
```

Commands available:

- `hwid`: returns device serial number
- `reset`: restart device
- `wifi ssid <value>`: sets wifi SSID
- `wifi psk <value>`: sets wifi password
- `wifi-clear`: clears wifi credentials
- `golioth pskid <value>`: sets Golioth PSK ID
- `golioth psk <value>`: sets Golioth PSK
- `golioth-clear`: clears Golioth credentials
