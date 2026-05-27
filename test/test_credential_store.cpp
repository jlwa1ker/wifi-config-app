#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <cstring>
#include <string>

// Replicate the constants from config.h for host-side testing
static const int MAX_SSID_LENGTH = 32;
static const int MAX_PASS_LENGTH = 64;

// Replicate the WiFiCredentials struct from credential_store.h
struct WiFiCredentials {
    bool valid;
    char ssid[MAX_SSID_LENGTH + 1];
    char password[MAX_PASS_LENGTH + 1];
};

// Extract the pure write logic from credential_store.cpp (no flash dependency)
// This mirrors the strncpy + null-termination logic in credentialStore_write()
static bool credentialStore_write_pure(WiFiCredentials& creds, const char* ssid, const char* password) {
    creds.valid = true;
    strncpy(creds.ssid, ssid, MAX_SSID_LENGTH);
    creds.ssid[MAX_SSID_LENGTH] = '\0';
    strncpy(creds.password, password, MAX_PASS_LENGTH);
    creds.password[MAX_PASS_LENGTH] = '\0';
    return true;
}

// Extract the pure read logic from credential_store.cpp
static bool credentialStore_read_pure(const WiFiCredentials& creds, WiFiCredentials& out) {
    if (!creds.valid) {
        return false;
    }
    out = creds;
    return true;
}

// Generator for printable ASCII strings within a given length range
static rc::Gen<std::string> genPrintableAscii(int minLen, int maxLen) {
    return rc::gen::mapcat(
        rc::gen::inRange(minLen, maxLen + 1),
        [](int len) {
            return rc::gen::container<std::string>(
                len,
                rc::gen::inRange<char>(32, 127)  // printable ASCII: space (32) to tilde (126)
            );
        }
    );
}

/**
 * Feature: wifi-config-app, Property 1: Credential persistence round-trip
 *
 * Validates: Requirements 5.1, 5.3
 *
 * For any valid SSID (1-32 chars, printable ASCII) and password (0-64 chars,
 * printable ASCII), writing to a WiFiCredentials struct and reading back
 * produces identical values with valid=true.
 */
TEST_CASE("Property 1: Credential persistence round-trip", "[property][credential_store]") {
    rc::check("write then read produces exact same SSID and password with valid=true",
        []() {
            // Generate valid SSID: 1-32 printable ASCII characters
            const auto ssid = *genPrintableAscii(1, MAX_SSID_LENGTH);
            // Generate valid password: 0-64 printable ASCII characters
            const auto password = *genPrintableAscii(0, MAX_PASS_LENGTH);

            // Simulate write
            WiFiCredentials store = {};
            credentialStore_write_pure(store, ssid.c_str(), password.c_str());

            // Simulate read
            WiFiCredentials readBack = {};
            bool readSuccess = credentialStore_read_pure(store, readBack);

            // Assertions
            RC_ASSERT(readSuccess == true);
            RC_ASSERT(readBack.valid == true);
            RC_ASSERT(std::string(readBack.ssid) == ssid);
            RC_ASSERT(std::string(readBack.password) == password);
        }
    );
}
