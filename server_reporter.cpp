/*
 * Copyright (C) 2025 James L. Walker, Kiro (AI Assistant)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "server_reporter.h"
#include "reading_cache.h"
#include "credential_store.h"
#include "config.h"
#include <WiFi101.h>
#include <ArduinoJson.h>
#include <string.h>

// Module-level location buffer, populated from credential store
static char storedLocation[MAX_LOCATION_LENGTH + 1] = "";

// Retrieve the device location from stored credentials.
// Call after credentialStore_init() and a successful read.
const char* serverReporter_getLocation() {
  if (storedLocation[0] == '\0') {
    WiFiCredentials creds;
    if (credentialStore_read(creds)) {
      strncpy(storedLocation, creds.location, MAX_LOCATION_LENGTH);
      storedLocation[MAX_LOCATION_LENGTH] = '\0';
    }
  }
  return storedLocation;
}

// Static WiFiClient instance for HTTP POST
static WiFiClient client;

// JSON serialization buffer (sized for up to 96 readings)
// Each reading is ~100 bytes in JSON; 96 * 100 + overhead ≈ 10KB
// Use a smaller working buffer and serialize in chunks if needed
#define JSON_BUFFER_SIZE 4096

ReportResult serverReporter_send(int& removalCount) {
    removalCount = 0;

    // Ensure clean client state from any previous attempt
    client.stop();

    // Get all cached readings
    int count = 0;
    int head = 0;
    const CachedReading* readings = readingCache_getAll(count, head);

    if (count == 0) {
        // Nothing to send — treat as success with 0 removals
        return REPORT_SUCCESS;
    }

    // Serialize readings to JSON
    // Estimate document size: base overhead + per-reading size
    size_t docCapacity = 256 + ((size_t)count * 192);
    JsonDocument doc;
    JsonArray readingsArray = doc["readings"].to<JsonArray>();

    for (int i = 0; i < count; i++) {
        int idx = (head + i) % READING_CACHE_MAX_SIZE;
        const CachedReading& reading = readings[idx];

        JsonObject entry = readingsArray.add<JsonObject>();
        entry["timestamp"] = reading.timestamp;
        entry["temperature_f"] = reading.temperature_f;
        entry["humidity_pct"] = reading.humidity_pct;
        entry["location"] = serverReporter_getLocation();
    }

    // Measure serialized size
    size_t jsonLength = measureJson(doc);

    // Resolve DNS before connecting (WiFi101 hostname connect can be unreliable)
    IPAddress resolvedIP;
    if (WiFi.hostByName(INGEST_HOST, resolvedIP) != 1) {
        Serial.println("DNS resolution failed for ingest host.");
        return REPORT_CONNECT_FAILED;
    }

    // Connect to server by resolved IP
    if (!client.connect(resolvedIP, INGEST_PORT)) {
        Serial.println("TCP connect failed.");
        return REPORT_CONNECT_FAILED;
    }

    // Send HTTP POST request (HTTP/1.0 for simpler response handling)
    client.print("POST ");
    client.print(INGEST_PATH);
    client.print(" HTTP/1.0\r\nHost: ");
    client.print(INGEST_HOST);
    client.print("\r\nContent-Type: application/json\r\nContent-Length: ");
    client.print((int)jsonLength);
    client.print("\r\n\r\n");

    // Serialize JSON directly to the client
    serializeJson(doc, client);

    // Wait for response with timeout
    unsigned long start = millis();
    while (!client.available()) {
        if (millis() - start > INGEST_RESPONSE_TIMEOUT_MS) {
            client.stop();
            return REPORT_TIMEOUT;
        }
        if (!client.connected()) {
            client.stop();
            return REPORT_TIMEOUT;
        }
        delay(10);
    }

    // Read entire response into buffer
    char response[512];
    int len = 0;
    unsigned long readStart = millis();
    while (millis() - readStart < 5000 && len < (int)sizeof(response) - 1) {
        if (client.available()) {
            response[len++] = client.read();
        } else if (!client.connected()) {
            break;
        } else {
            delay(10);
        }
    }
    response[len] = '\0';
    client.stop();

    Serial.print("Response length: ");
    Serial.println(len);
    Serial.print("Raw response: ");
    Serial.println(response);

    // Skip HTTP headers — find the blank line separating headers from body
    char* body = strstr(response, "\r\n\r\n");
    if (body == NULL) {
        Serial.println("No header/body separator found!");
        return REPORT_PARSE_ERROR;
    }
    body += 4; // Skip past "\r\n\r\n"

    Serial.print("Response body: ");
    Serial.println(body);

    // Parse response JSON using the response parser logic
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, body);

    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        return REPORT_PARSE_ERROR;
    }

    if (!responseDoc["inserted_count"].is<int>() || !responseDoc["skipped_count"].is<int>()) {
        Serial.println("Missing inserted_count or skipped_count fields!");
        return REPORT_PARSE_ERROR;
    }

    int insertedCount = responseDoc["inserted_count"].as<int>();
    int skippedCount = responseDoc["skipped_count"].as<int>();

    removalCount = insertedCount + skippedCount;
    return REPORT_SUCCESS;
}
