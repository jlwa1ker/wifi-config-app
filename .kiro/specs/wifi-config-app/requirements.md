# Requirements Document

## Introduction

This document specifies the requirements for a WiFi configuration application running on an ESP8266 microcontroller. The device acts as a hygrometer that needs to connect to a user's WiFi network. On first boot (or when no credentials are stored), the device creates an unsecured access point named "tempmon" and serves a web-based configuration form. Once the user provides valid WiFi credentials, the device connects to the specified network, persists the credentials, and serves a landing page displaying health information retrieved from a remote server.

## Glossary

- **Device**: The ESP8266 microcontroller running the WiFi configuration application
- **Access_Point**: The unsecured WiFi network created by the Device with SSID "tempmon"
- **Config_Form**: The HTML web form served by the Device that collects WiFi network name and password
- **Credential_Store**: The EEPROM/flash memory area where WiFi credentials are persisted
- **Health_Endpoint**: The remote HTTP endpoint at http://tempmon2-alb-150754285.us-east-1.elb.amazonaws.com/health that returns JSON status information
- **Landing_Page**: The HTML page served by the Device after successful WiFi connection, displaying health information
- **Web_Server**: The HTTP server running on the Device that serves the Config_Form and Landing_Page

## Requirements

### Requirement 1: Access Point Creation

**User Story:** As a user, I want the device to create a WiFi network I can connect to, so that I can configure it without needing an existing network connection.

#### Acceptance Criteria

1. WHEN the Device boots and the Credential_Store contains no saved credentials, THE Device SHALL create an unsecured Access_Point with SSID "tempmon" without WPA2 or any other encryption
2. WHEN the Device boots and the Credential_Store contains saved credentials, THE Device SHALL skip Access_Point creation and attempt to connect to the stored network
3. WHILE the Access_Point is active, THE Device SHALL assign itself the IP address 192.168.4.1
4. WHILE the Access_Point is active, THE Web_Server SHALL listen for HTTP connections on port 80

### Requirement 2: WiFi Configuration Form

**User Story:** As a user, I want to see a web form where I can enter my WiFi credentials, so that I can connect the device to my home network.

#### Acceptance Criteria

1. WHEN a user navigates to http://192.168.4.1 while the Access_Point is active, THE Web_Server SHALL serve the Config_Form
2. IF the Web_Server fails to serve the Config_Form due to a technical error, THEN THE Web_Server SHALL return an HTTP 500 response with a plain-text error message
3. THE Config_Form SHALL contain a text input field for the WiFi network name (SSID)
4. THE Config_Form SHALL contain a password input field for the WiFi network password
5. THE Config_Form SHALL contain a submit button to send the credentials to the Device

### Requirement 3: WiFi Connection Attempt

**User Story:** As a user, I want the device to connect to my WiFi network using the credentials I provided, so that the device can access the internet.

#### Acceptance Criteria

1. WHEN the user submits the Config_Form with a network name and password, THE Device SHALL attempt to connect to the specified WiFi network using the provided credentials
2. THE Device SHALL wait up to 20 seconds for the WiFi connection attempt to succeed or fail
3. WHEN the WiFi connection attempt succeeds within 20 seconds, THE Device SHALL disable the Access_Point
4. IF the WiFi connection attempt exceeds 20 seconds, THEN THE Device SHALL treat the attempt as timed out and keep the Access_Point enabled, even if the connection eventually succeeds

### Requirement 4: Connection Failure Handling

**User Story:** As a user, I want to be informed if the device fails to connect to my network, so that I can correct the credentials and try again.

#### Acceptance Criteria

1. IF the WiFi connection attempt fails or times out, THEN THE Device SHALL remain in Access_Point mode
2. THE Device MAY enter Station mode during the connection attempt itself, but SHALL return to Access_Point mode upon failure
3. IF the WiFi connection attempt fails or times out, THEN THE Web_Server SHALL serve the Config_Form with an error message indicating the connection failed
4. IF the WiFi connection attempt fails or times out, THEN THE Device SHALL NOT store the failed credentials in the Credential_Store

