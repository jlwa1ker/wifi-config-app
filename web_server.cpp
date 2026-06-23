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
#include "web_server.h"
#include "config.h"
#include <string.h>

// Module-level state
static WiFiServer server(80);  // Static instance, port set at construction
static bool serverStarted = false;
static WebServerMode currentMode = MODE_CONFIG_FORM;
static char errorMessage[128] = "";

// Health data buffers for landing page
static char healthStatus[32] = "";
static char healthVersion[32] = "";
static bool healthIsCached = false;
static bool healthHasFailed = false;

// Landing page request-served flag
static bool landingPageServed = false;

// Credential submission state
static bool submissionReady = false;
static char submittedSSID[MAX_SSID_LENGTH + 1];
static char submittedPassword[MAX_PASS_LENGTH + 1];
static char submittedLocation[MAX_LOCATION_LENGTH + 1];

// Forward declarations for internal helpers
static void urlDecode(const char* src, char* dst, int dstSize);
static void parseFormBody(const char* body, char* ssid, int ssidSize, char* password, int passSize, char* location, int locationSize);
static void sendConfigFormResponse(WiFiClient& client);
static void sendLandingPageResponse(WiFiClient& client);
static void sendErrorResponse(WiFiClient& client, int statusCode, const char* message);

void webServer_init(int port) {
  // WiFi101's WiFiServer doesn't support changing port after construction,
  // but we constructed with port 80. Just call begin().
  server.begin();
  serverStarted = true;
}

void webServer_poll() {
  if (!serverStarted) {
    return;
  }

  WiFiClient client = server.available();
  if (!client || !client.connected()) {
    return;
  }

  // Wait briefly for data to arrive
  unsigned long start = millis();
  while (!client.available()) {
    if (!client.connected()) {
      return;
    }
    if (millis() - start > 5000) {
      client.stop();
      return;
    }
    delay(1);
  }

  // Read the request line (e.g., "GET / HTTP/1.1")
  char requestLine[128];
  int rlIdx = 0;
  while (client.available() && rlIdx < (int)sizeof(requestLine) - 1) {
    char c = client.read();
    if (c == '\n') {
      break;
    }
    if (c != '\r') {
      requestLine[rlIdx++] = c;
    }
  }
  requestLine[rlIdx] = '\0';

  // Parse method and path from request line
  char method[8] = "";
  char path[64] = "";
  char* spacePos = strchr(requestLine, ' ');
  if (spacePos != NULL) {
    int methodLen = spacePos - requestLine;
    if (methodLen > 0 && methodLen < (int)sizeof(method)) {
      strncpy(method, requestLine, methodLen);
      method[methodLen] = '\0';
    }
    char* pathStart = spacePos + 1;
    char* pathEnd = strchr(pathStart, ' ');
    if (pathEnd != NULL) {
      int pathLen = pathEnd - pathStart;
      if (pathLen > 0 && pathLen < (int)sizeof(path)) {
        strncpy(path, pathStart, pathLen);
        path[pathLen] = '\0';
      }
    }
  }

  // Read headers - look for Content-Length
  int contentLength = 0;
  char headerLine[256];
  while (client.available()) {
    int hlIdx = 0;
    while (client.available() && hlIdx < (int)sizeof(headerLine) - 1) {
      char c = client.read();
      if (c == '\n') {
        break;
      }
      if (c != '\r') {
        headerLine[hlIdx++] = c;
      }
    }
    headerLine[hlIdx] = '\0';

    // Empty line signals end of headers
    if (hlIdx == 0) {
      break;
    }

    // Check for Content-Length header (case-insensitive prefix match)
    if (strncasecmp(headerLine, "Content-Length:", 15) == 0) {
      contentLength = atoi(headerLine + 15);
    }
  }

  // Read POST body if present
  char body[256] = "";
  if (contentLength > 0 && contentLength < (int)sizeof(body)) {
    int bodyIdx = 0;
    unsigned long bodyStart = millis();
    while (bodyIdx < contentLength) {
      if (client.available()) {
        body[bodyIdx++] = client.read();
      } else if (millis() - bodyStart > 2000) {
        break;
      } else {
        delay(1);
      }
    }
    body[bodyIdx] = '\0';
  }

  // Route the request based on mode and method/path
  if (method[0] == '\0' || path[0] == '\0') {
    // Failed to parse request - technical failure
    sendErrorResponse(client, 500, "Internal server error: malformed request");
  } else if (currentMode == MODE_CONFIG_FORM) {
    if (strcmp(method, "POST") == 0 && strcmp(path, "/submit") == 0) {
      // Parse form submission
      parseFormBody(body, submittedSSID, sizeof(submittedSSID),
                    submittedPassword, sizeof(submittedPassword),
                    submittedLocation, sizeof(submittedLocation));
      submissionReady = true;

      // Send a brief acknowledgment response
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println("Cache-Control: no-cache, no-store, must-revalidate");
      client.println();
      client.println("<html><body><p>Connecting...</p></body></html>");
    } else if (strcmp(method, "GET") == 0 && strcmp(path, "/") == 0) {
      // Serve the config form
      sendConfigFormResponse(client);
    } else {
      // Unknown route in config mode - return 500
      sendErrorResponse(client, 500, "Internal server error: unrecognized request");
    }
  } else {
    // MODE_LANDING_PAGE - serve landing page
    sendLandingPageResponse(client);
    landingPageServed = true;
  }

  delay(1);
  client.stop();
}

