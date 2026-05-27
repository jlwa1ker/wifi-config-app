#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <string>

#include "landing_page_generator.h"

// Generator for printable ASCII strings within a given length range
static rc::Gen<std::string> genPrintableAscii(int minLen, int maxLen) {
    return rc::gen::mapcat(
        rc::gen::inRange(minLen, maxLen + 1),
        [](int len) {
            return rc::gen::container<std::string>(
                len,
                rc::gen::inRange<char>(32, 127)
            );
        }
    );
}

/**
 * Feature: wifi-config-app, Property 7: Landing page HTML contains health data
 *
 * Validates: Requirements 8.2
 *
 * For any non-empty status string (1-31 chars, printable ASCII) and non-empty
 * version string (1-31 chars, printable ASCII), the generated HTML contains
 * both strings verbatim (using std::string::find).
 */
TEST_CASE("Property 7: Landing page HTML contains health data", "[property][landing_page]") {
    rc::check("for any non-empty status and version, generated HTML contains both verbatim",
        []() {
            // Generate non-empty status string: 1-31 chars, printable ASCII
            const auto status = *genPrintableAscii(1, 31);
            // Generate non-empty version string: 1-31 chars, printable ASCII
            const auto version = *genPrintableAscii(1, 31);

            // Generate HTML with hasFailed=false (normal success case)
            // This ensures the status table branch is taken
            std::string html = generateLandingPageHtml(status.c_str(), version.c_str(),
                                                       false, false);

            // The generated HTML must contain the status string verbatim
            RC_ASSERT(html.find(status) != std::string::npos);
            // The generated HTML must contain the version string verbatim
            RC_ASSERT(html.find(version) != std::string::npos);
        }
    );
}
