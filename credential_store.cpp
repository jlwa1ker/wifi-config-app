/*
 * Copyright (C) 2025 James L. Walker, Kiro (AI Assistant)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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

bool credentialStore_write(const char* ssid, const char* password, const char* location) {
  cachedCredentials.valid = true;
  cachedCredentials.retryCount = 0;
  strncpy(cachedCredentials.ssid, ssid, MAX_SSID_LENGTH);
  cachedCredentials.ssid[MAX_SSID_LENGTH] = '\0';
  strncpy(cachedCredentials.password, password, MAX_PASS_LENGTH);
  cachedCredentials.password[MAX_PASS_LENGTH] = '\0';
  strncpy(cachedCredentials.location, location, MAX_LOCATION_LENGTH);
  cachedCredentials.location[MAX_LOCATION_LENGTH] = '\0';
  flash_store.write(cachedCredentials);
  return true;
}

void credentialStore_clear() {
  cachedCredentials.valid = false;
  cachedCredentials.retryCount = 0;
  flash_store.write(cachedCredentials);
}

uint8_t credentialStore_incrementRetry() {
  cachedCredentials.retryCount++;
  flash_store.write(cachedCredentials);
  return cachedCredentials.retryCount;
}

void credentialStore_resetRetry() {
  if (cachedCredentials.retryCount != 0) {
    cachedCredentials.retryCount = 0;
    flash_store.write(cachedCredentials);
  }
}

uint8_t credentialStore_getRetryCount() {
  return cachedCredentials.retryCount;
}
