#include "wifi_manager.h"
#include "config.h"

bool wifiManager_startAP() {
  // Create an open (no encryption) access point with the configured SSID
  int status = WiFi.beginAP(AP_SSID);
  return (status == WL_AP_LISTENING);
}

void wifiManager_stopAP() {
  WiFi.end();
}

bool wifiManager_connect(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start >= WIFI_CONNECT_TIMEOUT_MS) {
      return false;
    }
    delay(100);
  }
  return true;
}

void wifiManager_disconnect() {
  WiFi.disconnect();
}

IPAddress wifiManager_getIP() {
  return WiFi.localIP();
}

bool wifiManager_isConnected() {
  return (WiFi.status() == WL_CONNECTED);
}
