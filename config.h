#ifndef CONFIG_H
#define CONFIG_H

// Access Point settings
const char* const AP_SSID = "tempmon";
const int WEB_SERVER_PORT = 80;

// WiFi connection
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000;

// Health endpoint
const char* const HEALTH_HOST = "tempmon2-alb-150754285.us-east-1.elb.amazonaws.com";
const char* const HEALTH_PATH = "/health";
const int HEALTH_PORT = 80;
const int HEALTH_MAX_RETRIES = 3;
const unsigned long HEALTH_RETRY_DELAY_MS = 5000;

// Credential storage
const int MAX_SSID_LENGTH = 32;
const int MAX_PASS_LENGTH = 64;

#endif
