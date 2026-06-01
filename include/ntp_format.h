#ifndef NTP_FORMAT_H
#define NTP_FORMAT_H

#include <cstdint>
#include <cstdio>

/**
 * Format a Unix epoch timestamp as an ISO 8601 UTC string.
 *
 * Converts seconds since 1970-01-01T00:00:00Z into the format:
 *   YYYY-MM-DDTHH:MM:SS+00:00
 *
 * The output buffer must be at least 26 bytes (25 characters + null terminator).
 *
 * Extracted as pure logic for host-based testing. The Arduino module
 * (ntp_client.cpp) will include this header for the actual formatting.
 *
 * @param epoch       Seconds since 1970-01-01 00:00:00 UTC
 * @param buffer      Output buffer (must be at least 26 bytes)
 * @param bufferSize  Size of the output buffer
 */
static inline void formatEpochISO8601(uint32_t epoch, char* buffer, int bufferSize) {
    if (bufferSize < 26) {
        if (bufferSize > 0) {
            buffer[0] = '\0';
        }
        return;
    }

    // Extract time components
    uint32_t remaining = epoch;

    int second = remaining % 60;
    remaining /= 60;

    int minute = remaining % 60;
    remaining /= 60;

    int hour = remaining % 24;
    remaining /= 24;

    // remaining is now days since 1970-01-01
    uint32_t days = remaining;

    // Compute year from days since epoch
    int year = 1970;
    while (true) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        int daysInYear = leap ? 366 : 365;
        if (days < (uint32_t)daysInYear) {
            break;
        }
        days -= daysInYear;
        year++;
    }

    // Compute month and day from remaining days within the year
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (leap) {
        daysInMonth[1] = 29;
    }

    int month = 0;
    while (month < 12 && days >= (uint32_t)daysInMonth[month]) {
        days -= daysInMonth[month];
        month++;
    }

    int day = (int)days + 1;   // days are 0-indexed, calendar days are 1-indexed
    month += 1;                // months are 0-indexed above, calendar months are 1-indexed

    snprintf(buffer, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d+00:00",
             year, month, day, hour, minute, second);
}

#endif // NTP_FORMAT_H
