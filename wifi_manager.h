#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi101.h>

// Start the device in Access Point mode
// Returns true if AP was created successfully
bool wifiManager_startAP();

// Stop the Access Point
void wifiManager_stopAP();

// Attempt to connect to a WiFi network
// Returns true if connected within WIFI_CONNECT_TIMEOUT_MS
bool wifiManager_connect(const char* ssid, const char* password);

// Disconnect from the current WiFi network
void wifiManager_disconnect();

// Get the current IP address (valid in both AP and STA mode)
IPAddress wifiManager_getIP();

// Check if currently connected to a WiFi network (STA mode)
bool wifiManager_isConnected();

#endif