void webServer_setMode(WebServerMode mode) {
  currentMode = mode;
}

void webServer_setError(const char* errorMsg) {
  if (errorMsg != NULL) {
    strncpy(errorMessage, errorMsg, sizeof(errorMessage) - 1);
    errorMessage[sizeof(errorMessage) - 1] = '\0';
  } else {
    errorMessage[0] = '\0';
  }
}

void webServer_setHealthData(const char* status, const char* version, bool isCached, bool hasFailed) {
  if (status != NULL) {
    strncpy(healthStatus, status, sizeof(healthStatus) - 1);
    healthStatus[sizeof(healthStatus) - 1] = '\0';
  } else {
    healthStatus[0] = '\0';
  }

  if (version != NULL) {
    strncpy(healthVersion, version, sizeof(healthVersion) - 1);
    healthVersion[sizeof(healthVersion) - 1] = '\0';
  } else {
    healthVersion[0] = '\0';
  }

  healthIsCached = isCached;
  healthHasFailed = hasFailed;
}

bool webServer_wasRequestServed() {
  if (landingPageServed) {
    landingPageServed = false;
    return true;
  }
  return false;
}

bool webServer_hasSubmission() {
  return submissionReady;
}

void webServer_getSubmission(char* ssid, char* password, char* location) {
  strncpy(ssid, submittedSSID, MAX_SSID_LENGTH + 1);
  strncpy(password, submittedPassword, MAX_PASS_LENGTH + 1);
  strncpy(location, submittedLocation, MAX_LOCATION_LENGTH + 1);
  submissionReady = false;
}

// --- Internal helper functions ---

static void urlDecode(const char* src, char* dst, int dstSize) {
  int di = 0;
  int si = 0;
  while (src[si] != '\0' && di < dstSize - 1) {
    if (src[si] == '%' && src[si + 1] != '\0' && src[si + 2] != '\0') {
      // Decode %XX hex sequence
      char hex[3] = { src[si + 1], src[si + 2], '\0' };
      char* endPtr;
      long val = strtol(hex, &endPtr, 16);
      if (endPtr == hex + 2) {
        dst[di++] = (char)val;
        si += 3;
      } else {
        // Malformed hex - pass '%' through as-is
        dst[di++] = src[si++];
      }
    } else if (src[si] == '+') {
      // '+' represents a space in form-urlencoded data
      dst[di++] = ' ';
      si++;
    } else {
      // Regular character pass-through
      dst[di++] = src[si++];
    }
  }
  dst[di] = '\0';
}

static void parseFormBody(const char* body, char* ssid, int ssidSize, char* password, int passSize, char* location, int locationSize) {
  // Initialize outputs
  ssid[0] = '\0';
  password[0] = '\0';
  location[0] = '\0';

  // Parse "ssid=value&password=value&location=value" format
  const char* ptr = body;
  while (*ptr != '\0') {
    // Find the key
    const char* eqPos = strchr(ptr, '=');
    if (eqPos == NULL) {
      break;
    }

    int keyLen = eqPos - ptr;
    const char* valStart = eqPos + 1;

    // Find end of value (& or end of string)
    const char* ampPos = strchr(valStart, '&');
    int valLen;
    if (ampPos != NULL) {
      valLen = ampPos - valStart;
    } else {
      valLen = strlen(valStart);
    }

    // Extract the raw (still encoded) value into a temp buffer
    char rawValue[128];
    if (valLen >= (int)sizeof(rawValue)) {
      valLen = sizeof(rawValue) - 1;
    }
    strncpy(rawValue, valStart, valLen);
    rawValue[valLen] = '\0';

    // Match key and URL-decode value into the appropriate output
    if (keyLen == 4 && strncmp(ptr, "ssid", 4) == 0) {
      urlDecode(rawValue, ssid, ssidSize);
    } else if (keyLen == 8 && strncmp(ptr, "password", 8) == 0) {
      urlDecode(rawValue, password, passSize);
    } else if (keyLen == 8 && strncmp(ptr, "location", 8) == 0) {
      urlDecode(rawValue, location, locationSize);
    }

    // Advance past this key=value pair
    if (ampPos != NULL) {
      ptr = ampPos + 1;
    } else {
      break;
    }
  }
}

