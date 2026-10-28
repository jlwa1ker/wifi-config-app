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
#include "ntp_client.h"
#include <Arduino.h>
#include <WiFiUdp.h>
#include "include/ntp_format.h"
#include "include/ntp_sync_logic.h"

// NTP server and port
static const char* NTP_SERVER = "pool.ntp.org";
static const int NTP_PORT = 123;

// NTP packet size
static const int NTP_PACKET_SIZE = 48;

// Module-level static state
static WiFiUDP udp;
static unsigned long epochOffset = 0;       // epoch - (millis()/1000) at sync time
static unsigned long lastSyncMillis = 0;    // millis() when last sync succeeded
static bool synced = false;

// Non-blocking resync state machine (see include/ntp_sync_logic.h). Lazily
// initialized on first ntpClient_tick() so the blocking boot sync runs first.
static NtpSyncState fsm;
static bool fsmInitialized = false;

// Send an NTP request packet to the server
static void sendNTPPacket() {
  uint8_t packet[NTP_PACKET_SIZE];
  memset(packet, 0, NTP_PACKET_SIZE);

  // NTP request header: LI=3 (unsynchronized), Version=4, Mode=3 (client)
  // Binary: 11 100 011 = 0xE3
  packet[0] = 0xE3;

  udp.beginPacket(NTP_SERVER, NTP_PORT);
  udp.write(packet, NTP_PACKET_SIZE);
  udp.endPacket();
}

// Parse an NTP reply from a 48-byte response buffer and store the resulting
// clock offset. Updates epochOffset, lastSyncMillis, and synced.
static void applyNtpResponse(const uint8_t* response) {
  // Extract the transmit timestamp (seconds since 1900-01-01)
  // from bytes 40-43 of the response (big-endian 32-bit integer)
  unsigned long ntpTime = (unsigned long)response[40] << 24 |
                          (unsigned long)response[41] << 16 |
                          (unsigned long)response[42] << 8  |
                          (unsigned long)response[43];

  // Convert NTP time (since 1900) to Unix epoch (since 1970)
  // 70 years of seconds = 2208988800UL
  const unsigned long SEVENTY_YEARS = 2208988800UL;
  unsigned long epoch = ntpTime - SEVENTY_YEARS;

  // Store offset: epoch = offset + (millis()/1000)
  epochOffset = epoch - (millis() / 1000);
  lastSyncMillis = millis();
  synced = true;
}

// Check for an available NTP reply without blocking. Returns true and applies
// the offset if a full packet has arrived; returns false otherwise.
static bool pollForResponse() {
  if (udp.parsePacket() >= NTP_PACKET_SIZE) {
    uint8_t response[NTP_PACKET_SIZE];
    udp.read(response, NTP_PACKET_SIZE);
    applyNtpResponse(response);
    return true;
  }
  return false;
}

// Attempt a single blocking NTP sync. Returns true if epoch was obtained.
// Used only by the blocking boot path (ntpClient_sync).
static bool attemptSync() {
  udp.begin(2390);  // Local port for NTP
  sendNTPPacket();

  // Wait up to the response timeout for a reply
  unsigned long start = millis();
  while (millis() - start < NTP_RESPONSE_TIMEOUT_MS) {
    if (pollForResponse()) {
      udp.stop();
      return true;
    }
    delay(50);
  }

  udp.stop();
  return false;
}

bool ntpClient_sync() {
  for (int attempt = 0; attempt < NTP_MAX_RETRIES; attempt++) {
    if (attempt > 0) {
      delay(NTP_RETRY_DELAY_MS);
    }
    if (attemptSync()) {
      return true;
    }
  }
  return false;
}

void ntpClient_tick() {
  // Lazily initialize the FSM on first use. If a blocking boot sync already
  // succeeded, start IDLE with the normal 24-hour cadence from that sync;
  // otherwise start WAITING_TO_SYNC to attempt a first sync.
  if (!fsmInitialized) {
    ntpSync_init(fsm);
    if (synced) {
      fsm.state = NTP_IDLE;
      fsm.everSynced = true;
      fsm.lastAttemptFailed = false;
      fsm.lastAttemptMillis = lastSyncMillis;
    }
    fsmInitialized = true;
  }

  unsigned long now = millis();

  // Advance timer-only transitions (IDLE/BACKOFF -> WAITING_TO_SYNC when due).
  ntpSync_advanceTimers(fsm, now);

  switch (fsm.state) {
    case NTP_WAITING_TO_SYNC: {
      // Send one request and move to awaiting a reply.
      udp.begin(2390);
      sendNTPPacket();
      ntpSync_onRequestSent(fsm, now);
      break;
    }
    case NTP_AWAITING_RESPONSE: {
      if (pollForResponse()) {
        udp.stop();
        ntpSync_onSuccess(fsm, millis());
      } else if (ntpSync_responseTimedOut(fsm, now)) {
        udp.stop();
        ntpSync_onFailure(fsm, now);
      }
      break;
    }
    case NTP_IDLE:
    case NTP_BACKOFF:
      // Nothing to do; handled by ntpSync_advanceTimers above.
      break;
  }
}

NtpState ntpClient_state() {
  return fsm.state;
}

bool ntpClient_isSynced() {
  return synced;
}

bool ntpClient_timeValid() {
  return synced;
}

unsigned long ntpClient_getEpoch() {
  return epochOffset + (millis() / 1000);
}

void ntpClient_formatISO8601(char* buffer, int bufferSize) {
  unsigned long epoch = ntpClient_getEpoch();
  formatEpochISO8601((uint32_t)epoch, buffer, bufferSize);
}
