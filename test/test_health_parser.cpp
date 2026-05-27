#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <cstring>
#include <string>

#include "health_parser.h"

// Generator for printable ASCII strings within a given length range (no quotes or backslashes to keep JSON valid)
static rc::Gen<std::string> genJsonSafeString(int minLen, int maxLen) {
    return rc::gen::mapcat(
        rc::gen::inRange(minLen, maxLen + 1),
        [](int len) {
            return rc::gen::container<std::string>(
                len,
                rc::gen::suchThat(rc::gen::inRange<char>(32, 127), [](char c) {
                    // Exclude characters that would break JSON strings: " and backslash
                    return c != '"' && c != '\\';
                })
            );
        }
    );
}

// Generator for a random JSON key that is NOT "status" or "version"
static rc::Gen<std::string> genOtherKey() {
    return rc::gen::suchThat(genJsonSafeString(1, 20), [](const std::string& s) {
        return s != "status" && s != "version";
    });
}

// --- Generators for invalid JSON (Property 4) ---

// Generator for random alphanumeric strings that are NOT valid JSON primitives
// Ensures at least one letter is present (pure digit strings are valid JSON numbers)
static rc::Gen<std::string> genAlphanumeric(int minLen, int maxLen) {
    return rc::gen::suchThat(
        rc::gen::mapcat(
            rc::gen::inRange(minLen, maxLen + 1),
            [](int len) {
                return rc::gen::container<std::string>(
                    len,
                    rc::gen::oneOf(
                        rc::gen::inRange<char>('a', 'z' + 1),
                        rc::gen::inRange<char>('A', 'Z' + 1),
                        rc::gen::inRange<char>('0', '9' + 1)
                    )
                );
            }
        ),
        [](const std::string& s) {
            // Filter out strings that ArduinoJson would parse as valid JSON:
            // - "true", "false", "null" are valid JSON primitives
            // - All-digit strings are valid JSON numbers
            if (s == "true" || s == "false" || s == "null") return false;
            // Ensure at least one non-digit character exists
            bool hasLetter = false;
            for (char c : s) {
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                    hasLetter = true;
                    break;
                }
            }
            return hasLetter;
        }
    );
}

// Generator for strings that do NOT start with '{', '[', '"', or digits/minus
// (avoiding characters that could start valid JSON values)
static rc::Gen<std::string> genNonJsonStart() {
    return rc::gen::apply(
        [](char first, const std::string& rest) {
            return first + rest;
        },
        rc::gen::suchThat<char>(rc::gen::inRange<char>(32, 127), [](char c) {
            // Exclude characters that could start valid JSON: { [ " - digits t f n
            return c != '{' && c != '[' && c != '"' && c != '-'
                && !(c >= '0' && c <= '9')
                && c != 't' && c != 'f' && c != 'n';
        }),
        rc::gen::container<std::string>(rc::gen::inRange<char>(32, 127))
    );
}

// Generator for partial/malformed JSON strings
static rc::Gen<std::string> genMalformedJson() {
    return rc::gen::oneOf(
        // Partial JSON: opening brace with key but no closing
        rc::gen::map(genAlphanumeric(1, 20), [](const std::string& key) {
            return "{\"" + key + "\":";
        }),
        // Unclosed string value
        rc::gen::map(genAlphanumeric(1, 20), [](const std::string& key) {
            return "{\"" + key + "\": \"unclosed";
        }),
        // Missing closing brace
        rc::gen::map(genAlphanumeric(1, 15), [](const std::string& val) {
            return "{\"status\": \"" + val + "\"";
        }),
        // Trailing comma (invalid JSON)
        rc::gen::map(genAlphanumeric(1, 15), [](const std::string& val) {
            return "{\"status\": \"" + val + "\",}";
        }),
        // Just an opening brace
        rc::gen::just(std::string("{")),
        // Double opening braces
        rc::gen::just(std::string("{{"))
    );
}

// Generator for HTML-like content
static rc::Gen<std::string> genHtmlContent() {
    return rc::gen::map(genAlphanumeric(1, 30), [](const std::string& content) {
        return "<html><body>" + content + "</body></html>";
    });
}

