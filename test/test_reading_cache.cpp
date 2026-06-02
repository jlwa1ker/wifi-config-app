#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <cstring>
#include <string>
#include <vector>

#include "reading_cache_logic.h"

// Generator for ISO 8601-like timestamp strings (26 chars max)
static rc::Gen<std::string> genTimestamp() {
    return rc::gen::apply(
        [](int year, int month, int day, int hour, int minute, int second) {
            char buf[26];
            snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d+00:00",
                     year, month, day, hour, minute, second);
            return std::string(buf);
        },
        rc::gen::inRange(2020, 2100),
        rc::gen::inRange(1, 13),
        rc::gen::inRange(1, 29),
        rc::gen::inRange(0, 24),
        rc::gen::inRange(0, 60),
        rc::gen::inRange(0, 60)
    );
}

// Generator for temperature in Fahrenheit range
static rc::Gen<float> genTemperature() {
    return rc::gen::map(rc::gen::inRange(-400, 1850), [](int x) {
        return static_cast<float>(x) / 10.0f;
    });
}

// Generator for humidity percentage
static rc::Gen<float> genHumidity() {
    return rc::gen::map(rc::gen::inRange(0, 1001), [](int x) {
        return static_cast<float>(x) / 10.0f;
    });
}

/**
 * Feature: hygrometer-reporting, Property 4: Reading cache capacity invariant
 *
 * Validates: Requirements 4.2
 *
 * For any sequence of readingCache_add() calls (regardless of length),
 * readingCache_count() SHALL never exceed READING_CACHE_MAX_SIZE (96).
 */
TEST_CASE("Property 4: Reading cache capacity invariant", "[property][reading_cache]") {
    rc::check("count never exceeds READING_CACHE_MAX_SIZE after any sequence of adds",
        []() {
            // Generate a random number of add operations (1 to 300)
            const auto numOps = *rc::gen::inRange(1, 301);

            ReadingCache cache;
            readingCache_init(cache);

            for (int i = 0; i < numOps; i++) {
                const auto timestamp = *genTimestamp();
                const auto temperature = *genTemperature();
                const auto humidity = *genHumidity();

                readingCache_add(cache, timestamp.c_str(), temperature, humidity);

                // After every add, count must never exceed the maximum
                RC_ASSERT(readingCache_count(cache) <= READING_CACHE_MAX_SIZE);
            }

            // Final count must also be within bounds
            RC_ASSERT(readingCache_count(cache) <= READING_CACHE_MAX_SIZE);
            // Count should equal min(numOps, 96)
            int expectedCount = numOps < READING_CACHE_MAX_SIZE ? numOps : READING_CACHE_MAX_SIZE;
            RC_ASSERT(readingCache_count(cache) == expectedCount);
        }
    );
}


/**
 * Feature: hygrometer-reporting, Property 5: Ring buffer preserves most recent readings in FIFO order
 *
 * Validates: Requirements 4.3, 4.4
 *
 * For any sequence of N readings added to the cache where N > 96, the cache
 * SHALL contain exactly the most recent 96 readings in insertion order, and
 * readingCache_count() SHALL only decrease when readingCache_removeOldest()
 * is explicitly called.
 */
TEST_CASE("Property 5: Ring buffer preserves most recent readings in FIFO order", "[property][reading_cache]") {
    rc::check("cache contains the most recent 96 readings in insertion order after N > 96 adds",
        []() {
            // Generate a random number N in range [97, 300]
            const auto N = *rc::gen::inRange(97, 301);

            ReadingCache cache;
            readingCache_init(cache);

            // Build N readings with unique sequential timestamps
            std::vector<std::string> timestamps;
            timestamps.reserve(N);
            for (int i = 0; i < N; i++) {
                // Generate timestamps like "2024-01-01T00:00:01+00:00", "2024-01-01T00:00:02+00:00", etc.
                char ts[26];
                int seconds = (i + 1) % 60;
                int minutes = ((i + 1) / 60) % 60;
                int hours = ((i + 1) / 3600) % 24;
                snprintf(ts, sizeof(ts), "2024-01-01T%02d:%02d:%02d+00:00", hours, minutes, seconds);
                timestamps.push_back(std::string(ts));
            }

            // Add all N readings to the cache
            for (int i = 0; i < N; i++) {
                float temp = 70.0f + static_cast<float>(i) * 0.1f;
                float hum = 40.0f + static_cast<float>(i) * 0.05f;
                readingCache_add(cache, timestamps[i].c_str(), temp, hum);
            }

            // Verify the cache count is exactly 96
            RC_ASSERT(readingCache_count(cache) == READING_CACHE_MAX_SIZE);

            // Verify the cache contains the last 96 readings in insertion order (FIFO)
            int count = 0;
            int head = 0;
            const CachedReading* readings = readingCache_getAll(cache, count, head);

            RC_ASSERT(count == READING_CACHE_MAX_SIZE);

            // The last 96 readings inserted are those at indices [N-96, N-1]
            int startIdx = N - READING_CACHE_MAX_SIZE;
            for (int i = 0; i < count; i++) {
                int bufIdx = (head + i) % READING_CACHE_MAX_SIZE;
                int originalIdx = startIdx + i;

                // Verify timestamp matches the expected reading
                RC_ASSERT(std::string(readings[bufIdx].timestamp) == timestamps[originalIdx]);

                // Verify temperature matches
                float expectedTemp = 70.0f + static_cast<float>(originalIdx) * 0.1f;
                RC_ASSERT(readings[bufIdx].temperature_f == expectedTemp);

                // Verify humidity matches
                float expectedHum = 40.0f + static_cast<float>(originalIdx) * 0.05f;
                RC_ASSERT(readings[bufIdx].humidity_pct == expectedHum);

                // Verify slot is occupied
                RC_ASSERT(readings[bufIdx].occupied == true);
            }
        }
    );
}
