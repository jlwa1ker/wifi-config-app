#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include <string>
#include <cstdio>

#include "response_parser.h"

/**
 * Feature: hygrometer-reporting, Property 7: Response parser computes correct removal count
 *
 * Validates: Requirements 5.3, 5.5
 *
 * For any valid server success response containing inserted_count and skipped_count
 * fields, parseIngestResponse() SHALL return a value equal to
 * inserted_count + skipped_count.
 */
TEST_CASE("Property 7: Response parser computes correct removal count", "[property][response_parser]") {
    rc::check("parseIngestResponse returns inserted_count + skipped_count for valid JSON",
        []() {
            // Generate random inserted_count and skipped_count in [0, 96]
            const auto insertedCount = *rc::gen::inRange(0, 97);
            const auto skippedCount = *rc::gen::inRange(0, 97);

            // Construct a valid JSON response string
            char jsonBuffer[256];
            snprintf(jsonBuffer, sizeof(jsonBuffer),
                     "{\"status\":\"success\",\"inserted_count\":%d,\"skipped_count\":%d}",
                     insertedCount, skippedCount);

            // Parse and verify
            int result = parseIngestResponse(jsonBuffer);
            RC_ASSERT(result == insertedCount + skippedCount);
        }
    );
}

TEST_CASE("Response parser handles truncated response", "[unit][response_parser]") {
    SECTION("truncated after skipped_count — large skipped array cut off") {
        // This simulates the real-world bug: server returns a huge response
        // with a "skipped" array that gets truncated in the 512-byte buffer
        const char* truncated =
            "{\"status\":\"success\",\"inserted_count\":1,\"skipped_count\":95,"
            "\"request_ids\":[\"abc123\"],\"skipped\":[{\"index\":0,\"reason\":\"exact_match\"},{\"index\":1,\"reas";

        int result = parseIngestResponse(truncated);
        REQUIRE(result == 96); // 1 + 95
    }

    SECTION("truncated but both fields present") {
        const char* truncated =
            "{\"status\":\"success\",\"inserted_count\":5,\"skipped_count\":10,\"request_ids\":[\"a\",\"b\",\"c";

        int result = parseIngestResponse(truncated);
        REQUIRE(result == 15); // 5 + 10
    }
}

TEST_CASE("Response parser returns -1 for invalid input", "[unit][response_parser]") {
    SECTION("malformed - no fields present") {
        int result = parseIngestResponse("{not valid json");
        REQUIRE(result == -1);
    }

    SECTION("empty string") {
        int result = parseIngestResponse("");
        REQUIRE(result == -1);
    }

    SECTION("null input") {
        int result = parseIngestResponse(nullptr);
        REQUIRE(result == -1);
    }
}

TEST_CASE("Response parser returns -1 for missing required fields", "[unit][response_parser]") {
    SECTION("missing inserted_count") {
        int result = parseIngestResponse("{\"skipped_count\":2}");
        REQUIRE(result == -1);
    }

    SECTION("missing skipped_count") {
        int result = parseIngestResponse("{\"inserted_count\":3}");
        REQUIRE(result == -1);
    }

    SECTION("both fields missing") {
        int result = parseIngestResponse("{\"status\":\"success\"}");
        REQUIRE(result == -1);
    }
}
