#ifndef RESPONSE_PARSER_H
#define RESPONSE_PARSER_H

#include <ArduinoJson.h>

/**
 * Extracted pure JSON parsing logic for server ingest responses.
 * This function replicates the response parsing behavior without any
 * network or Arduino dependencies, making it testable on the host.
 *
 * The tempmon2 server responds to POST /ingest with a JSON body like:
 * {
 *   "status": "success",
 *   "inserted_count": 3,
 *   "skipped_count": 1,
 *   ...
 * }
 *
 * The removal count is inserted_count + skipped_count, representing
 * readings that were either successfully stored or recognized as
 * duplicates — both cases mean the device can safely discard them
 * from its local cache.
 *
 * Requirements: 5.3, 5.5
 */

/**
 * Parse a server ingest response body and compute the removal count.
 *
 * @param responseBody  The JSON string from the server response
 * @return The number of readings to remove from cache
 *         (inserted_count + skipped_count), or -1 on parse failure
 */
static inline int parseIngestResponse(const char* responseBody) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, responseBody);

    if (error) {
        return -1;
    }

    if (!doc.containsKey("inserted_count") || !doc.containsKey("skipped_count")) {
        return -1;
    }

    int insertedCount = doc["inserted_count"].as<int>();
    int skippedCount = doc["skipped_count"].as<int>();

    return insertedCount + skippedCount;
}

#endif // RESPONSE_PARSER_H
