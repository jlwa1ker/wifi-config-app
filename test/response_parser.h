#ifndef RESPONSE_PARSER_H
#define RESPONSE_PARSER_H

#include <cstring>
#include <cstdlib>

/**
 * Extracted response parsing logic for server ingest responses.
 * Uses string search to extract inserted_count and skipped_count,
 * which allows parsing even when the response JSON is truncated
 * (e.g., large "skipped" array exceeds the read buffer).
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
 * Uses string search rather than full JSON parsing to handle truncated responses.
 *
 * @param responseBody  The response body string (may be truncated)
 * @return The number of readings to remove from cache
 *         (inserted_count + skipped_count), or -1 on parse failure
 */
static inline int parseIngestResponse(const char* responseBody) {
    if (responseBody == nullptr) {
        return -1;
    }

    int insertedCount = -1;
    int skippedCount = -1;

    const char* insertedKey = strstr(responseBody, "\"inserted_count\":");
    if (insertedKey) {
        insertedCount = atoi(insertedKey + 17);
    }

    const char* skippedKey = strstr(responseBody, "\"skipped_count\":");
    if (skippedKey) {
        skippedCount = atoi(skippedKey + 16);
    }

    if (insertedCount < 0 || skippedCount < 0) {
        return -1;
    }

    return insertedCount + skippedCount;
}

#endif // RESPONSE_PARSER_H
