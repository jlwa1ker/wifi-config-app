#ifndef MOCK_WIFICLIENT_H
#define MOCK_WIFICLIENT_H

// Mock WiFiClient for host-based testing.
// Provides minimal stubs so that server_reporter.cpp and health_client.cpp
// can compile without Arduino hardware.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include "WiFi101.h"  // For IPAddress

class WiFiClient {
public:
    WiFiClient() : connected_(false), rx_len_(0), read_pos_(0), tx_len_(0) {
        memset(rx_buffer_, 0, sizeof(rx_buffer_));
        memset(tx_buffer_, 0, sizeof(tx_buffer_));
    }

    // Connect to a server by IP address
    int connect(IPAddress ip, uint16_t port) {
        (void)ip;
        (void)port;
        connected_ = mock_connect_success_;
        return connected_ ? 1 : 0;
    }

    // Connect to a server by hostname
    int connect(const char* host, uint16_t port) {
        (void)host;
        (void)port;
        connected_ = mock_connect_success_;
        return connected_ ? 1 : 0;
    }

    // Check if connected
    bool connected() const {
        return connected_;
    }

    // Explicit bool conversion
    operator bool() const {
        return connected_;
    }

    // Number of bytes available to read
    int available() {
        return (int)(rx_len_ - read_pos_);
    }

    // Read a single byte
    int read() {
        if (read_pos_ < rx_len_) {
            return rx_buffer_[read_pos_++];
        }
        return -1;
    }

    // Read multiple bytes
    int read(uint8_t* buf, size_t size) {
        size_t avail = rx_len_ - read_pos_;
        size_t to_read = (size < avail) ? size : avail;
        if (to_read > 0) {
            memcpy(buf, rx_buffer_ + read_pos_, to_read);
            read_pos_ += to_read;
        }
        return (int)to_read;
    }

    // Write a single byte
    size_t write(uint8_t b) {
        if (tx_len_ < sizeof(tx_buffer_)) {
            tx_buffer_[tx_len_++] = b;
            return 1;
        }
        return 0;
    }

    // Write a buffer
    size_t write(const uint8_t* buf, size_t size) {
        size_t to_write = size;
        if (tx_len_ + to_write > sizeof(tx_buffer_)) {
            to_write = sizeof(tx_buffer_) - tx_len_;
        }
        memcpy(tx_buffer_ + tx_len_, buf, to_write);
        tx_len_ += to_write;
        return to_write;
    }

    // Print a string (mimics Arduino Print class)
    size_t print(const char* str) {
        size_t len = strlen(str);
        return write((const uint8_t*)str, len);
    }

    size_t print(int val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", val);
        return print(buf);
    }

    size_t println(const char* str) {
        size_t n = print(str);
        n += print("\r\n");
        return n;
    }

    size_t println() {
        return print("\r\n");
    }

    // Close the connection
    void stop() {
        connected_ = false;
    }

    void flush() {
        // No-op in mock
    }

    // Test helpers to control mock behavior
    void setMockConnectSuccess(bool success) { mock_connect_success_ = success; }
    void setMockConnected(bool connected) { connected_ = connected; }
    void setMockRxData(const char* data) {
        size_t len = strlen(data);
        size_t to_copy = (len < sizeof(rx_buffer_)) ? len : sizeof(rx_buffer_);
        memcpy(rx_buffer_, data, to_copy);
        rx_len_ = to_copy;
        read_pos_ = 0;
    }
    void setMockRxData(const uint8_t* data, size_t len) {
        size_t to_copy = (len < sizeof(rx_buffer_)) ? len : sizeof(rx_buffer_);
        memcpy(rx_buffer_, data, to_copy);
        rx_len_ = to_copy;
        read_pos_ = 0;
    }
    const uint8_t* getMockTxData() const { return tx_buffer_; }
    size_t getMockTxLen() const { return tx_len_; }
    void clearMockTx() { tx_len_ = 0; memset(tx_buffer_, 0, sizeof(tx_buffer_)); }

private:
    bool connected_;
    bool mock_connect_success_ = true;

    uint8_t rx_buffer_[2048];
    size_t rx_len_;
    size_t read_pos_;

    uint8_t tx_buffer_[2048];
    size_t tx_len_;
};

#endif
