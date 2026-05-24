#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <cstring>
#include <string>
#include <vector>

#include "health_cache.h"

// Generator for printable ASCII strings within a given length range
static rc::Gen<std::string> genPrintableAscii(int minLen, int maxLen) {
    return rc::gen::mapcat(
        rc::gen::inRange(minLen, maxLen + 1),
        [](int len) {
            return rc::gen::container<std::string>(
                len,
                rc::gen::inRange<char>(32, 127)  // printable ASCII
            );
        }
    );
}

// Generator for a single HealthCacheResult (success or failure)
static rc::Gen<HealthCacheResult> genHealthResult() {
    return rc::gen::exec([]() {
        HealthCacheResult r;
        r.success = *rc::gen::arbitrary<bool>();
        r.missingFields = *rc::gen::arbitrary<bool>();
        auto status = *genPrintableAscii(0, 31);
        auto version = *genPrintableAscii(0, 31);
        strncpy(r.status, status.c_str(), sizeof(r.status) - 1);
        r.status[sizeof(r.status) - 1] = '\0';
        strncpy(r.version, version.c_str(), sizeof(r.version) - 1);
        r.version[sizeof(r.version) - 1] = '\0';
        return r;
    });
}

// Generator for a non-empty sequence of HealthCacheResults with at least one success
static rc::Gen<std::vector<HealthCacheResult>> genSequenceWithAtLeastOneSuccess() {
    return rc::gen::suchThat(
        rc::gen::mapcat(
            rc::gen::inRange(1, 20),  // sequence length 1-19
            [](int len) {
                return rc::gen::container<std::vector<HealthCacheResult>>(len, genHealthResult());
            }
        ),
        [](const std::vector<HealthCacheResult>& seq) {
            // Ensure at least one success in the sequence
            for (const auto& r : seq) {
                if (r.success) return true;
            }
            return false;
        }
    );
}

// Generator for a sequence of HealthCacheResults with NO successes
static rc::Gen<std::vector<HealthCacheResult>> genSequenceWithNoSuccess() {
    return rc::gen::map(
        rc::gen::mapcat(
            rc::gen::inRange(1, 20),  // sequence length 1-19
            [](int len) {
                return rc::gen::container<std::vector<HealthCacheResult>>(len, genHealthResult());
            }
        ),
        [](std::vector<HealthCacheResult> seq) {
            // Force all results to be failures
            for (auto& r : seq) {
                r.success = false;
            }
            return seq;
        }
    );
}

/**
 * Feature: wifi-config-app, Property 6: Health cache reflects last successful parse
 *
 * Validates: Requirements 7.5
 *
 * For any sequence of health fetch results where at least one succeeds,
 * the cached values shall always equal the status and version from the
 * most recent successful result, regardless of subsequent failures.
 */
TEST_CASE("Property 6: Health cache reflects last successful parse", "[property][health_cache]") {

    SECTION("After sequence with at least one success, cache equals last success") {
        rc::check("cached values equal the most recent successful result",
            []() {
                auto sequence = *genSequenceWithAtLeastOneSuccess();

                // Initialize cache
                HealthCache cache;
                healthCache_init(cache);

                // Apply all results in order
                for (const auto& result : sequence) {
                    healthCache_update(cache, result);
                }

                // Find the last successful result in the sequence
                const HealthCacheResult* lastSuccess = nullptr;
                for (const auto& result : sequence) {
                    if (result.success) {
                        lastSuccess = &result;
                    }
                }

                // Cache must be valid
                RC_ASSERT(healthCache_isValid(cache));

                // Read cached values
                char readStatus[32];
                char readVersion[32];
                bool getCachedResult = healthCache_getCached(cache, readStatus, readVersion);

                // getCached must succeed
                RC_ASSERT(getCachedResult == true);

                // Cached values must equal the last successful result
                RC_ASSERT(std::string(readStatus) == std::string(lastSuccess->status));
                RC_ASSERT(std::string(readVersion) == std::string(lastSuccess->version));
            }
        );
    }

    SECTION("After sequence with no successes, cache remains invalid") {
        rc::check("cache stays in initial state when no successes occur",
            []() {
                auto sequence = *genSequenceWithNoSuccess();

                // Initialize cache
                HealthCache cache;
                healthCache_init(cache);

                // Apply all results in order
                for (const auto& result : sequence) {
                    healthCache_update(cache, result);
                }

                // Cache must remain invalid
                RC_ASSERT(healthCache_isValid(cache) == false);

                // getCached must return false
                char readStatus[32];
                char readVersion[32];
                bool getCachedResult = healthCache_getCached(cache, readStatus, readVersion);
                RC_ASSERT(getCachedResult == false);
            }
        );
    }
}
