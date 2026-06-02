#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include <ArduinoJson.h>
#include <string>

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
            StaticJsonDocument<256> doc;
            doc["status"] = "success";
            doc["inserted_count"] = insertedCount;
            doc["skipped_count"] = skippedCount;

            char jsonBuffer[256];
            serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));

            // Parse and verify
            int result = parseIngestResponse(jsonBuffer);
            RC_ASSERT(result == insertedCount + skippedCount);
        }
    );
}

TEST_CASE("Response parser returns -1 for invalid JSON", "[unit][response_parser]") {
    SECTION("malformed JSON") {
        int result = parseIngestResponse("{not valid json");
        REQUIRE(result == -1);
    }

    SECTION("empty string") {
        int result = parseIngestResponse("");
        REQUIRE(result == -1);
    }

    SECTION("null input") {
        int result = parseIngestResponse("null");
        REQUIRE(result == -1);
    }
}

TEST_CASE("Response parser returns -1 for JSON missing required fields", "[unit][response_parser]") {
    SECTION("missing inserted_count") {
        int result = parseIngestResponse("{\"skipped_count\": 2}");
        REQUIRE(result == -1);
    }

    SECTION("missing skipped_count") {
        int result = parseIngestResponse("{\"inserted_count\": 3}");
        REQUIRE(result == -1);
    }

    SECTION("both fields missing") {
        int result = parseIngestResponse("{\"status\": \"success\"}");
        REQUIRE(result == -1);
    }
}
