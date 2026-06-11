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
#ifndef CREDENTIAL_STORE_H
#define CREDENTIAL_STORE_H

#include "config.h"

struct WiFiCredentials {
  bool valid;                           // Validity flag
  char ssid[MAX_SSID_LENGTH + 1];      // Null-terminated SSID
  char password[MAX_PASS_LENGTH + 1];   // Null-terminated password
  char location[MAX_LOCATION_LENGTH + 1]; // Null-terminated location
};

// Initialize the credential store
void credentialStore_init();

// Check if valid credentials are stored
bool credentialStore_hasCredentials();

// Read stored credentials. Returns false if no valid credentials.
bool credentialStore_read(WiFiCredentials& creds);

// Write credentials to flash. Returns true on success.
bool credentialStore_write(const char* ssid, const char* password, const char* location);

// Clear stored credentials (sets valid flag to false)
void credentialStore_clear();

#endif
