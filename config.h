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
#ifndef CONFIG_H
#define CONFIG_H

// Access Point settings
const char* const AP_SSID = "tempmon";
const int WEB_SERVER_PORT = 80;

// WiFi connection
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;

// Health endpoint
const char* const HEALTH_HOST = "tempmon.walkerweb.us";
const char* const HEALTH_PATH = "/health";
const int HEALTH_PORT = 80;
const int HEALTH_MAX_RETRIES = 3;
const unsigned long HEALTH_RETRY_DELAY_MS = 5000;

// Credential storage
const int MAX_SSID_LENGTH = 32;
const int MAX_PASS_LENGTH = 64;

#endif
