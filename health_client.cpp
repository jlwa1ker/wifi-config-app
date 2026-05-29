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
#include "health_client.h"
#include "config.h"
#include <WiFi101.h>
#include <ArduinoJson.h>
#include <string.h>

// Module-level cache for last successful health result
static char cachedStatus[32];
static char cachedVersion[32];
static bool cacheValid = false;

// Attempt a single HTTP GET request to the health endpoint.
// Returns true if the response was received and parsed into result.
// If resolvedIP is provided (non-zero), connect by IP instead of hostname.
static bool attemptFetch(HealthResult& result, IPAddress resolvedIP = IPAddress(0,0,0,0)) {
  WiFiClient client;

  bool connected;
  if (resolvedIP != IPAddress(0,0,0,0)) {
    connected = client.connect(resolvedIP, HEALTH_PORT);
  } else {
    connected = client.connect(HEALTH_HOST, HEALTH_PORT);
  }
  
  if (!connected) {
    return false;
  }

  // Send HTTP GET request (HTTP/1.0 for simpler response handling)
  client.print("GET ");
  client.print(HEALTH_PATH);
  client.print(" HTTP/1.0\r\nHost: ");
  client.print(HEALTH_HOST);
  client.print("\r\n\r\n");

  // Wait for response with timeout (10 seconds)
  unsigned long start = millis();
  while (!client.available()) {
    if (millis() - start > 10000) {
      client.stop();
      return false;
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

  // Skip HTTP headers - find the blank line separating headers from body
  char* body = strstr(response, "\r\n\r\n");
  if (body == NULL) {
    return false;
  }
  body += 4; // Skip past "\r\n\r\n"

  // Parse JSON body
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    // Invalid JSON - return failure with empty fields
    result.success = false;
    result.status[0] = '\0';
    result.version[0] = '\0';
    result.missingFields = false;
    return true; // We got a response, but JSON was invalid
  }

  // JSON is valid - extract fields
  result.success = true;
  result.missingFields = false;

  if (doc.containsKey("status")) {
    strncpy(result.status, doc["status"].as<const char*>(), sizeof(result.status) - 1);
    result.status[sizeof(result.status) - 1] = '\0';
  } else {
    result.status[0] = '\0';
    result.missingFields = true;
  }

  if (doc.containsKey("version")) {
    strncpy(result.version, doc["version"].as<const char*>(), sizeof(result.version) - 1);
    result.version[sizeof(result.version) - 1] = '\0';
  } else {
    result.version[0] = '\0';
    result.missingFields = true;
  }

  return true;
}

HealthResult healthClient_fetch() {
  HealthResult result;
  result.success = false;
  result.status[0] = '\0';
  result.version[0] = '\0';
  result.missingFields = false;

  // Resolve DNS once before attempting connections
  IPAddress resolvedIP;
  WiFi.hostByName(HEALTH_HOST, resolvedIP);

  // Try initial attempt + up to HEALTH_MAX_RETRIES retries
  int totalAttempts = 1 + HEALTH_MAX_RETRIES;

  for (int attempt = 0; attempt < totalAttempts; attempt++) {
    if (attempt > 0) {
      delay(HEALTH_RETRY_DELAY_MS);
    }

    if (attemptFetch(result, resolvedIP)) {
      // We got a response (may or may not have valid JSON)
      if (result.success) {
        // Valid JSON parsed - cache the result
        strncpy(cachedStatus, result.status, sizeof(cachedStatus) - 1);
        cachedStatus[sizeof(cachedStatus) - 1] = '\0';
        strncpy(cachedVersion, result.version, sizeof(cachedVersion) - 1);
        cachedVersion[sizeof(cachedVersion) - 1] = '\0';
        cacheValid = true;
      }
      return result;
    }
    // Connection failed or timed out - retry
  }

  // All attempts failed
  return result;
}

bool healthClient_getCached(char* status, char* version) {
  if (!cacheValid) {
    return false;
  }
  strncpy(status, cachedStatus, 32);
  strncpy(version, cachedVersion, 32);
  return true;
}
