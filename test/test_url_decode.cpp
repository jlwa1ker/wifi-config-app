#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <string>

#include "url_decode.h"

/**
 * Unit tests for urlDecode and parseFormBody functions.
 *
 * Validates: Requirements 3.1
 *
 * These tests verify that URL-encoded form submissions are correctly decoded
 * so that user-provided WiFi credentials are accurately extracted.
 */

// --- Standard URL-encoded characters ---

TEST_CASE("urlDecode: %20 decodes to space", "[unit][url_decode]") {
    char dst[64];
    urlDecode("hello%20world", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "hello world");
}

TEST_CASE("urlDecode: %2F decodes to /", "[unit][url_decode]") {
    char dst[64];
    urlDecode("path%2Fto%2Ffile", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "path/to/file");
}

TEST_CASE("urlDecode: + decodes to space", "[unit][url_decode]") {
    char dst[64];
    urlDecode("hello+world", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "hello world");
}

TEST_CASE("urlDecode: %3D decodes to =", "[unit][url_decode]") {
    char dst[64];
    urlDecode("key%3Dvalue", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "key=value");
}

TEST_CASE("urlDecode: %26 decodes to &", "[unit][url_decode]") {
    char dst[64];
    urlDecode("a%26b", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "a&b");
}

// --- Multiple encoded characters ---

TEST_CASE("urlDecode: multiple encoded characters in one string", "[unit][url_decode]") {
    char dst[64];
    urlDecode("hello%20world%21", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "hello world!");
}

// --- Edge cases ---

TEST_CASE("urlDecode: empty string input produces empty output", "[unit][url_decode]") {
    char dst[64];
    dst[0] = 'X'; // pre-fill to verify it gets overwritten
    urlDecode("", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "");
}

TEST_CASE("urlDecode: already-decoded string passes through unchanged", "[unit][url_decode]") {
    char dst[64];
    urlDecode("hello", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "hello");
}

// --- Malformed percent sequences ---

TEST_CASE("urlDecode: single % at end passes through as literal", "[unit][url_decode]") {
    char dst[64];
    // '%' at end: src[si+1] == '\0', so the condition `src[si+1] != '\0'` fails
    // and the '%' is treated as a regular character
    urlDecode("test%", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "test%");
}

TEST_CASE("urlDecode: %G0 (invalid hex) passes % through as literal", "[unit][url_decode]") {
    char dst[64];
    // 'G0' is not a valid hex pair - strtol will not consume both chars
    urlDecode("test%G0end", dst, sizeof(dst));
    // The '%' is passed through, then 'G', '0', 'e', 'n', 'd' follow as regular chars
    REQUIRE(std::string(dst) == "test%G0end");
}

TEST_CASE("urlDecode: %0 (incomplete - % followed by one char then null) passes % through", "[unit][url_decode]") {
    char dst[64];
    // '%' followed by '0' then '\0': src[si+2] == '\0', so condition fails
    urlDecode("test%0", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "test%0");
}

// --- Buffer size limit ---

TEST_CASE("urlDecode: output is truncated if dst buffer is too small", "[unit][url_decode]") {
    char dst[6]; // only room for 5 chars + null terminator
    urlDecode("hello world", dst, sizeof(dst));
    REQUIRE(std::string(dst) == "hello");
    REQUIRE(dst[5] == '\0');
}

// --- Full form-encoded string via parseFormBody ---

TEST_CASE("parseFormBody: full form-encoded string with encoded characters", "[unit][url_decode]") {
    char ssid[33];
    char password[65];
    parseFormBody("ssid=My%20Network&password=p%40ss", ssid, sizeof(ssid), password, sizeof(password));
    REQUIRE(std::string(ssid) == "My Network");
    REQUIRE(std::string(password) == "p@ss");
}

TEST_CASE("parseFormBody: + in values decoded as space", "[unit][url_decode]") {
    char ssid[33];
    char password[65];
    parseFormBody("ssid=My+Network&password=my+pass", ssid, sizeof(ssid), password, sizeof(password));
    REQUIRE(std::string(ssid) == "My Network");
    REQUIRE(std::string(password) == "my pass");
}

TEST_CASE("parseFormBody: empty body produces empty outputs", "[unit][url_decode]") {
    char ssid[33];
    char password[65];
    parseFormBody("", ssid, sizeof(ssid), password, sizeof(password));
    REQUIRE(std::string(ssid) == "");
    REQUIRE(std::string(password) == "");
}
