# Embedded Architecture

## Platform

- **Framework**: Arduino
- **Microcontroller**: Adafruit Feather M0 (SAMD21)
- **Output**: An Arduino sketch (`.ino` file) is the primary build artifact

## Project Structure

- The sketch MAY be divided into multiple files (`.ino`, `.h`, `.cpp`) as appropriate for clarity and maintainability
- Use separate files to isolate concerns (e.g., WiFi management, web server, credential storage, sensor reading, display)

## Dependencies

- Leverage third-party Arduino libraries for device support and utilities rather than writing low-level drivers from scratch
- Prefer well-maintained, widely-used libraries from the Arduino Library Manager or PlatformIO registry

## Hardware Specification

### Sensor: AHT20 (Temperature & Humidity)

- **I2C address**: 0x38
- **Library**: `Adafruit_AHTX0` (handles I2C initialization internally)
- **Function**: Continuously reads temperature and relative humidity; periodically reports temperature to the tempmon2 web application

### Display: Adafruit SSD1306 OLED

- **I2C address**: 0x3C
- **Library**: `Adafruit_SSD1306`
- **Bus**: Shares the I2C bus with the AHT20

### Communication

- Both the AHT20 and SSD1306 communicate with the Feather M0 via I2C

## Scope

- The current requirements cover WiFi configuration of the Feather M0 only
- Scope will be expanded by refinement to include sensor reading, display output, and periodic reporting to tempmon2
