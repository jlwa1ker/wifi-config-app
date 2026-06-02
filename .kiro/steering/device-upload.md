# Device Upload Workflow

## Tool

Use `arduino-cli` for compiling and uploading sketches.

## Board

- **Board**: Adafruit Feather M0 (SAMD21)
- **FQBN**: `adafruit:samd:adafruit_feather_m0`
- **Port**: COM5 (USB serial)
- **Core**: `adafruit:samd`

## Commands

Compile the sketch:

```
arduino-cli compile --fqbn adafruit:samd:adafruit_feather_m0 c:\Users\walke\git\wifi-config-app
```

Upload to the device:

```
arduino-cli upload -p COM5 --fqbn adafruit:samd:adafruit_feather_m0 c:\Users\walke\git\wifi-config-app
```

List connected boards (to verify port):

```
arduino-cli board list
```

## Notes

- Always compile before uploading to catch errors early
- If COM5 is not found, run `arduino-cli board list` to check the current port
- The board may appear on a different port after a reset or reconnection
