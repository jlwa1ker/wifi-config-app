#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <cstring>
#include <string>

#include "reading_cache_logic.h"

/**
 * Unit test: Cache is fully cleared after successful transmission.
 *
 * Simulates the upload flow:
 * 1. Cache N readings (simulating accumulated data)
 * 2. Simulate a successful server response (removalCount = N)
 * 3. Call readingCache_removeOldest(removalCount)
 * 4. Verify cache count is 0 (screen would show "0" in status bar)
 *
 * This tests the contract that the main loop relies on:
 *   - serverReporter_send() returns REPORT_SUCCESS with removalCount = N
 *   - readingCache_removeOldest(N) is called
 *   - readingCache_count() then returns 0
 */
TEST_CASE("Successful transmission clears entire cache", "[unit][integration][cache]") {
    SECTION("single reading cached and transmitted") {
        ReadingCache cache;
        readingCache_init(cache);

        readingCache_add(cache, "2026-06-01T12:00:00+00:00", 72.5f, 45.0f);
        REQUIRE(readingCache_count(cache) == 1);

        // Simulate successful send: server accepted 1 reading
        int removalCount = 1;
        readingCache_removeOldest(cache, removalCount);

        REQUIRE(readingCache_count(cache) == 0);
    }

    SECTION("multiple readings cached and all transmitted") {
        ReadingCache cache;
        readingCache_init(cache);

        // Add 5 readings
        readingCache_add(cache, "2026-06-01T12:00:00+00:00", 72.5f, 45.0f);
        readingCache_add(cache, "2026-06-01T12:15:00+00:00", 73.0f, 44.0f);
        readingCache_add(cache, "2026-06-01T12:30:00+00:00", 73.5f, 43.0f);
        readingCache_add(cache, "2026-06-01T12:45:00+00:00", 74.0f, 42.0f);
        readingCache_add(cache, "2026-06-01T13:00:00+00:00", 74.5f, 41.0f);
        REQUIRE(readingCache_count(cache) == 5);

        // Simulate successful send: server accepted all 5
        int removalCount = 5;
        readingCache_removeOldest(cache, removalCount);

        REQUIRE(readingCache_count(cache) == 0);
    }

    SECTION("full cache (96 readings) transmitted successfully") {
        ReadingCache cache;
        readingCache_init(cache);

        // Fill the cache to capacity
        for (int i = 0; i < READING_CACHE_MAX_SIZE; i++) {
            char ts[26];
            snprintf(ts, sizeof(ts), "2026-06-01T%02d:%02d:00+00:00",
                     (i * 15) / 60, (i * 15) % 60);
            readingCache_add(cache, ts, 70.0f + i * 0.1f, 50.0f - i * 0.1f);
        }
        REQUIRE(readingCache_count(cache) == READING_CACHE_MAX_SIZE);

        // Simulate successful send: server accepted all 96
        int removalCount = READING_CACHE_MAX_SIZE;
        readingCache_removeOldest(cache, removalCount);

        // Cache should be completely empty — status bar would show "0"
        REQUIRE(readingCache_count(cache) == 0);
    }
}

TEST_CASE("Partial transmission leaves remaining readings in cache", "[unit][integration][cache]") {
    SECTION("server accepts fewer than cached") {
        ReadingCache cache;
        readingCache_init(cache);

        // Add 5 readings
        readingCache_add(cache, "2026-06-01T12:00:00+00:00", 72.5f, 45.0f);
        readingCache_add(cache, "2026-06-01T12:15:00+00:00", 73.0f, 44.0f);
        readingCache_add(cache, "2026-06-01T12:30:00+00:00", 73.5f, 43.0f);
        readingCache_add(cache, "2026-06-01T12:45:00+00:00", 74.0f, 42.0f);
        readingCache_add(cache, "2026-06-01T13:00:00+00:00", 74.5f, 41.0f);
        REQUIRE(readingCache_count(cache) == 5);

        // Simulate partial success: server only accepted 3 (inserted + skipped)
        int removalCount = 3;
        readingCache_removeOldest(cache, removalCount);

        // 2 readings should remain
        REQUIRE(readingCache_count(cache) == 2);

        // Remaining readings should be the newest ones (FIFO order preserved)
        int count, head;
        const CachedReading* readings = readingCache_getAll(cache, count, head);
        REQUIRE(count == 2);

        int idx0 = (head + 0) % READING_CACHE_MAX_SIZE;
        int idx1 = (head + 1) % READING_CACHE_MAX_SIZE;
        REQUIRE(std::string(readings[idx0].timestamp) == "2026-06-01T12:45:00+00:00");
        REQUIRE(std::string(readings[idx1].timestamp) == "2026-06-01T13:00:00+00:00");
    }
}

TEST_CASE("Failed transmission retains all readings in cache", "[unit][integration][cache]") {
    ReadingCache cache;
    readingCache_init(cache);

    // Add 3 readings
    readingCache_add(cache, "2026-06-01T12:00:00+00:00", 72.5f, 45.0f);
    readingCache_add(cache, "2026-06-01T12:15:00+00:00", 73.0f, 44.0f);
    readingCache_add(cache, "2026-06-01T12:30:00+00:00", 73.5f, 43.0f);
    REQUIRE(readingCache_count(cache) == 3);

    // Simulate failed send: no removal happens (removalCount not used)
    // In the main loop, on failure readingCache_removeOldest() is NOT called
    // So the cache remains unchanged

    REQUIRE(readingCache_count(cache) == 3);

    // Verify all readings are still there in order
    int count, head;
    const CachedReading* readings = readingCache_getAll(cache, count, head);
    REQUIRE(count == 3);

    int idx0 = (head + 0) % READING_CACHE_MAX_SIZE;
    int idx1 = (head + 1) % READING_CACHE_MAX_SIZE;
    int idx2 = (head + 2) % READING_CACHE_MAX_SIZE;
    REQUIRE(std::string(readings[idx0].timestamp) == "2026-06-01T12:00:00+00:00");
    REQUIRE(std::string(readings[idx1].timestamp) == "2026-06-01T12:15:00+00:00");
    REQUIRE(std::string(readings[idx2].timestamp) == "2026-06-01T12:30:00+00:00");
}
