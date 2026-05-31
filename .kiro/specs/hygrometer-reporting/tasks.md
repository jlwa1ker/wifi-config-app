# Implementation Plan: Hygrometer Reporting

## Overview

This plan implements periodic hygrometer (temperature and humidity) reporting for the Adafruit Feather M0 WiFi device. The implementation adds AHT20 sensor polling, a 60-second running average, NTP time synchronization, a bounded reading cache, and HTTP POST transmission to the tempmon2 server. Each module follows the existing project pattern of C module-level functions with static state and header-exposed APIs. Pure logic is extracted into testable headers for host-based property testing with RapidCheck and Catch2.

## Tasks

- [x] 1. Implement temperature conversion utility and sensor poller
  - [x] 1.1 Create temperature conversion utility header
    - Create `test/temperature_convert.h` with the `celsiusToFahrenheit()` pure inline function
    - Implements the formula: `(temp_c × 9.0 / 5.0) + 32.0`, rounded to 1 decimal place
    - _Requirements: 6.1, 6.2_

  - [x]* 1.2 Write property test for temperature conversion
    - Create `test/test_temperature_convert.cpp`
    - **Property 8: Celsius to Fahrenheit conversion with rounding**
    - Generate random floats in [-40, 85] range and verify conversion formula and single-decimal rounding
    - **Validates: Requirements 6.1, 6.2**

  - [x] 1.3 Create sensor poller module
    - Create `sensor_poller.h` and `sensor_poller.cpp`
    - Implement `sensorPoller_init()` — initializes AHT20 via `Adafruit_AHTX0` library, returns true if sensor detected
    - Implement `sensorPoller_read(float& temp_c, float& humidity_pct)` — reads sensor, returns false on failure
    - _Requirements: 1.1, 1.2, 1.3_

- [x] 2. Implement running average module
  - [x] 2.1 Create running average pure logic header for testing
    - Create `test/running_average_logic.h` with extracted pure logic
    - Implement circular buffer of 60 `AverageSample` structs with `timestamp_ms` tracking
    - Implement `addSample()` that expires samples older than 60 seconds and adds new sample
    - Implement `getAverage()` that returns false if fewer than 10 samples, true with computed mean otherwise
    - _Requirements: 2.1, 2.2, 2.3, 2.5_

  - [x]* 2.2 Write property test for running average correctness
    - Create `test/test_running_average.cpp`
    - **Property 1: Running average computes correct windowed mean**
    - Generate random sample sequences with timestamps; verify average equals arithmetic mean of samples within 60-second window
    - **Validates: Requirements 2.1, 2.2, 2.3**

  - [x]* 2.3 Write property test for minimum sample threshold
    - Add to `test/test_running_average.cpp`
    - **Property 2: Minimum sample threshold gates average validity**
    - Generate random sample counts 0–60; verify `getAverage()` returns false when count < 10, true when count ≥ 10
    - **Validates: Requirements 2.5**

  - [x] 2.4 Create running average Arduino module
    - Create `running_average.h` and `running_average.cpp`
    - Implement `runningAverage_init()`, `runningAverage_addSample()`, `runningAverage_get()`, `runningAverage_sampleCount()`
    - Uses `millis()` for timestamps; includes the logic from `running_average_logic.h`
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5_

- [x] 3. Implement NTP client module
  - [x] 3.1 Create NTP format pure logic header for testing
    - Create `test/ntp_format.h` with ISO 8601 formatting logic
    - Implement epoch-to-ISO-8601 conversion: breaks epoch into year/month/day/hour/minute/second, formats as `YYYY-MM-DDTHH:MM:SS+00:00`
    - _Requirements: 3.5_

  - [x]* 3.2 Write property test for ISO 8601 timestamp format
    - Create `test/test_ntp_format.cpp`
    - **Property 3: ISO 8601 timestamp format round-trip**
    - Generate random epoch values in [2020-01-01, 2099-12-31]; verify format matches pattern and round-trip produces same epoch
    - **Validates: Requirements 3.5**

  - [x] 3.3 Create NTP client Arduino module
    - Create `ntp_client.h` and `ntp_client.cpp`
    - Implement `ntpClient_sync()` with up to 5 retries at 10-second intervals using `WiFiUDP`
    - Implement `ntpClient_isSynced()`, `ntpClient_needsResync()` (24-hour interval), `ntpClient_getEpoch()`, `ntpClient_formatISO8601()`
    - Uses `pool.ntp.org` as NTP server; stores epoch offset from `millis()`
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_

