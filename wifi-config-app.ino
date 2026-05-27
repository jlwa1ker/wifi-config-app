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

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Helper to show a message on the OLED
void oledMsg(const char* line1, const char* line2 = "", const char* line3 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(line1);
  if (line2[0]) display.println(line2);
  if (line3[0]) display.println(line3);
  display.display();
}

// Application state machine
enum AppState {
  STATE_AP_MODE,
  STATE_STA_MODE
};

AppState currentState;

void setup() {
  // Initialize serial for debug output
  Serial.begin(115200);
  // Wait for serial port to connect (native USB) with 5-second timeout
  unsigned long serialStart = millis();
  while (!Serial && millis() - serialStart < 5000) {
    delay(10);
  }

  // Initialize OLED display
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed");
  }
  oledMsg("Booting...");

  Serial.println("wifi-config-app: booting...");

  // Initialize the credential store (reads current state from flash)
  credentialStore_init();

  // Log credential state for debugging
  Serial.print("Credential store valid flag: ");
  Serial.println(credentialStore_hasCredentials() ? "true" : "false");

  if (!credentialStore_hasCredentials()) {
    // No stored credentials — enter AP mode for configuration
    Serial.println("No credentials found. Starting AP mode.");
    oledMsg("No creds", "Starting AP...");
    bool apResult = wifiManager_startAP();
    Serial.print("AP start result: ");
    Serial.println(apResult ? "SUCCESS" : "FAILED");
    if (apResult) {
      oledMsg("AP Mode", "SSID: tempmon", "http://192.168.1.1");
    } else {
      oledMsg("AP FAILED!");
    }
    webServer_init(WEB_SERVER_PORT);
    webServer_setMode(MODE_CONFIG_FORM);
    currentState = STATE_AP_MODE;
  } else {
    // Credentials exist — attempt to connect to the stored network
    WiFiCredentials creds;
    credentialStore_read(creds);

    Serial.print("Credentials found. Connecting to: ");
    Serial.println(creds.ssid);
    oledMsg("Creds found", creds.ssid, "Connecting...");

    if (wifiManager_connect(creds.ssid, creds.password)) {
      // Connection succeeded — enter STA mode
      Serial.print("Connected! IP: ");
      Serial.println(wifiManager_getIP());

      char ipStr[20];
      IPAddress ip = wifiManager_getIP();
      snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
      oledMsg("Connected!", ipStr, "Starting server...");

      webServer_init(WEB_SERVER_PORT);

      // Set initial landing page data (health will be fetched on first request)
      webServer_setHealthData("", "", false, true);
      webServer_setMode(MODE_LANDING_PAGE);
      currentState = STATE_STA_MODE;

      oledMsg("STA Mode", ipStr, "Server ready");
    } else {
      // Connection failed — clear credentials and reboot into AP mode
      // (WiFi101 cannot transition STA→AP without a hardware reset)
      Serial.println("Connection failed. Clearing credentials and rebooting...");
      oledMsg("Connect FAILED", creds.ssid, "Clearing & reboot");
      credentialStore_clear();
      delay(2000);
      NVIC_SystemReset();
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

      Serial.print("Credentials received for: ");
      Serial.println(ssid);
      oledMsg("Creds received", ssid, "Saving & reboot...");

      // Save credentials to flash and reboot.
      // WiFi101 cannot do beginAP() then begin() in the same session,
      // so we save first and attempt connection on the next boot.
      if (credentialStore_write(ssid, password)) {
        Serial.println("Credentials saved. Rebooting to connect...");
        delay(500);
        NVIC_SystemReset();
      } else {
        Serial.println("Failed to write credentials to flash!");
        webServer_setError("Failed to save credentials. Please try again.");
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
