#ifndef HEALTH_RETRY_H
#define HEALTH_RETRY_H

#include <functional>

// Constants mirroring config.h
static const int HEALTH_MAX_RETRIES = 3;

/**
 * Result of the retry logic execution.
 */
struct RetryResult {
    bool success;       // True if at least one attempt succeeded
    int attempts;       // Total number of attempts made
};

/**
 * Testable retry logic that mirrors the retry behavior from health_client.cpp.
 *
 * The callback simulates attemptFetch():
 *   - Returns true if a response was received (stops retrying)
 *   - Returns false if connection failed (triggers retry)
 *
 * Retry behavior:
 *   - Makes 1 initial attempt + up to HEALTH_MAX_RETRIES retries (4 total max)
 *   - Stops immediately when callback returns true (got a response)
 *   - Only retries when callback returns false (connection failure)
 *
 * @param attemptCallback  Function called for each attempt. Returns true to
 *                         indicate success (stop), false to indicate failure (retry).
 * @return RetryResult with success status and total attempts made.
 */
inline RetryResult executeWithRetry(std::function<bool()> attemptCallback) {
    RetryResult result;
    result.success = false;
    result.attempts = 0;

    int totalAttempts = 1 + HEALTH_MAX_RETRIES; // = 4

    for (int attempt = 0; attempt < totalAttempts; attempt++) {
        result.attempts++;

        if (attemptCallback()) {
            // Got a response (success or parseable failure) - stop retrying
            result.success = true;
            return result;
        }
        // Connection failed - continue to next retry
    }

    // All attempts failed
    return result;
}

#endif
