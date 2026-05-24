/*
 * wifi-config-app.ino
 *
 * Main sketch for the WiFi configuration application on the
 * Adafruit Feather M0 (SAMD21). Implements a two-state machine:
 *   - AP mode: serves a credential entry form
 *   - STA mode: connects to WiFi and serves a health landing page
 */

#include "config.h"
#include "credential_store.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "health_client.h"

// Application state machine
enum AppState {
  STATE_AP_MODE,
  STATE_STA_MODE
};

AppState currentState;

void setup() {
  // Initialize serial for debug output
  Serial.begin(115200);
  delay(1000); // Brief delay for serial monitor to connect

  Serial.println("wifi-config-app: booting...");

  // Initialize the credential store (reads current state from flash)
  credentialStore_init();

  if (!credentialStore_hasCredentials()) {
    // No stored credentials — enter AP mode for configuration
    Serial.println("No credentials found. Starting AP mode.");
    wifiManager_startAP();
    webServer_init(WEB_SERVER_PORT);
    webServer_setMode(MODE_CONFIG_FORM);
    currentState = STATE_AP_MODE;
  } else {
    // Credentials exist — attempt to connect to the stored network
    WiFiCredentials creds;
    credentialStore_read(creds);

    Serial.print("Credentials found. Connecting to: ");
    Serial.println(creds.ssid);

    if (wifiManager_connect(creds.ssid, creds.password)) {
      // Connection succeeded — enter STA mode
      Serial.print("Connected! IP: ");
      Serial.println(wifiManager_getIP());

      webServer_init(WEB_SERVER_PORT);

      // Fetch health data from the remote endpoint
      HealthResult health = healthClient_fetch();
      if (health.success) {
        webServer_setHealthData(health.status, health.version, false, false);
      } else {
        // Check if we have cached data from a previous successful fetch
        char cachedStatus[32];
        char cachedVersion[32];
        if (healthClient_getCached(cachedStatus, cachedVersion)) {
          webServer_setHealthData(cachedStatus, cachedVersion, true, true);
        } else {
          webServer_setHealthData("", "", false, true);
        }
      }

      webServer_setMode(MODE_LANDING_PAGE);
      currentState = STATE_STA_MODE;
    } else {
      // Connection failed — clear credentials and fall back to AP mode
      Serial.println("Connection failed. Clearing credentials and starting AP.");
      credentialStore_clear();
      wifiManager_startAP();
      webServer_init(WEB_SERVER_PORT);
      webServer_setMode(MODE_CONFIG_FORM);
      currentState = STATE_AP_MODE;
    }
  }

  Serial.print("Boot complete. State: ");
  Serial.println(currentState == STATE_AP_MODE ? "AP_MODE" : "STA_MODE");
}

void loop() {
  // Poll the web server for incoming HTTP requests
  webServer_poll();

  if (currentState == STATE_AP_MODE) {
    if (webServer_hasSubmission()) {
      char ssid[MAX_SSID_LENGTH + 1];
      char password[MAX_PASS_LENGTH + 1];
      webServer_getSubmission(ssid, password);

      Serial.print("Attempting connection to: ");
      Serial.println(ssid);

      if (wifiManager_connect(ssid, password)) {
        // Connection succeeded
        Serial.print("Connected! IP: ");
        Serial.println(wifiManager_getIP());

        // Write credentials to flash
        if (credentialStore_write(ssid, password)) {
          // Success - transition to STA mode
          wifiManager_stopAP();

          // Fetch health data
          HealthResult health = healthClient_fetch();
          if (health.success) {
            webServer_setHealthData(health.status, health.version, false, false);
          } else {
            char cachedStatus[32];
            char cachedVersion[32];
            if (healthClient_getCached(cachedStatus, cachedVersion)) {
              webServer_setHealthData(cachedStatus, cachedVersion, true, true);
            } else {
              webServer_setHealthData("", "", false, true);
            }
          }

          webServer_setMode(MODE_LANDING_PAGE);
          webServer_setError(NULL); // Clear any previous error
          currentState = STATE_STA_MODE;
        } else {
          // Credential store write failed (Requirement 5.2)
          Serial.println("Failed to write credentials to flash!");
          webServer_setError("Failed to save credentials. Please try again.");
          wifiManager_disconnect();
          // Remain in AP mode - restart AP since we disconnected
          wifiManager_startAP();
        }
      } else {
        // Connection failed or timed out (Requirements 4.1, 4.2, 4.3, 4.4)
        Serial.println("Connection failed or timed out.");
        webServer_setError("Connection failed. Please check credentials and try again.");
        // Remain in AP mode serving config form
        // Restart AP since WiFi.begin() was called during connection attempt
        wifiManager_startAP();
      }
    }
  } else if (currentState == STATE_STA_MODE) {
    // After serving a landing page request, fetch fresh health data
    // for the next request. The first request uses data fetched during setup().
    if (webServer_wasRequestServed()) {
      HealthResult health = healthClient_fetch();
      if (health.success) {
        webServer_setHealthData(health.status, health.version, false, false);
      } else {
        // Health fetch failed — use cached data if available
        char cachedStatus[32];
        char cachedVersion[32];
        if (healthClient_getCached(cachedStatus, cachedVersion)) {
          webServer_setHealthData(cachedStatus, cachedVersion, true, true);
        } else {
          webServer_setHealthData("", "", false, true);
        }
      }
    }
  }
}
