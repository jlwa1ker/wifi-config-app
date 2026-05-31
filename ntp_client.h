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
#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#define NTP_MAX_RETRIES 5
#define NTP_RETRY_DELAY_MS 10000
#define NTP_RESYNC_INTERVAL_MS 86400000UL  // 24 hours

// Perform NTP synchronization. Retries up to NTP_MAX_RETRIES times
// with NTP_RETRY_DELAY_MS between attempts.
// Returns true if sync succeeded.
bool ntpClient_sync();

// Check if NTP has been successfully synchronized at least once
bool ntpClient_isSynced();

// Check if a re-sync is needed (24 hours elapsed since last sync)
bool ntpClient_needsResync();

// Get the current UTC epoch time (seconds since 1970-01-01T00:00:00Z).
// Only valid after successful sync.
unsigned long ntpClient_getEpoch();

// Format the current UTC time as ISO 8601 string with timezone offset.
// Writes to the provided buffer (must be at least 26 bytes).
// Example output: "2024-01-15T10:30:00+00:00"
void ntpClient_formatISO8601(char* buffer, int bufferSize);

#endif
