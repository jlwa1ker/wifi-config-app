#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

// Mock Arduino.h for host-based testing.
// Provides minimal stubs for common Arduino functions used by the modules.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>

// Arduino type aliases
typedef uint8_t byte;
typedef bool boolean;

// Pin modes (unused in our modules but commonly included)
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define HIGH 1
#define LOW 0

// Math helpers
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

// Mock millis() — returns a controllable value for testing
// Tests can set this via mock_millis_value
static unsigned long mock_millis_value = 0;

inline unsigned long millis() {
    return mock_millis_value;
}

inline void setMockMillis(unsigned long value) {
    mock_millis_value = value;
}

// Mock delay() — no-op in tests
inline void delay(unsigned long ms) {
    (void)ms;
    // In tests, delay is a no-op. Advance mock_millis_value if needed.
}

// Mock Serial (minimal stub for debug output)
class MockSerial {
public:
    void begin(unsigned long baud) { (void)baud; }
    void print(const char* s) { (void)s; }
    void print(int v) { (void)v; }
    void print(unsigned long v) { (void)v; }
    void print(float v) { (void)v; }
    void println(const char* s = "") { (void)s; }
    void println(int v) { (void)v; }
    void println(unsigned long v) { (void)v; }
    void println(float v) { (void)v; }
    operator bool() const { return true; }
};

static MockSerial Serial;

// roundf is standard C but ensure it's available
#ifndef roundf
#define roundf(x) ((float)((int)((x) + ((x) >= 0 ? 0.5f : -0.5f))))
#endif

#endif // MOCK_ARDUINO_H
