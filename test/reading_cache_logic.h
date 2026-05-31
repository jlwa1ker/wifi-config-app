#ifndef READING_CACHE_LOGIC_H
#define READING_CACHE_LOGIC_H

#include <cstring>

/**
 * Testable reading cache module that implements a ring buffer for storing
 * timestamped temperature/humidity readings awaiting server transmission.
 *
 * In reading_cache.cpp, the cache is implemented as module-level static state.
 * This header extracts the pure logic into a struct-based component for
 * host-based property testing with RapidCheck and Catch2.
 *
 * The ring buffer holds up to 96 readings (24 hours at 15-minute intervals).
 * When full, new readings overwrite the oldest entry.
 */

#define READING_CACHE_MAX_SIZE 96

struct CachedReading {
    char timestamp[26];    // ISO 8601 UTC string (e.g., "2024-01-15T10:30:00+00:00")
    float temperature_f;   // Fahrenheit, rounded to 1 decimal
    float humidity_pct;    // Relative humidity percentage
    bool occupied;         // Slot is in use
};

struct ReadingCache {
    CachedReading readings[READING_CACHE_MAX_SIZE];
    int head;   // Index of the oldest entry (next to be removed)
    int count;  // Number of valid entries currently in the buffer
};

/**
 * Initialize the reading cache to an empty state.
 */
inline void readingCache_init(ReadingCache& cache) {
    cache.head = 0;
    cache.count = 0;
    for (int i = 0; i < READING_CACHE_MAX_SIZE; i++) {
        cache.readings[i].occupied = false;
        cache.readings[i].timestamp[0] = '\0';
        cache.readings[i].temperature_f = 0.0f;
        cache.readings[i].humidity_pct = 0.0f;
    }
}

/**
 * Add a new reading to the cache. If the cache is full, the oldest
 * reading is overwritten (ring buffer behavior).
 */
inline void readingCache_add(ReadingCache& cache, const char* timestamp, float temperature_f, float humidity_pct) {
    int insertIdx = (cache.head + cache.count) % READING_CACHE_MAX_SIZE;

    if (cache.count == READING_CACHE_MAX_SIZE) {
        // Buffer is full — overwrite the oldest entry and advance head
        insertIdx = cache.head;
        cache.head = (cache.head + 1) % READING_CACHE_MAX_SIZE;
    } else {
        cache.count++;
    }

    strncpy(cache.readings[insertIdx].timestamp, timestamp, sizeof(cache.readings[insertIdx].timestamp) - 1);
    cache.readings[insertIdx].timestamp[sizeof(cache.readings[insertIdx].timestamp) - 1] = '\0';
    cache.readings[insertIdx].temperature_f = temperature_f;
    cache.readings[insertIdx].humidity_pct = humidity_pct;
    cache.readings[insertIdx].occupied = true;
}

/**
 * Get the number of readings currently in the cache.
 */
inline int readingCache_count(const ReadingCache& cache) {
    return cache.count;
}

/**
 * Remove the oldest N readings from the cache.
 * If n exceeds the current count, all readings are removed.
 */
inline void readingCache_removeOldest(ReadingCache& cache, int n) {
    if (n >= cache.count) {
        // Remove all — reset to empty state
        for (int i = 0; i < cache.count; i++) {
            int idx = (cache.head + i) % READING_CACHE_MAX_SIZE;
            cache.readings[idx].occupied = false;
        }
        cache.head = 0;
        cache.count = 0;
    } else {
        for (int i = 0; i < n; i++) {
            cache.readings[cache.head].occupied = false;
            cache.head = (cache.head + 1) % READING_CACHE_MAX_SIZE;
        }
        cache.count -= n;
    }
}

/**
 * Clear all readings from the cache.
 */
inline void readingCache_clear(ReadingCache& cache) {
    for (int i = 0; i < READING_CACHE_MAX_SIZE; i++) {
        cache.readings[i].occupied = false;
    }
    cache.head = 0;
    cache.count = 0;
}

/**
 * Get a pointer to the internal readings array and the current count.
 * To iterate in FIFO order (oldest to newest), use:
 *   for (int i = 0; i < count; i++) {
 *       int idx = (head + i) % READING_CACHE_MAX_SIZE;
 *       // access cache.readings[idx]
 *   }
 *
 * This function provides the head index and count for ordered iteration.
 */
inline const CachedReading* readingCache_getAll(const ReadingCache& cache, int& outCount, int& outHead) {
    outCount = cache.count;
    outHead = cache.head;
    return cache.readings;
}

#endif // READING_CACHE_LOGIC_H
