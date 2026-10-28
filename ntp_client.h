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

#include "include/ntp_sync_logic.h"

#define NTP_MAX_RETRIES 5
#define NTP_RETRY_DELAY_MS 10000

// Perform a blocking NTP synchronization. Retries up to NTP_MAX_RETRIES times
// with NTP_RETRY_DELAY_MS between attempts. Used once at boot, where a valid
// epoch is required before the reporting schedule can be aligned.
// Returns true if sync succeeded.
bool ntpClient_sync();

// Advance the non-blocking resync state machine. Call once per loop() in STA
// mode. Sends at most one request and performs at most one receive check per
// call; never blocks. Handles the periodic 24-hour resync with a 5-minute
// backoff retry on failure (issue #9).
void ntpClient_tick();

// Current resync state machine state (for OLED status / logging).
NtpState ntpClient_state();

// Check if NTP has been successfully synchronized at least once.
bool ntpClient_isSynced();

// Whether the clock offset is currently usable for timestamps. True once an
// initial sync has succeeded and remains true through a failing resync, so
// reporting can continue while resync retries in the background.
bool ntpClient_timeValid();

// Get the current UTC epoch time (seconds since 1970-01-01T00:00:00Z).
// Only valid after successful sync.
unsigned long ntpClient_getEpoch();

// Format the current UTC time as ISO 8601 string with timezone offset.
// Writes to the provided buffer (must be at least 26 bytes).
// Example output: "2024-01-15T10:30:00+00:00"
void ntpClient_formatISO8601(char* buffer, int bufferSize);

#endif