// Generator for strings that look like primitives but are NOT valid standalone JSON
// Note: ArduinoJson parses standalone true/false/null/numbers as valid JSON,
// so we only generate strings that are truly unparseable
static rc::Gen<std::string> genUnparseableStrings() {
    return rc::gen::oneOf(
        // Strings with leading/trailing garbage around numbers
        rc::gen::map(rc::gen::inRange(1, 1000), [](int n) {
            return std::to_string(n) + "abc";
        }),
        // Undefined-like values
        rc::gen::just(std::string("undefined")),
        // NaN (not valid JSON)
        rc::gen::just(std::string("NaN")),
        // Infinity (not valid JSON)
        rc::gen::just(std::string("Infinity")),
        // Multiple values (not valid JSON)
        rc::gen::just(std::string("true false"))
    );
}

// Combined generator for all types of invalid JSON
static rc::Gen<std::string> genInvalidJson() {
    return rc::gen::oneOf(
        // Empty string
        rc::gen::just(std::string("")),
        // Random alphanumeric strings (letters won't parse as JSON unless they spell true/false/null)
        genAlphanumeric(1, 50),
        // Strings starting with non-JSON characters
        genNonJsonStart(),
        // Malformed JSON
        genMalformedJson(),
        // HTML content
        genHtmlContent(),
        // Unparseable primitive-like strings
        genUnparseableStrings()
    );
}

/**
 * Feature: wifi-config-app, Property 3: JSON health response parsing extracts fields correctly
 *
 * Validates: Requirements 7.4, 9.1, 9.2
 *
 * For any valid JSON string containing a "status" field with an arbitrary string
 * value and a "version" field with an arbitrary string value, the health response
 * parser shall extract both fields and return them unchanged.
 */
TEST_CASE("Property 3: JSON health response parsing extracts fields correctly", "[property][health_parser]") {
    rc::check("valid JSON with status and version fields are extracted unchanged",
        []() {
            // Generate random status string: 1-31 chars, printable ASCII, no quotes/backslashes
            const auto status = *genJsonSafeString(1, 31);
            // Generate random version string: 1-31 chars, printable ASCII, no quotes/backslashes
            const auto version = *genJsonSafeString(1, 31);

            // Construct valid JSON: {"status":"<random>","version":"<random>"}
            std::string json = "{\"status\":\"" + status + "\",\"version\":\"" + version + "\"}";

            // Parse it
            HealthResult result = parseHealthResponse(json.c_str());

            // Verify parsing succeeded
            RC_ASSERT(result.success == true);
            RC_ASSERT(result.missingFields == false);

            // Verify both fields are extracted unchanged
            RC_ASSERT(std::string(result.status) == status);
            RC_ASSERT(std::string(result.version) == version);
        }
    );
}

/**
 * Feature: wifi-config-app, Property 4: Invalid JSON produces parse failure
 *
 * Validates: Requirements 9.3
 *
 * For any string that is not valid JSON, the health response parser shall
 * return a failure result with empty status and version fields and no partial
 * data extraction.
 */
TEST_CASE("Property 4: Invalid JSON produces parse failure", "[property][health_parser]") {
    rc::check("any non-JSON string produces failure with empty fields",
        []() {
            const auto input = *genInvalidJson();

            HealthResult result = parseHealthResponse(input.c_str());

            // Invalid JSON should result in success=false
            RC_ASSERT(result.success == false);
            // Status field must be empty (no partial extraction)
            RC_ASSERT(result.status[0] == '\0');
            // Version field must be empty (no partial extraction)
            RC_ASSERT(result.version[0] == '\0');
            // missingFields should be false (not applicable for invalid JSON)
            RC_ASSERT(result.missingFields == false);
        }
    );
}

/**
 * Feature: wifi-config-app, Property 5: Valid JSON with missing fields signals incomplete data
 *
 * Validates: Requirements 9.4
 *
 * For any valid JSON object that does not contain a "status" field, a "version" field,
 * or both, the health response parser shall return success with the missingFields flag
 * set to true, and any present field shall be extracted correctly.
 */
