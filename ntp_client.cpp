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
#include <WiFiUdp.h>
#include "test/ntp_format.h"

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

// Attempt a single NTP sync. Returns true if epoch was obtained.
static bool attemptSync() {
  udp.begin(2390);  // Local port for NTP

  sendNTPPacket();

  // Wait up to 2 seconds for a response
  unsigned long start = millis();
  while (millis() - start < 2000) {
    if (udp.parsePacket() >= NTP_PACKET_SIZE) {
      uint8_t response[NTP_PACKET_SIZE];
      udp.read(response, NTP_PACKET_SIZE);
      udp.stop();

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

bool ntpClient_isSynced() {
  return synced;
}

bool ntpClient_needsResync() {
  if (!synced) {
    return true;
  }
  return (millis() - lastSyncMillis) > NTP_RESYNC_INTERVAL_MS;
}

unsigned long ntpClient_getEpoch() {
  return epochOffset + (millis() / 1000);
}

void ntpClient_formatISO8601(char* buffer, int bufferSize) {
  unsigned long epoch = ntpClient_getEpoch();
  formatEpochISO8601((uint32_t)epoch, buffer, bufferSize);
}
