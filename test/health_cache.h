#ifndef HEALTH_CACHE_H
#define HEALTH_CACHE_H

#include <cstring>

/**
 * Testable cache module that mirrors the caching logic from health_client.cpp.
 *
 * In health_client.cpp, the cache is implemented as module-level static variables:
 *   static char cachedStatus[32];
 *   static char cachedVersion[32];
 *   static bool cacheValid = false;
 *
 * On successful parse: copy status and version to cache, set cacheValid=true.
 * getCached: if cacheValid, copy cached values out and return true; else return false.
 *
 * This header extracts that logic into a struct-based component for property testing.
 */

struct HealthCache {
    char cachedStatus[32];
    char cachedVersion[32];
    bool cacheValid;
};

// Replicate HealthResult struct for cache testing
struct HealthCacheResult {
    bool success;
    char status[32];
    char version[32];
    bool missingFields;
};

// Initialize cache to empty/invalid state
inline void healthCache_init(HealthCache& cache) {
    cache.cachedStatus[0] = '\0';
    cache.cachedVersion[0] = '\0';
    cache.cacheValid = false;
}

// Update cache on successful parse (mirrors health_client.cpp logic)
// Only updates cache when result.success is true
inline void healthCache_update(HealthCache& cache, const HealthCacheResult& result) {
    if (result.success) {
        strncpy(cache.cachedStatus, result.status, sizeof(cache.cachedStatus) - 1);
        cache.cachedStatus[sizeof(cache.cachedStatus) - 1] = '\0';
        strncpy(cache.cachedVersion, result.version, sizeof(cache.cachedVersion) - 1);
        cache.cachedVersion[sizeof(cache.cachedVersion) - 1] = '\0';
        cache.cacheValid = true;
    }
}

// Read cached values (mirrors healthClient_getCached logic)
// Returns false if no cached data is available
inline bool healthCache_getCached(const HealthCache& cache, char* status, char* version) {
    if (!cache.cacheValid) {
        return false;
    }
    strncpy(status, cache.cachedStatus, 32);
    strncpy(version, cache.cachedVersion, 32);
    return true;
}

// Check if cache contains valid data
inline bool healthCache_isValid(const HealthCache& cache) {
    return cache.cacheValid;
}

#endif
