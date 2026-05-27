#ifndef HEALTH_PARSER_H
#define HEALTH_PARSER_H

#include <ArduinoJson.h>
#include <cstring>

/**
 * Extracted pure JSON parsing logic from health_client.cpp.
 * This function replicates the JSON parsing behavior without any
 * network or Arduino dependencies, making it testable on the host.
 */

struct HealthResult {
    bool success;           // Whether the JSON parsed correctly
    char status[32];        // Parsed "status" field
    char version[32];       // Parsed "version" field
    bool missingFields;     // True if JSON was valid but fields were missing
};

/**
 * Parse a JSON health response body into a HealthResult struct.
 * Mirrors the parsing logic in health_client.cpp's attemptFetch().
 *
 * @param json  The JSON string to parse (the HTTP response body)
 * @return HealthResult with parsed data or failure indication
 */
inline HealthResult parseHealthResponse(const char* json) {
    HealthResult result;
    result.success = false;
    result.status[0] = '\0';
    result.version[0] = '\0';
    result.missingFields = false;

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        // Invalid JSON - return failure with empty fields
        return result;
    }

    // JSON is valid - extract fields
    result.success = true;

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

    return result;
}

#endif
