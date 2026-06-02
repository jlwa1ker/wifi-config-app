# Diagnostics

## Serial Monitor

Use serial monitoring on COM5 at 115200 baud as the primary diagnostic tool for runtime issues on the device.

```
arduino-cli monitor -p COM5 --config baudrate=115200,dtr=off
```

**Important**: Use `dtr=off` to prevent resetting the board when the monitor connects. The serial monitor holds the COM port exclusively. You must stop the serial monitor before uploading a sketch, then restart it after the upload completes.

Workflow:
1. Stop the serial monitor
2. Compile and upload the sketch
3. Re-enter WiFi credentials via the AP portal (upload erases flash storage)
4. Start the serial monitor after the device reconnects in STA mode

**Flash storage caveat**: The SAMD21 FlashStorage library stores credentials in program flash. Uploading a new sketch overwrites the entire flash, erasing stored WiFi credentials. The device will boot into AP mode after every upload and must be reconfigured.

When debugging device behavior:
1. Add `Serial.println()` statements to trace execution flow
2. Log error codes, state transitions, and network results
3. Compile and upload the updated sketch
4. Open the serial monitor to observe output

## Logging Conventions

- Log state transitions (e.g., "NTP sync succeeded", "Upload FAILED")
- Log error codes as integers for enum-based results (e.g., `ReportResult`)
- Log counts and values that help diagnose timing or data issues
- Keep log messages concise — the serial buffer is limited on SAMD21
