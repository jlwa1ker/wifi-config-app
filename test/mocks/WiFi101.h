#ifndef MOCK_WIFI101_H
#define MOCK_WIFI101_H

// Mock WiFi101 for host-based testing.
// Provides minimal stubs so that headers can be included without Arduino.

#include <cstdint>

#define WL_CONNECTED 3
#define WL_IDLE_STATUS 0

class IPAddress {
public:
  IPAddress() : addr_(0) {}
  IPAddress(uint32_t addr) : addr_(addr) {}
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    : addr_((uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24)) {}
  uint32_t addr_;
};

#endif
