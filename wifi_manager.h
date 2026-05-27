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
