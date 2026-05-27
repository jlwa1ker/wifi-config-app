#include "wifi_manager.h"
#include "config.h"

// Flag to track if WiFi pins have been configured
static bool pinsConfigured = false;

static void ensurePinsConfigured() {
  if (!pinsConfigured) {
    // Adafruit Feather M0 WiFi ATWINC1500 pin configuration:
    // CS=8, IRQ=7, RST=4, EN=2
    WiFi.setPins(8, 7, 4, 2);
    pinsConfigured = true;
  }
}

bool wifiManager_startAP() {
  ensurePinsConfigured();
  // Disconnect any active STA connection without killing the radio
  WiFi.disconnect();
  delay(200);
  // Create an open (no encryption) access point with the configured SSID
  int status = WiFi.beginAP(AP_SSID);
  return (status == WL_AP_LISTENING);
}

void wifiManager_stopAP() {
  // On WiFi101/ATWINC1500, calling WiFi.begin() for STA mode
  // implicitly stops the AP. We don't need to call WiFi.end()
  // as that would kill the radio entirely.
  // This function is now a no-op; the transition happens in connect().
}

bool wifiManager_connect(const char* ssid, const char* password) {
  ensurePinsConfigured();
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start >= WIFI_CONNECT_TIMEOUT_MS) {
      WiFi.disconnect();
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
