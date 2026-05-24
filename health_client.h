#ifndef HEALTH_CLIENT_H
#define HEALTH_CLIENT_H

struct HealthResult {
  bool success;           // Whether the request succeeded and parsed correctly
  char status[32];       // Parsed "status" field
  char version[32];      // Parsed "version" field
  bool missingFields;    // True if JSON was valid but fields were missing
};

// Perform a health check with retries.
// Returns the result of the health check.
HealthResult healthClient_fetch();

// Get cached health data (from last successful fetch)
// Returns false if no cached data is available
bool healthClient_getCached(char* status, char* version);

#endif
