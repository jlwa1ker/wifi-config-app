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
- **Resync_State_Machine**: The non-blocking state machine inside the NTP_Client that schedules periodic re-synchronization and retries failed attempts without blocking the rest of the Device
- **Reporter**: The component that owns the Report_Interval cadence and the snapshot, cache and upload lifecycle, independent of NTP_Client scheduling
- **Time_Sync_Icon**: A small clock glyph in the OLED_Display status bar showing the outcome of the most recent NTP synchronization attempt
- **Boot_Sequence**: The period from power-up or reset until the Device reaches run mode, enters AP mode, or stops with an error
- **Configuration_Sequence**: The period from receipt of WiFi credentials from the web application until the Device enters run mode
- **Display_Timeout**: The fixed period, set by the constant `DISPLAY_ON_DURATION_MS`, for which the OLED_Display stays lit after a wake event
- **Wake_Button**: A normally-open pushbutton wired between GPIO pin 10 and ground, read with the internal pull-up enabled

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
6. THE Resync_State_Machine SHALL perform periodic re-synchronization without blocking sensor polling, reporting, or display updates
7. IF a re-synchronization attempt fails, THEN THE Resync_State_Machine SHALL wait 5 minutes and try again, repeating until an attempt succeeds, and SHALL NOT retry sooner than 5 minutes after a failure
8. WHEN a re-synchronization attempt succeeds, THE Resync_State_Machine SHALL return to the 24-hour re-synchronization interval
9. WHILE a re-synchronization is failing, THE NTP_Client SHALL continue to provide timestamps from the clock offset of the last successful synchronization
10. WHEN a single NTP request receives no reply within 2 seconds, THE Resync_State_Machine SHALL treat the attempt as failed

_Criteria 6 to 10 resolve issue #9: a failed 24-hour re-synchronization previously retried continuously and blocked reporting._

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
7. THE Reporter SHALL schedule reports independently of the Resync_State_Machine; a failing or in-progress NTP re-synchronization SHALL NOT delay or skip a report while a valid clock offset exists
8. WHEN the initial NTP synchronization succeeds, THE Reporter SHALL align the first report to the next wall-clock multiple of the Report_Interval since midnight UTC (or report immediately when already on a boundary)

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
4. WHILE the Device is in STA mode and the initial NTP synchronization has succeeded, THE OLED_Display status bar SHALL show the Time_Sync_Icon
5. WHILE the most recent NTP synchronization attempt succeeded, THE Time_Sync_Icon SHALL be a plain clock
6. WHILE the most recent NTP synchronization attempt failed, THE Time_Sync_Icon SHALL be a clock with a diagonal slash, and WHEN a later attempt succeeds THE Time_Sync_Icon SHALL return to a plain clock
7. THE OLED_Display SHALL NOT show the Time_Sync_Icon before the initial NTP synchronization succeeds or while the Device is in AP mode

### Requirement 8: OLED Power Management

**User Story:** As a device owner, I want the display dark whenever nobody is looking at it, so that the OLED does not wear out; the panel starts to fail after about 1000 hours lit and cannot be powered from a GPIO pin because it draws too much current.

#### Acceptance Criteria

1. THE Device SHALL turn the OLED_Display off by turning every pixel off, and SHALL NOT rely on switching the panel's power supply
2. THE Device SHALL keep the OLED_Display lit for the entire Boot_Sequence, with no timer running
3. WHEN the Device enters AP mode, or the Boot_Sequence stops because of an error, THE Device SHALL start the Display_Timeout countdown
4. WHEN the Device reaches run mode, THE Device SHALL start the Display_Timeout countdown
5. WHEN the Device receives WiFi credentials from the web application, THE Device SHALL turn the OLED_Display on and keep it lit, with no timer running, for the Configuration_Sequence until the Device enters run mode
6. THE Display_Timeout SHALL be defined by the constant `DISPLAY_ON_DURATION_MS` in `config.h`, with an initial value of 15 seconds, so that it can be adjusted at build time
7. WHEN the Display_Timeout countdown expires, THE Device SHALL turn the OLED_Display off
8. WHEN the Wake_Button is pressed, THE Device SHALL turn the OLED_Display on if it is off and start the Display_Timeout countdown, or restart the countdown from its start time if it is already running
9. WHILE the OLED_Display is held lit for the Boot_Sequence or Configuration_Sequence, THE Device SHALL ignore Wake_Button presses so that a press cannot end the hold early
10. THE Device SHALL debounce the Wake_Button in software, registering one press for each press-and-release regardless of contact bounce, and ignoring a low level shorter than 30 milliseconds
11. WHILE the OLED_Display is off, sensor updates and upload indicators SHALL NOT turn it on
