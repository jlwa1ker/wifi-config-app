#ifndef JSON_SERIALIZER_H
#define JSON_SERIALIZER_H

#include <ArduinoJson.h>
#include "reading_cache_logic.h"

/**
 * Extracted pure JSON serialization logic for reading-to-JSON conversion.
 * This function serializes an array of CachedReading structs into the JSON
 * format expected by the tempmon2 server's /ingest endpoint, making it
 * testable on the host without Arduino dependencies.
 *
 * Expected output format:
 * {
 *   "readings": [
 *     {
 *       "timestamp": "2024-01-15T10:30:00+00:00",
 *       "temperature_f": 72.5,
 *       "humidity_pct": 45.2,
 *       "location": "office"
 *     }
 *   ]
 * }
 */

/**
 * Serialize all readings in a ReadingCache into JSON format for the tempmon2 ingest endpoint.
 * Iterates the ring buffer in FIFO order (oldest to newest) using head and count.
 *
 * @param cache     The ReadingCache containing readings to serialize
 * @param location  The location string to include in each reading entry
 * @param buffer    Output buffer to write the JSON string into
 * @param bufferSize Size of the output buffer in bytes
 * @return Number of bytes written to the buffer (excluding null terminator), or 0 on failure
 */
inline size_t serializeReadingsToJson(const ReadingCache& cache, const char* location,
                                      char* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) {
        return 0;
    }

    if (cache.count == 0) {
        // Empty cache — produce a valid JSON with empty readings array
        DynamicJsonDocument doc(64);
        doc.createNestedArray("readings");
        size_t written = serializeJson(doc, buffer, bufferSize);
        if (written == 0 || written >= bufferSize) {
            buffer[0] = '\0';
            return 0;
        }
        return written;
    }

    // Estimate document size: base overhead + per-reading size
    // Each reading needs memory for: object structure (16 bytes per member × 4 members = 64),
    // array element (16 bytes), string copies for timestamp (~26 bytes), plus overhead.
    // Use 192 bytes per reading to be safe.
    size_t docCapacity = 256 + (cache.count * 192);
    DynamicJsonDocument doc(docCapacity);

    JsonArray readings = doc.createNestedArray("readings");

    // Iterate in FIFO order (oldest to newest)
    for (int i = 0; i < cache.count; i++) {
        int idx = (cache.head + i) % READING_CACHE_MAX_SIZE;
        const CachedReading& reading = cache.readings[idx];

        JsonObject entry = readings.createNestedObject();
        entry["timestamp"] = reading.timestamp;
        entry["temperature_f"] = reading.temperature_f;
        entry["humidity_pct"] = reading.humidity_pct;
        entry["location"] = location;
    }

    size_t written = serializeJson(doc, buffer, bufferSize);
    if (written == 0 || written >= bufferSize) {
        buffer[0] = '\0';
        return 0;
    }

    return written;
}

#endif // JSON_SERIALIZER_H
