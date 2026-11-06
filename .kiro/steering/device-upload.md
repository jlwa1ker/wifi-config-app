# Device Upload Workflow

## Tool

Use `arduino-cli` for compiling and uploading sketches.

## Board

- **Board**: Adafruit Feather M0 (SAMD21)
- **FQBN**: `adafruit:samd:adafruit_feather_m0`
- **Port**: USB serial — commonly COM5, but **not fixed** (see Notes)
- **Core**: `adafruit:samd`

> **Port is not stable.** The COM port assigned to the board can change when the
> board resets (including the reset triggered by an upload or by opening a serial
> monitor with DTR asserted) or when it is unplugged and reconnected. It has been
> observed on COM11 as well as COM5. Always run `arduino-cli board list` to
> confirm the current port before uploading or monitoring, rather than assuming
> COM5.

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
- The board's COM port is not fixed — it can change after a reset (an upload
  resets the board, as does opening a serial monitor with DTR asserted) or after
  an unplug/reconnect. Do not assume COM5.
- Run `arduino-cli board list` to confirm the current port before each upload or
  monitor session, and use whatever port it reports (e.g. COM11) in the
  `-p <PORT>` argument