### Requirement 5: Credential Persistence

**User Story:** As a user, I want the device to remember my WiFi credentials, so that I do not have to reconfigure it every time it restarts.

#### Acceptance Criteria

1. WHEN the WiFi connection attempt succeeds, THE Device SHALL store the network name and password in the Credential_Store
2. IF the Credential_Store write operation fails, THEN THE Device SHALL notify the user via the Web_Server that credential storage failed and disconnect from the WiFi network
3. THE Credential_Store SHALL retain the saved credentials across power cycles
4. WHEN the Device boots and the Credential_Store contains saved credentials, THE Device SHALL attempt to connect to the stored network using the saved credentials

### Requirement 6: Auto-Reconnection on Boot

**User Story:** As a user, I want the device to automatically connect to my WiFi network when it powers up, so that it is ready to use without manual intervention.

#### Acceptance Criteria

1. WHEN the Device boots with saved credentials and the WiFi connection succeeds, THE Device SHALL start the Web_Server on the assigned network IP address
2. IF the Device boots with saved credentials and the WiFi connection fails, THEN THE Device SHALL clear the Credential_Store and create the Access_Point for reconfiguration

### Requirement 7: Health Endpoint Call

**User Story:** As a user, I want the device to check the status of the remote server, so that I can see whether the monitoring system is operational.

#### Acceptance Criteria

1. WHEN the Device successfully connects to WiFi, THE Device SHALL send an HTTP GET request to http://tempmon2-alb-150754285.us-east-1.elb.amazonaws.com/health
2. IF the initial Health_Endpoint request fails due to network issues or server unavailability, THEN THE Device SHALL retry the request up to 3 times with a 5-second delay between attempts
3. WHEN a user navigates to the Device IP address while connected to WiFi, THE Device SHALL send an HTTP GET request to http://tempmon2-alb-150754285.us-east-1.elb.amazonaws.com/health
4. THE Device SHALL parse the JSON response from the Health_Endpoint, extracting the "status" and "version" fields
5. WHEN the Health_Endpoint returns a successful response, THE Device SHALL cache the parsed "status" and "version" values for display during subsequent failures

### Requirement 8: Landing Page Display

**User Story:** As a user, I want to see the health status of the remote server on a web page, so that I can verify the system is working.

#### Acceptance Criteria

1. WHEN a user navigates to the Device IP address while connected to WiFi, THE Web_Server SHALL always serve the Landing_Page regardless of health endpoint outcome
2. WHEN the Health_Endpoint returns a successful response, THE Landing_Page SHALL display the parsed "status" and "version" values
3. IF the Health_Endpoint request fails or returns an error and cached data is available, THEN THE Landing_Page SHALL display the cached "status" and "version" values alongside an error message indicating the latest health check failed
4. IF the Health_Endpoint request fails or returns an error and no cached data is available, THEN THE Landing_Page SHALL display an error message indicating the health check failed
5. THE Landing_Page SHALL display the server status and version in a human-readable HTML format

### Requirement 9: Health Response Parsing

**User Story:** As a developer, I want the device to correctly parse the health endpoint JSON response, so that the displayed information is accurate.

#### Acceptance Criteria

1. WHEN the Health_Endpoint returns a JSON body with the format {"status":"ok","version":"X.Y.Z"}, THE Device SHALL extract the "status" string value
2. WHEN the Health_Endpoint returns a JSON body with the format {"status":"ok","version":"X.Y.Z"}, THE Device SHALL extract the "version" string value
3. IF the Health_Endpoint returns a response that is not valid JSON, THEN THE Device SHALL strictly treat the response as a failed health check without attempting to extract partial data
4. IF the Health_Endpoint returns valid JSON that does not contain the expected "status" or "version" fields, THEN THE Device SHALL treat the response as a successful health check with missing fields displayed as unavailable