- [x] 4. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 5. Implement reading cache module
  - [x] 5.1 Create reading cache pure logic header for testing
    - Create `test/reading_cache_logic.h` with ring buffer logic
    - Implement `CachedReading` struct (timestamp char[26], temperature_f, humidity_pct, occupied)
    - Implement ring buffer add/remove/count/clear operations with 96-slot capacity
    - _Requirements: 4.1, 4.2, 4.3, 4.4_

  - [x]* 5.2 Write property test for reading cache capacity invariant
    - Create `test/test_reading_cache.cpp`
    - **Property 4: Reading cache capacity invariant**
    - Generate random sequences of add operations (length 1–300); verify count never exceeds 96
    - **Validates: Requirements 4.2**

  - [x]* 5.3 Write property test for ring buffer FIFO order
    - Add to `test/test_reading_cache.cpp`
    - **Property 5: Ring buffer preserves most recent readings in FIFO order**
    - Generate sequences of N > 96 readings; verify cache contains exactly the most recent 96 in insertion order
    - **Validates: Requirements 4.3, 4.4**

  - [x] 5.4 Create reading cache Arduino module
    - Create `reading_cache.h` and `reading_cache.cpp`
    - Implement `readingCache_init()`, `readingCache_add()`, `readingCache_count()`, `readingCache_getAll()`, `readingCache_removeOldest()`, `readingCache_clear()`
    - _Requirements: 4.1, 4.2, 4.3, 4.4_

- [x] 6. Implement server reporter module
  - [x] 6.1 Create JSON serializer pure logic header for testing
    - Create `test/json_serializer.h` with reading-to-JSON serialization logic
    - Serialize an array of `CachedReading` structs into the expected JSON format with "readings" array containing "timestamp", "temperature_f", "humidity_pct", and "location" fields
    - _Requirements: 5.2_

  - [x]* 6.2 Write property test for JSON serialization
    - Create `test/test_json_serializer.cpp`
    - **Property 6: JSON serialization contains all required fields**
    - Generate random arrays of CachedReading structs; verify serialized JSON is valid and contains all required fields with correct values
    - **Validates: Requirements 5.2**

  - [x] 6.3 Create response parser pure logic header for testing
    - Create `test/response_parser.h` with server response JSON parsing logic
    - Parse JSON response body; extract `inserted_count` and `skipped_count`; return sum as removal count
    - _Requirements: 5.3, 5.5_

  - [x]* 6.4 Write property test for response parser
    - Create `test/test_response_parser.cpp`
    - **Property 7: Response parser computes correct removal count**
    - Generate random valid JSON responses with `inserted_count` and `skipped_count`; verify return equals their sum
    - **Validates: Requirements 5.3, 5.5**

  - [x] 6.5 Create server reporter Arduino module
    - Create `server_reporter.h` and `server_reporter.cpp`
    - Implement `serverReporter_send()` using `WiFiClient` to POST JSON to `tempmon.walkerweb.us/ingest` on port 80
    - Implement `serverReporter_parseResponse()` to determine how many readings to remove from cache
    - Handle connection failures, timeouts, and parse errors via `ReportResult` enum
    - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6_

