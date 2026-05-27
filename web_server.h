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
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi101.h>

// Initialize the web server on the specified port
void webServer_init(int port);

// Poll for incoming HTTP requests and handle them.
// Must be called in loop().
void webServer_poll();

// Set the current mode to determine which page to serve
enum WebServerMode { MODE_CONFIG_FORM, MODE_LANDING_PAGE };
void webServer_setMode(WebServerMode mode);

// Set an error message to display on the config form
void webServer_setError(const char* errorMsg);

// Set health data for the landing page
void webServer_setHealthData(const char* status, const char* version, bool isCached, bool hasFailed);

// Check if a landing page request was just served (returns true once, then resets)
bool webServer_wasRequestServed();

// Check if a credential submission is pending
bool webServer_hasSubmission();

// Retrieve the submitted credentials and clear the submission flag.
// Buffers must be at least MAX_SSID_LENGTH+1 and MAX_PASS_LENGTH+1 bytes.
void webServer_getSubmission(char* ssid, char* password);

#endif
