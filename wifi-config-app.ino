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
#include "sensor_poller.h"
#include "running_average.h"
#include "reading_cache.h"
#include "ntp_client.h"
#include "server_reporter.h"
#include "include/temperature_convert.h"

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

// Show current sensor averages and cache count (STA mode)
void oledShowReadings(float temp_f, float humidity_pct, int cacheCount) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  char line1[22];
  snprintf(line1, sizeof(line1), "Temp: %.1f F", (double)temp_f);
  display.println(line1);

  char line2[22];
  snprintf(line2, sizeof(line2), "Hum:  %.1f %%", (double)humidity_pct);
  display.println(line2);

  char line3[22];
  snprintf(line3, sizeof(line3), "Cache: %d readings", cacheCount);
  display.println(line3);

  display.display();
}

// Show upload success indicator
void oledShowUploadSuccess(int sentCount) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Upload OK!");

  char line2[22];
  snprintf(line2, sizeof(line2), "Sent: %d readings", sentCount);
  display.println(line2);

  display.display();
}

// Show upload failure indicator with pending cache count
void oledShowUploadFailed(int cacheCount) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Upload FAILED!");

  char line2[22];
  snprintf(line2, sizeof(line2), "Cached: %d pending", cacheCount);
  display.println(line2);

  display.display();
}

// Show NTP synchronization error
void oledShowNtpError() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("NTP Sync FAILED!");
  display.println("No time source.");
  display.println("Retrying...");
  display.display();
}

// Application state machine
enum AppState {
  STATE_AP_MODE,
  STATE_STA_MODE
};

AppState currentState;

// Flag indicating whether the AHT20 sensor initialized successfully.
// When false, the STA mode loop will not poll the sensor.
bool sensorReady = false;

// Flag indicating whether NTP synchronization succeeded at boot.
// When false, the reporting loop will not run (no valid timestamps).
bool ntpSynced = false;

void setup() {
  // Initialize serial for debug output
  Serial.begin(115200);
  delay(100);

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
      oledMsg("Connected!", ipStr, "Fetching health...");

      // Fetch health and display on OLED (no web server in STA mode)
      HealthResult health = healthClient_fetch();
      if (health.success) {
        char line3[32];
        snprintf(line3, sizeof(line3), "%s v%s", health.status, health.version);
        oledMsg("STA Mode", ipStr, line3);
      } else {
        oledMsg("STA Mode", ipStr, "Health: failed");
      }

      // Synchronize clock via NTP before starting sensor/reporting loop.
      // Readings require accurate timestamps, so NTP must succeed first.
      oledMsg("NTP Sync", "Contacting server...");
      if (ntpClient_sync()) {
        ntpSynced = true;
        Serial.println("NTP sync succeeded.");
      } else {
        ntpSynced = false;
        Serial.println("NTP sync FAILED after all retries.");
        oledShowNtpError();
      }

      // Only initialize hygrometer modules if NTP sync succeeded
      if (ntpSynced) {
        runningAverage_init();
        readingCache_init();

        if (sensorPoller_init()) {
          sensorReady = true;
          Serial.println("AHT20 sensor initialized.");
        } else {
          sensorReady = false;
          Serial.println("AHT20 sensor init FAILED!");
          oledMsg("Sensor FAILED!", "AHT20 not found", "No polling");
        }
      } // end if (ntpSynced)

      currentState = STATE_STA_MODE;
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
    // Periodically check WiFi status to keep module alive
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
      lastCheck = millis();
      int status = WiFi.status();
      if (status != WL_AP_LISTENING && status != WL_AP_CONNECTED) {
        // Module dropped AP mode — restart it
        Serial.println("AP dropped, restarting...");
        WiFi.beginAP(AP_SSID);
        webServer_init(WEB_SERVER_PORT);
      }
    }

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
    // Periodic NTP re-sync check (every 24 hours)
    if (ntpSynced && ntpClient_needsResync()) {
      Serial.println("NTP re-sync needed. Syncing...");
      if (ntpClient_sync()) {
        Serial.println("NTP re-sync succeeded.");
      } else {
        // Continue using existing time offset; retry at next opportunity
        Serial.println("NTP re-sync failed. Will retry later.");
      }
    }

    // Poll sensor once per second and feed into running average
    static unsigned long lastPollTime = 0;
    if (sensorReady && ntpSynced && (millis() - lastPollTime >= 1000)) {
      lastPollTime = millis();

      float temp_c, humidity_pct;
      if (sensorPoller_read(temp_c, humidity_pct)) {
        runningAverage_addSample(temp_c, humidity_pct);

        // Update OLED with current averages if enough samples
        float avg_temp_c, avg_humidity_pct;
        if (runningAverage_get(avg_temp_c, avg_humidity_pct)) {
          float avg_temp_f = celsiusToFahrenheit(avg_temp_c);
          oledShowReadings(avg_temp_f, avg_humidity_pct, readingCache_count());
        }
      }
    }

    // Every 15 minutes: snapshot running average, cache reading, and attempt server upload
    static unsigned long lastReportTime = 0;
    if (sensorReady && ntpSynced && (millis() - lastReportTime >= REPORT_INTERVAL_MS)) {
      lastReportTime = millis();

      float avg_temp_c, avg_humidity_pct;
      if (runningAverage_get(avg_temp_c, avg_humidity_pct)) {
        // Convert to Fahrenheit and get NTP timestamp
        float temp_f = celsiusToFahrenheit(avg_temp_c);
        char timestamp[26];
        ntpClient_formatISO8601(timestamp, sizeof(timestamp));

        // Store in reading cache
        readingCache_add(timestamp, temp_f, avg_humidity_pct);

        // Attempt to send all cached readings to server
        int removalCount = 0;
        ReportResult result = serverReporter_send(removalCount);

        if (result == REPORT_SUCCESS) {
          readingCache_removeOldest(removalCount);
          oledShowUploadSuccess(removalCount);
        } else {
          // Retain readings in cache for next attempt
          oledShowUploadFailed(readingCache_count());
        }
      }
      // If runningAverage_get() returns false (< 10 samples), skip this cycle
    }
  }
}
