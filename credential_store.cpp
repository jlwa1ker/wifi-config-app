#include "credential_store.h"
#include <FlashStorage.h>
#include <string.h>

// Allocate a flash page for the WiFiCredentials struct
FlashStorage(flash_store, WiFiCredentials);

// Module-level cached copy of credentials read from flash
static WiFiCredentials cachedCredentials;

void credentialStore_init() {
  cachedCredentials = flash_store.read();
}

bool credentialStore_hasCredentials() {
  return cachedCredentials.valid;
}

bool credentialStore_read(WiFiCredentials& creds) {
  if (!cachedCredentials.valid) {
    return false;
  }
  creds = cachedCredentials;
  return true;
}

bool credentialStore_write(const char* ssid, const char* password) {
  cachedCredentials.valid = true;
  strncpy(cachedCredentials.ssid, ssid, MAX_SSID_LENGTH);
  cachedCredentials.ssid[MAX_SSID_LENGTH] = '\0';
  strncpy(cachedCredentials.password, password, MAX_PASS_LENGTH);
  cachedCredentials.password[MAX_PASS_LENGTH] = '\0';
  flash_store.write(cachedCredentials);
  return true;
}

void credentialStore_clear() {
  cachedCredentials.valid = false;
  flash_store.write(cachedCredentials);
}