- [x] 7. Checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 8. Integrate modules into main sketch and add OLED display updates
  - [x] 8.1 Add OLED display helper functions for hygrometer status
    - Add `oledShowReadings()`, `oledShowUploadSuccess()`, `oledShowUploadFailed()`, `oledShowNtpError()` functions to `wifi-config-app.ino`
    - Display current temperature (°F), humidity (%), and cache count
    - _Requirements: 7.1, 7.2, 7.3_

  - [x] 8.2 Integrate NTP sync into STA mode boot sequence
    - In `setup()`, after successful WiFi connection in STA mode, call `ntpClient_sync()`
    - On NTP failure after all retries, display error via `oledShowNtpError()` and skip entering the reporting loop
    - Add periodic re-sync check (24-hour interval) in the main loop
    - _Requirements: 3.1, 3.2, 3.3, 3.4_

  - [x] 8.3 Integrate sensor polling and running average into STA mode loop
    - In `loop()` for `STATE_STA_MODE`, poll `sensorPoller_read()` once per second using `millis()` timing
    - Feed valid readings into `runningAverage_addSample()`
    - Update OLED with current averages via `oledShowReadings()`
    - _Requirements: 1.1, 1.2, 1.3, 2.1, 2.2, 2.3, 7.1_

  - [x] 8.4 Integrate reading cache and server reporting into STA mode loop
    - Every 15 minutes, snapshot running average, convert to Fahrenheit, store in reading cache with NTP timestamp
    - Skip caching if running average has fewer than 10 samples
    - After caching, attempt `serverReporter_send()` with all cached readings
    - On success, call `readingCache_removeOldest()` for accepted readings; show `oledShowUploadSuccess()`
    - On failure, retain readings in cache; show `oledShowUploadFailed()`
    - _Requirements: 2.4, 2.5, 4.1, 5.1, 5.3, 5.4, 5.5, 7.2, 7.3_

  - [x] 8.5 Initialize sensor and cache modules in setup
    - Call `sensorPoller_init()` in `setup()` after NTP sync succeeds
    - Call `runningAverage_init()` and `readingCache_init()` in `setup()`
    - If sensor init fails, display error on OLED and do not enter polling loop
    - _Requirements: 1.1, 4.1_

- [x] 9. Update test build configuration
  - [x] 9.1 Update CMakeLists.txt with new test targets
    - Add test executables for `test_temperature_convert`, `test_running_average`, `test_ntp_format`, `test_reading_cache`, `test_json_serializer`, `test_response_parser`
    - Link each with `Catch2::Catch2WithMain` and `rapidcheck`
    - Link JSON tests with `ArduinoJson`
    - Add necessary include directories and mock headers
    - _Requirements: All (testing infrastructure)_

  - [x] 9.2 Add mock headers for new Arduino dependencies
    - Create/update mock headers in `test/mocks/` for `Adafruit_AHTX0`, `WiFiUdp.h`, and any other new Arduino-specific APIs used by the modules
    - Ensure mocks provide sufficient stubs for host-based compilation
    - _Requirements: All (testing infrastructure)_

- [x] 10. Final checkpoint - Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document
- Unit tests validate specific examples and edge cases
- Pure logic is extracted into testable headers following the existing project pattern (`test/health_parser.h`, `test/url_decode.h`)
- All Arduino-specific code lives in `.cpp`/`.h` module files; pure logic in `test/*.h` headers for host-based testing
- The implementation language is C++ (Arduino framework) matching the existing project

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1", "2.1", "3.1", "5.1", "9.2"] },
    { "id": 1, "tasks": ["1.2", "1.3", "2.2", "2.3", "3.2", "5.2", "5.3", "6.1", "6.3"] },
    { "id": 2, "tasks": ["2.4", "3.3", "5.4", "6.2", "6.4"] },
    { "id": 3, "tasks": ["6.5", "9.1"] },
    { "id": 4, "tasks": ["8.1", "8.5"] },
    { "id": 5, "tasks": ["8.2", "8.3"] },
    { "id": 6, "tasks": ["8.4"] }
  ]
}
```
