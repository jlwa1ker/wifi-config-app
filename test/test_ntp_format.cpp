#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include <cstring>
#include <string>
#include <cstdint>

#include "ntp_format.h"

// Epoch range: 2020-01-01T00:00:00Z to 2099-12-31T23:59:59Z
static constexpr uint32_t EPOCH_MIN = 1577836800U;  // 2020-01-01 00:00:00 UTC
static constexpr uint32_t EPOCH_MAX = 4102444799U;  // 2099-12-31 23:59:59 UTC

/**
 * Parse an ISO 8601 string of the form "YYYY-MM-DDTHH:MM:SS+00:00" back to a Unix epoch.
 * Returns the epoch value, or 0 on parse failure.
 */
static uint32_t parseISO8601ToEpoch(const char* str) {
    int year, month, day, hour, minute, second;
    if (sscanf(str, "%4d-%2d-%2dT%2d:%2d:%2d+00:00",
               &year, &month, &day, &hour, &minute, &second) != 6) {
        return 0;
    }

    // Convert date to days since epoch (1970-01-01)
    uint32_t days = 0;

    // Add days for complete years from 1970 to year-1
    for (int y = 1970; y < year; y++) {
        bool leap = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
        days += leap ? 366 : 365;
    }

    // Add days for complete months in the target year
    int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    if (leap) {
        daysInMonth[1] = 29;
    }
    for (int m = 0; m < month - 1; m++) {
        days += daysInMonth[m];
    }

    // Add remaining days (day is 1-indexed)
    days += (day - 1);

    // Convert to seconds
    uint32_t epoch = days * 86400U + hour * 3600U + minute * 60U + second;
    return epoch;
}

/**
 * Feature: hygrometer-reporting, Property 3: ISO 8601 timestamp format round-trip
 *
 * Validates: Requirements 3.5
 *
 * For any valid UTC epoch value (within the range 2020-01-01 to 2099-12-31),
 * formatting the epoch as an ISO 8601 string and parsing it back SHALL produce
 * the same epoch value, and the formatted string SHALL match the pattern
 * YYYY-MM-DDTHH:MM:SS+00:00.
 */
TEST_CASE("Property 3: ISO 8601 timestamp format round-trip", "[property][ntp_format]") {
    rc::check("formatted epoch matches pattern and round-trips to same value",
        []() {
            // Generate a random epoch in [2020-01-01, 2099-12-31]
            const auto epoch = *rc::gen::inRange<uint32_t>(EPOCH_MIN, EPOCH_MAX + 1);

            // Format the epoch
            char buffer[26];
            formatEpochISO8601(epoch, buffer, sizeof(buffer));

            std::string formatted(buffer);

            // Verify length is exactly 25 characters
            RC_ASSERT(formatted.length() == 25);

            // Verify structural pattern: YYYY-MM-DDTHH:MM:SS+00:00
            // Position checks for separators
            RC_ASSERT(formatted[4] == '-');   // Year-Month separator
            RC_ASSERT(formatted[7] == '-');   // Month-Day separator
            RC_ASSERT(formatted[10] == 'T');  // Date-Time separator
            RC_ASSERT(formatted[13] == ':');  // Hour-Minute separator
            RC_ASSERT(formatted[16] == ':');  // Minute-Second separator
            RC_ASSERT(formatted[19] == '+');  // Timezone offset start
            RC_ASSERT(formatted[22] == ':');  // Timezone offset separator

            // Verify timezone suffix is +00:00
            RC_ASSERT(formatted.substr(19) == "+00:00");

            // Verify all digit positions contain digits
            for (int i : {0, 1, 2, 3}) {
                RC_ASSERT(formatted[i] >= '0' && formatted[i] <= '9');
            }
            for (int i : {5, 6}) {
                RC_ASSERT(formatted[i] >= '0' && formatted[i] <= '9');
            }
            for (int i : {8, 9}) {
                RC_ASSERT(formatted[i] >= '0' && formatted[i] <= '9');
            }
            for (int i : {11, 12}) {
                RC_ASSERT(formatted[i] >= '0' && formatted[i] <= '9');
            }
            for (int i : {14, 15}) {
                RC_ASSERT(formatted[i] >= '0' && formatted[i] <= '9');
            }
            for (int i : {17, 18}) {
                RC_ASSERT(formatted[i] >= '0' && formatted[i] <= '9');
            }

            // Round-trip: parse the formatted string back to epoch
            uint32_t roundTripped = parseISO8601ToEpoch(buffer);
            RC_ASSERT(roundTripped == epoch);
        }
    );
}
