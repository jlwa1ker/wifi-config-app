# Requirements Document

## Introduction

This document specifies the requirements for periodic hygrometer (temperature and relative humidity) reporting from the Adafruit Feather M0 device to the tempmon2 server. The device reads the AHT20 sensor, maintains a running average, caches timestamped readings, and transmits them in batches to the server's ingest endpoint. An NTP-synchronized clock provides UTC timestamps. A bounded cache ensures data is preserved across transient network failures without exhausting the device's limited RAM.

## Glossary

- **Device**: The Adafruit Feather M0 (SAMD21) microcontroller with ATWINC1500 WiFi module running the application
- **AHT20_Sensor**: The AHT20 temperature and humidity sensor connected via I2C at address 0x38, accessed through the Adafruit_AHTX0 library
- **Ingest_Endpoint**: The HTTP POST endpoint at http://tempmon.walkerweb.us/ingest that accepts batched hygrometer readings in JSON format
- **Reading_Cache**: A bounded in-memory buffer that stores timestamped temperature and humidity readings awaiting successful transmission to the server
- **Running_Average**: A rolling arithmetic mean of sensor samples collected over the most recent 60 seconds, used to smooth sensor noise before recording a reading
- **NTP_Client**: The component that synchronizes the device's internal clock with a Network Time Protocol server to obtain accurate UTC timestamps
- **Report_Interval**: The 15-minute period between successive attempts to transmit cached readings to the Ingest_Endpoint
- **OLED_Display**: The SSD1306 OLED display (I2C address 0x3C) used to show device status

## Requirements

### Requirement 1: Sensor Polling

**User Story:** As a system operator, I want the device to read the AHT20 sensor at a consistent rate, so that the running average is based on evenly-spaced samples.

#### Acceptance Criteria

1. WHILE the Device is in STA mode, THE Device SHALL read temperature and relative humidity from the AHT20_Sensor once per second
2. IF the AHT20_Sensor fails to return a valid reading, THEN THE Device SHALL discard that sample and continue polling at the next one-second interval
3. THE Device SHALL NOT poll the AHT20_Sensor more frequently than once per second

### Requirement 2: Running Average Calculation

**User Story:** As a system operator, I want sensor noise smoothed out, so that reported values represent stable conditions rather than instantaneous fluctuations.

#### Acceptance Criteria

1. THE Device SHALL maintain a Running_Average of temperature samples collected within the most recent 60 seconds
2. THE Device SHALL maintain a Running_Average of relative humidity samples collected within the most recent 60 seconds
3. WHEN a new sensor sample is collected, THE Device SHALL add the sample to the Running_Average and discard any samples older than 60 seconds
4. WHEN the Report_Interval elapses, THE Device SHALL use the current Running_Average values as the temperature and humidity for the new cached reading
5. IF fewer than 10 samples exist in the Running_Average window at report time, THEN THE Device SHALL skip recording a reading for that interval

### Requirement 3: NTP Time Synchronization

**User Story:** As a system operator, I want the device to have an accurate UTC clock, so that readings are timestamped correctly for the server.

#### Acceptance Criteria

1. WHEN the Device first enters STA mode, THE NTP_Client SHALL synchronize the internal clock with an NTP server before recording any readings
2. THE NTP_Client SHALL re-synchronize the internal clock at least once every 24 hours to prevent clock drift
3. IF the initial NTP synchronization fails, THEN THE Device SHALL retry synchronization up to 5 times with a 10-second delay between attempts
4. IF all NTP synchronization retries fail, THEN THE Device SHALL display an error on the OLED_Display and refrain from recording readings until synchronization succeeds
5. THE NTP_Client SHALL produce timestamps in UTC conforming to ISO 8601 format with timezone offset (e.g., "2024-01-15T10:30:00+00:00")

### Requirement 4: Reading Cache

**User Story:** As a system operator, I want readings preserved when the server is unreachable, so that data is not lost during transient network outages.

#### Acceptance Criteria

1. WHEN the Report_Interval elapses and the Running_Average contains sufficient samples, THE Device SHALL store a new reading in the Reading_Cache with the current UTC timestamp, the Running_Average temperature in degrees Fahrenheit, and the Running_Average relative humidity as a percentage
2. THE Reading_Cache SHALL hold a maximum of 96 readings (equivalent to 24 hours of data at 15-minute intervals)
3. IF the Reading_Cache is full when a new reading is recorded, THEN THE Device SHALL discard the oldest reading in the cache to make room for the new reading
4. THE Reading_Cache SHALL retain all unsent readings across multiple Report_Intervals until they are successfully transmitted to the Ingest_Endpoint

### Requirement 5: Server Reporting

**User Story:** As a system operator, I want the device to transmit all pending readings to the server, so that the monitoring dashboard stays current.

#### Acceptance Criteria

1. WHEN the Report_Interval elapses, THE Device SHALL send an HTTP POST request to the Ingest_Endpoint containing all readings currently in the Reading_Cache
2. THE Device SHALL format the request body as a JSON object with a "readings" array, where each element contains "timestamp" (ISO 8601 UTC string), "temperature_f" (numeric, degrees Fahrenheit), "humidity_pct" (numeric, percentage), and "location" (string identifier for this device)
3. WHEN the Ingest_Endpoint returns an HTTP 200 or 201 response with a JSON body containing "status" equal to "success", THE Device SHALL remove the successfully transmitted readings from the Reading_Cache
4. IF the Ingest_Endpoint returns an HTTP error response or the connection fails, THEN THE Device SHALL retain all readings in the Reading_Cache for the next Report_Interval attempt
5. IF the Ingest_Endpoint returns a response indicating some readings were skipped as duplicates, THEN THE Device SHALL remove those skipped readings from the Reading_Cache along with the inserted readings
6. THE Device SHALL NOT start a WiFiServer in STA mode; the web server only runs in AP mode for WiFi credential configuration, so no WiFiServer/WiFiClient conflict exists during data transmission

### Requirement 6: Temperature Unit Conversion

**User Story:** As a system operator, I want temperature reported in Fahrenheit, so that it matches the server's expected format.

#### Acceptance Criteria

1. THE Device SHALL convert the AHT20_Sensor temperature reading from Celsius to Fahrenheit using the formula: temperature_f = (temperature_c × 9 / 5) + 32
2. THE Device SHALL round the converted Fahrenheit temperature to one decimal place before storing it in the Reading_Cache

### Requirement 7: OLED Status Display

**User Story:** As a user physically near the device, I want to see the current sensor readings and reporting status on the display, so that I can verify the device is operating correctly.

#### Acceptance Criteria

1. WHILE the Device is in STA mode, THE OLED_Display SHALL show the current Running_Average temperature and relative humidity values
2. WHEN the Device successfully transmits readings to the Ingest_Endpoint, THE OLED_Display SHALL briefly indicate a successful upload
3. IF the Device fails to transmit readings to the Ingest_Endpoint, THEN THE OLED_Display SHALL indicate the failure and show the number of cached readings pending transmission
