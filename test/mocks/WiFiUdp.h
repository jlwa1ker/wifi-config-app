#ifndef MOCK_WIFIUDP_H
#define MOCK_WIFIUDP_H

// Mock WiFiUDP for host-based testing.
// Provides minimal stubs so that ntp_client.cpp can compile without Arduino hardware.

#include <cstdint>
#include <cstring>
#include "WiFi101.h"  // For IPAddress

#define UDP_TX_PACKET_MAX_SIZE 256

class WiFiUDP {
public:
    WiFiUDP() : local_port_(0), packet_available_(0), rx_len_(0), read_pos_(0) {
        memset(rx_buffer_, 0, sizeof(rx_buffer_));
        memset(tx_buffer_, 0, sizeof(tx_buffer_));
    }

    // Start listening on a port
    uint8_t begin(uint16_t port) {
        local_port_ = port;
        return 1;  // Success
    }

    // Stop listening
    void stop() {
        local_port_ = 0;
    }

    // Begin building a packet to send to a remote host (by IP)
    int beginPacket(IPAddress ip, uint16_t port) {
        remote_ip_ = ip;
        remote_port_ = port;
        tx_len_ = 0;
        return 1;  // Success
    }

    // Begin building a packet to send to a remote host (by hostname)
    int beginPacket(const char* host, uint16_t port) {
        (void)host;
        remote_port_ = port;
        tx_len_ = 0;
        return 1;  // Success
    }

    // Write data to the packet being built
    size_t write(const uint8_t* buffer, size_t size) {
        size_t to_copy = size;
        if (tx_len_ + to_copy > sizeof(tx_buffer_)) {
            to_copy = sizeof(tx_buffer_) - tx_len_;
        }
        memcpy(tx_buffer_ + tx_len_, buffer, to_copy);
        tx_len_ += to_copy;
        return to_copy;
    }

    size_t write(uint8_t byte) {
        return write(&byte, 1);
    }

    // Finish and send the packet
    int endPacket() {
        return mock_send_success_ ? 1 : 0;
    }

    // Check if a response packet is available
    int parsePacket() {
        return packet_available_;
    }

    // Read data from the received packet
    int read(uint8_t* buffer, size_t len) {
        size_t available = rx_len_ - read_pos_;
        size_t to_read = (len < available) ? len : available;
        if (to_read > 0) {
            memcpy(buffer, rx_buffer_ + read_pos_, to_read);
            read_pos_ += to_read;
        }
        return (int)to_read;
    }

    int read() {
        if (read_pos_ < rx_len_) {
            return rx_buffer_[read_pos_++];
        }
        return -1;
    }

    // Number of bytes available to read
    int available() {
        return (int)(rx_len_ - read_pos_);
    }

    // Test helpers to control mock behavior
    void setMockSendSuccess(bool success) { mock_send_success_ = success; }
    void setMockPacketAvailable(int size) { packet_available_ = size; }
    void setMockRxData(const uint8_t* data, size_t len) {
        size_t to_copy = (len < sizeof(rx_buffer_)) ? len : sizeof(rx_buffer_);
        memcpy(rx_buffer_, data, to_copy);
        rx_len_ = to_copy;
        read_pos_ = 0;
        packet_available_ = (int)to_copy;
    }
    const uint8_t* getMockTxData() const { return tx_buffer_; }
    size_t getMockTxLen() const { return tx_len_; }

private:
    uint16_t local_port_;
    IPAddress remote_ip_;
    uint16_t remote_port_ = 0;
    int packet_available_;
    bool mock_send_success_ = true;

    uint8_t rx_buffer_[256];
    size_t rx_len_;
    size_t read_pos_;

    uint8_t tx_buffer_[256];
    size_t tx_len_ = 0;
};

#endif
