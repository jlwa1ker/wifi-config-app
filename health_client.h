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
#ifndef HEALTH_CLIENT_H
#define HEALTH_CLIENT_H

struct HealthResult {
  bool success;           // Whether the request succeeded and parsed correctly
  char status[32];       // Parsed "status" field
  char version[32];      // Parsed "version" field
  bool missingFields;    // True if JSON was valid but fields were missing
};

// Perform a health check with retries.
// Returns the result of the health check.
HealthResult healthClient_fetch();

// Get cached health data (from last successful fetch)
// Returns false if no cached data is available
bool healthClient_getCached(char* status, char* version);

#endif