TEST_CASE("Property 5: Valid JSON with missing fields signals incomplete data", "[property][health_parser]") {

    SECTION("JSON with only status field (missing version)") {
        rc::check("valid JSON with status but no version returns success with missingFields=true",
            []() {
                const auto statusVal = *genJsonSafeString(1, 30);

                std::string json = "{\"status\":\"" + statusVal + "\"}";
                HealthResult result = parseHealthResponse(json.c_str());

                RC_ASSERT(result.success == true);
                RC_ASSERT(result.missingFields == true);
                // Status field is present and extracted correctly
                RC_ASSERT(std::string(result.status) == statusVal);
                // Version field is missing, so output is empty
                RC_ASSERT(std::string(result.version) == "");
            }
        );
    }

    SECTION("JSON with only version field (missing status)") {
        rc::check("valid JSON with version but no status returns success with missingFields=true",
            []() {
                const auto versionVal = *genJsonSafeString(1, 30);

                std::string json = "{\"version\":\"" + versionVal + "\"}";
                HealthResult result = parseHealthResponse(json.c_str());

                RC_ASSERT(result.success == true);
                RC_ASSERT(result.missingFields == true);
                // Status field is missing, so output is empty
                RC_ASSERT(std::string(result.status) == "");
                // Version field is present and extracted correctly
                RC_ASSERT(std::string(result.version) == versionVal);
            }
        );
    }

    SECTION("JSON with neither status nor version") {
        rc::check("valid JSON with neither status nor version returns success with missingFields=true",
            []() {
                const auto key = *genOtherKey();
                const auto val = *genJsonSafeString(0, 20);

                std::string json = "{\"" + key + "\":\"" + val + "\"}";
                HealthResult result = parseHealthResponse(json.c_str());

                RC_ASSERT(result.success == true);
                RC_ASSERT(result.missingFields == true);
                RC_ASSERT(std::string(result.status) == "");
                RC_ASSERT(std::string(result.version) == "");
            }
        );
    }

    SECTION("JSON with extra fields but missing one or both required fields") {
        rc::check("valid JSON with extra fields but missing version returns success with missingFields=true",
            []() {
                const auto statusVal = *genJsonSafeString(1, 30);
                const auto extraKey = *genOtherKey();
                const auto extraVal = *genJsonSafeString(0, 20);

                std::string json = "{\"status\":\"" + statusVal + "\",\"" + extraKey + "\":\"" + extraVal + "\"}";
                HealthResult result = parseHealthResponse(json.c_str());

                RC_ASSERT(result.success == true);
                RC_ASSERT(result.missingFields == true);
                RC_ASSERT(std::string(result.status) == statusVal);
                RC_ASSERT(std::string(result.version) == "");
            }
        );

        rc::check("valid JSON with extra fields but missing status returns success with missingFields=true",
            []() {
                const auto versionVal = *genJsonSafeString(1, 30);
                const auto extraKey = *genOtherKey();
                const auto extraVal = *genJsonSafeString(0, 20);

                std::string json = "{\"version\":\"" + versionVal + "\",\"" + extraKey + "\":\"" + extraVal + "\"}";
                HealthResult result = parseHealthResponse(json.c_str());

                RC_ASSERT(result.success == true);
                RC_ASSERT(result.missingFields == true);
                RC_ASSERT(std::string(result.status) == "");
                RC_ASSERT(std::string(result.version) == versionVal);
            }
        );

        rc::check("valid JSON with extra fields but missing both status and version returns success with missingFields=true",
            []() {
                const auto key1 = *genOtherKey();
                const auto val1 = *genJsonSafeString(0, 20);
                const auto key2 = *rc::gen::suchThat(genOtherKey(), [&key1](const std::string& s) {
                    return s != key1;
                });
                const auto val2 = *genJsonSafeString(0, 20);

                std::string json = "{\"" + key1 + "\":\"" + val1 + "\",\"" + key2 + "\":\"" + val2 + "\"}";
                HealthResult result = parseHealthResponse(json.c_str());

                RC_ASSERT(result.success == true);
                RC_ASSERT(result.missingFields == true);
                RC_ASSERT(std::string(result.status) == "");
                RC_ASSERT(std::string(result.version) == "");
            }
        );
    }
}
