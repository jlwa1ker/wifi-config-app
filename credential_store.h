#ifndef CREDENTIAL_STORE_H
#define CREDENTIAL_STORE_H

#include "config.h"

struct WiFiCredentials {
  bool valid;                       // Validity flag
  char ssid[MAX_SSID_LENGTH + 1];  // Null-terminated SSID
  char password[MAX_PASS_LENGTH + 1]; // Null-terminated password
};

// Initialize the credential store
void credentialStore_init();

// Check if valid credentials are stored
bool credentialStore_hasCredentials();

// Read stored credentials. Returns false if no valid credentials.
bool credentialStore_read(WiFiCredentials& creds);

// Write credentials to flash. Returns true on success.
bool credentialStore_write(const char* ssid, const char* password);

// Clear stored credentials (sets valid flag to false)
void credentialStore_clear();

#endif
