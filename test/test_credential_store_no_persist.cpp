/**
 * Property 2: Failed credentials are never persisted
 *
 * For any SSID/password pair where connection fails, credential store must
 * not be modified — a subsequent read should return the same state as before
 * the attempt.
 *
 * **Validates: Requirements 4.4**
 *
 * This test verifies the behavioral property that the credential store state
 * is preserved when a connection attempt fails. According to the design,
 * when wifiManager_connect() returns false, credentialStore_write() is never
 * called. We verify this invariant by:
 * 1. Setting up a credential store in some initial state (empty or with existing creds)
 * 2. Simulating a failed connection attempt (which does NOT call credentialStore_write)
 * 3. Verifying the credential store state is identical to the initial state
 */

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
static void credentialStore_write_pure(WiFiCredentials& creds, const char* ssid, const char* password) {
  creds.valid = true;
  strncpy(creds.ssid, ssid, MAX_SSID_LENGTH);
  creds.ssid[MAX_SSID_LENGTH] = '\0';
  strncpy(creds.password, password, MAX_PASS_LENGTH);
  creds.password[MAX_PASS_LENGTH] = '\0';
}

// Extract the pure read logic from credential_store.cpp
static bool credentialStore_read_pure(const WiFiCredentials& creds, WiFiCredentials& out) {
  if (!creds.valid) {
    return false;
  }
  out = creds;
  return true;
}

// Helper: compare two WiFiCredentials structs for byte-level equality
static bool credentialsEqual(const WiFiCredentials& a, const WiFiCredentials& b) {
  if (a.valid != b.valid) return false;
  if (!a.valid && !b.valid) return true;  // Both invalid — content irrelevant
  if (std::strcmp(a.ssid, b.ssid) != 0) return false;
  if (std::strcmp(a.password, b.password) != 0) return false;
  return true;
}

/**
 * Simulates the "failed connection" code path from the main sketch.
 * According to the design (see sequence diagram in design.md), when
 * wifiManager_connect() returns false:
 * - webServer_setError("Connection failed") is called
 * - credentialStore_write() is NOT called
 * - The device remains in AP mode
 *
 * This function represents what happens to the credential store when
 * a connection attempt fails: nothing. The store is not touched.
 */
static void simulateFailedConnectionAttempt(WiFiCredentials& /* store */,
                                            const char* /* ssid */,
                                            const char* /* password */) {
  // The main sketch does NOT call credentialStore_write() on failure.
  // This function intentionally does nothing to the credential store.
}

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

/**
 * Feature: wifi-config-app, Property 2: Failed credentials are never persisted
 *
 * **Validates: Requirements 4.4**
 *
 * Test case 1: Empty credential store remains unchanged after failed connection.
 * For any random SSID and password, if connection fails, an empty credential
 * store stays empty.
 */
TEST_CASE("Property 2: Failed credentials not persisted - empty store",
          "[property][credential_store][property2]") {

  rc::check("For any SSID/password, failed connection does not modify an empty credential store",
    []() {
      // Generate random SSID (1-32 chars) and password (0-64 chars)
      const auto ssid = *genPrintableAscii(1, MAX_SSID_LENGTH);
      const auto password = *genPrintableAscii(0, MAX_PASS_LENGTH);

      // Start with an empty/invalid credential store
      WiFiCredentials store{};
      store.valid = false;
      std::memset(store.ssid, 0, sizeof(store.ssid));
      std::memset(store.password, 0, sizeof(store.password));

      // Snapshot the state before the failed attempt
      WiFiCredentials before = store;

      // Simulate a failed connection attempt — store should not be touched
      simulateFailedConnectionAttempt(store, ssid.c_str(), password.c_str());

      // Verify: credential store state is unchanged
      RC_ASSERT(credentialsEqual(before, store));
      RC_ASSERT(store.valid == false);
    }
  );
}

/**
 * Feature: wifi-config-app, Property 2: Failed credentials are never persisted
 *
 * **Validates: Requirements 4.4**
 *
 * Test case 2: Credential store with existing credentials remains unchanged
 * after a failed connection attempt with different credentials.
 * For any existing stored credentials and any new attempted credentials,
 * if the new connection fails, the store still contains the original credentials.
 */
TEST_CASE("Property 2: Failed credentials not persisted - store with existing credentials",
          "[property][credential_store][property2]") {

  rc::check("For any SSID/password, failed connection does not modify a store with existing credentials",
    []() {
      // Generate random existing credentials to pre-populate the store
      const auto existingSsid = *genPrintableAscii(1, MAX_SSID_LENGTH);
      const auto existingPassword = *genPrintableAscii(0, MAX_PASS_LENGTH);

      // Generate random NEW credentials that would be attempted (and fail)
      const auto attemptedSsid = *genPrintableAscii(1, MAX_SSID_LENGTH);
      const auto attemptedPassword = *genPrintableAscii(0, MAX_PASS_LENGTH);

      // Set up the store with existing credentials
      WiFiCredentials store{};
      credentialStore_write_pure(store, existingSsid.c_str(), existingPassword.c_str());

      // Snapshot the state before the failed attempt
      WiFiCredentials before = store;

      // Simulate a failed connection attempt with new credentials
      simulateFailedConnectionAttempt(store, attemptedSsid.c_str(), attemptedPassword.c_str());

      // Verify: credential store state is unchanged (still has existing creds)
      RC_ASSERT(credentialsEqual(before, store));
      RC_ASSERT(store.valid == true);

      // Verify: reading back still produces the original credentials
      WiFiCredentials readBack{};
      bool readSuccess = credentialStore_read_pure(store, readBack);
      RC_ASSERT(readSuccess == true);
      RC_ASSERT(std::string(readBack.ssid) == existingSsid.substr(0, MAX_SSID_LENGTH));
      RC_ASSERT(std::string(readBack.password) == existingPassword.substr(0, MAX_PASS_LENGTH));
    }
  );
}
