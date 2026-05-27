#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <functional>

#include "health_retry.h"

/**
 * Feature: wifi-config-app, Property 8: Retry logic respects maximum attempts
 *
 * Validates: Requirements 7.2
 *
 * For any sequence of health endpoint failures, the health client shall make
 * at most 4 total requests (1 initial + 3 retries) and shall return success
 * if and only if at least one attempt succeeds within those bounds.
 */
TEST_CASE("Property 8: Retry logic respects maximum attempts", "[property][health_retry]") {

    SECTION("All attempts fail: exactly 4 attempts are made") {
        rc::check("when all attempts fail, exactly 4 total attempts are made",
            []() {
                // Generate an arbitrary number of "available" failures (doesn't matter,
                // the logic should always cap at 4)
                auto result = executeWithRetry([]() {
                    return false; // always fail
                });

                RC_ASSERT(result.attempts == 4);
                RC_ASSERT(result.success == false);
            }
        );
    }

    SECTION("Success on attempt K (1..4): exactly K attempts are made") {
        rc::check("when attempt K succeeds, exactly K attempts are made and result is success",
            []() {
                // Generate K in range [1, 4] - the attempt number that succeeds
                int successOnAttempt = *rc::gen::inRange(1, 5);
                int callCount = 0;

                auto result = executeWithRetry([&]() {
                    callCount++;
                    return (callCount == successOnAttempt);
                });

                RC_ASSERT(result.attempts == successOnAttempt);
                RC_ASSERT(result.success == true);
            }
        );
    }

    SECTION("Success if and only if at least one attempt succeeds") {
        rc::check("returns success=true iff at least one attempt within bounds succeeds",
            []() {
                // Generate a sequence of 4 booleans representing attempt outcomes
                auto outcomes = *rc::gen::container<std::vector<bool>>(4, rc::gen::arbitrary<bool>());

                int callIndex = 0;
                auto result = executeWithRetry([&]() {
                    bool outcome = outcomes[callIndex];
                    callIndex++;
                    return outcome;
                });

                // Determine expected behavior:
                // The retry stops at the first true outcome.
                // Find the first true in outcomes (if any)
                int firstSuccess = -1;
                for (int i = 0; i < 4; i++) {
                    if (outcomes[i]) {
                        firstSuccess = i;
                        break;
                    }
                }

                if (firstSuccess >= 0) {
                    // Should have succeeded on attempt (firstSuccess + 1)
                    RC_ASSERT(result.success == true);
                    RC_ASSERT(result.attempts == firstSuccess + 1);
                } else {
                    // All 4 attempts failed
                    RC_ASSERT(result.success == false);
                    RC_ASSERT(result.attempts == 4);
                }
            }
        );
    }

    SECTION("Never exceeds 4 total attempts regardless of failure count") {
        rc::check("total attempts never exceed 4 for any failure pattern",
            []() {
                // Generate a random number of failures before potential success
                // Even if we'd "need" more, it should cap at 4
                int failuresBeforeSuccess = *rc::gen::inRange(0, 100);
                int callCount = 0;

                auto result = executeWithRetry([&]() {
                    callCount++;
                    return (callCount > failuresBeforeSuccess);
                });

                RC_ASSERT(result.attempts <= 4);
                RC_ASSERT(result.attempts >= 1);

                if (failuresBeforeSuccess < 4) {
                    // Success happens within the retry window
                    RC_ASSERT(result.success == true);
                    RC_ASSERT(result.attempts == failuresBeforeSuccess + 1);
                } else {
                    // All 4 attempts fail (success would be on attempt > 4)
                    RC_ASSERT(result.success == false);
                    RC_ASSERT(result.attempts == 4);
                }
            }
        );
    }
}