static void sendConfigFormResponse(WiFiClient& client) {
  // Defensive check: if client is not connected, we cannot send a response
  if (!client.connected()) {
    return;
  }

  // Scan for nearby WiFi networks
  int numNetworks = WiFi.scanNetworks();

  // Sort by RSSI (strongest first) using selection sort, take top 10
  int sortedIndices[10];
  int sortedCount = (numNetworks < 10) ? numNetworks : 10;
  bool used[256];
  memset(used, false, (numNetworks < 256) ? numNetworks : 256);

  for (int i = 0; i < sortedCount; i++) {
    int bestIdx = -1;
    int32_t bestRSSI = -9999;
    for (int j = 0; j < numNetworks; j++) {
      if (!used[j] && WiFi.RSSI(j) > bestRSSI) {
        bestRSSI = WiFi.RSSI(j);
        bestIdx = j;
      }
    }
    if (bestIdx >= 0) {
      sortedIndices[i] = bestIdx;
      used[bestIdx] = true;
    }
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=UTF-8");
  client.println("Connection: close");
  client.println("Cache-Control: no-cache, no-store, must-revalidate");
  client.println();
  client.println("<!DOCTYPE html>");
  client.println("<html lang=\"en\">");
  client.println("<head>");
  client.println("<meta charset=\"UTF-8\">");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  client.println("<title>WiFi Configuration</title>");
  client.println("<style>");
  client.println("body { font-family: sans-serif; max-width: 400px; margin: 40px auto; padding: 0 20px; }");
  client.println("h1 { color: #333; }");
  client.println(".error { color: red; font-weight: bold; margin-bottom: 16px; }");
  client.println("label { display: block; margin-bottom: 12px; }");
  client.println("input[type=\"text\"], input[type=\"password\"] { width: 100%; padding: 8px; margin-top: 4px; box-sizing: border-box; }");
  client.println("button { padding: 10px 20px; background: #007bff; color: #fff; border: none; cursor: pointer; }");
  client.println("button:hover { background: #0056b3; }");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("<h1>WiFi Configuration</h1>");

  // Error message area
  if (errorMessage[0] != '\0') {
    client.print("<p class=\"error\" role=\"alert\">");
    client.print(errorMessage);
    client.println("</p>");
  }

  client.println("<form method=\"POST\" action=\"/submit\">");
  client.println("<label for=\"ssid\">Network Name (SSID)</label>");
  client.println("<input type=\"text\" id=\"ssid\" name=\"ssid\" list=\"ssid-list\" required>");

  // Generate datalist with scanned SSIDs
  client.println("<datalist id=\"ssid-list\">");
  for (int i = 0; i < sortedCount; i++) {
    client.print("<option value=\"");
    client.print(WiFi.SSID(sortedIndices[i]));
    client.println("\">");
  }
  client.println("</datalist>");

  client.println("<label for=\"password\">Password</label>");
  client.println("<input type=\"password\" id=\"password\" name=\"password\">");
  client.println("<label for=\"location\">Location</label>");
  client.println("<input type=\"text\" id=\"location\" name=\"location\" required>");
  client.println("<button type=\"submit\">Connect</button>");
  client.println("</form>");
  client.println("</body>");
  client.println("</html>");
}

static void sendErrorResponse(WiFiClient& client, int statusCode, const char* message) {
  if (!client.connected()) {
    return;
  }
  client.print("HTTP/1.1 ");
  client.print(statusCode);
  client.println(" Internal Server Error");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println("Cache-Control: no-cache, no-store, must-revalidate");
  client.println();
  client.println(message);
}

static void sendLandingPageResponse(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println("Cache-Control: no-cache, no-store, must-revalidate");
  client.println();
  client.println("<!DOCTYPE html>");
  client.println("<html lang=\"en\">");
  client.println("<head>");
  client.println("<meta charset=\"UTF-8\">");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  client.println("<title>TempMon Health</title>");
  client.println("<style>");
  client.println("body { font-family: sans-serif; margin: 2em; background: #f9f9f9; color: #222; }");
  client.println("h1 { color: #333; }");
  client.println(".status-table { border-collapse: collapse; margin-top: 1em; }");
  client.println(".status-table th, .status-table td { border: 1px solid #ccc; padding: 0.5em 1em; text-align: left; }");
  client.println(".status-table th { background: #eee; }");
  client.println(".error { color: #c00; background: #fee; border: 1px solid #c00; padding: 0.75em; margin-top: 1em; border-radius: 4px; }");
  client.println(".warning { color: #856404; background: #fff3cd; border: 1px solid #ffc107; padding: 0.75em; margin-top: 1em; border-radius: 4px; }");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("<h1>TempMon Health</h1>");

  if (healthHasFailed && healthStatus[0] == '\0') {
    // Health check failed and no cached data available
    client.println("<div class=\"error\" role=\"alert\">");
    client.println("<p>Health check failed. No cached data available.</p>");
    client.println("</div>");
  } else {
    if (healthHasFailed) {
      // Health check failed but cached data exists
      client.println("<div class=\"warning\" role=\"alert\">");
      client.println("<p>Latest health check failed. Showing cached data.</p>");
      client.println("</div>");
    }
    // Display health status and version
    client.println("<table class=\"status-table\" aria-label=\"Health status\">");
    client.println("<tr><th scope=\"row\">Status</th><td>");
    client.print(healthStatus);
    client.println("</td></tr>");
    client.println("<tr><th scope=\"row\">Version</th><td>");
    client.print(healthVersion);
    client.println("</td></tr>");
    client.println("</table>");
  }

  client.println("</body>");
  client.println("</html>");
}
