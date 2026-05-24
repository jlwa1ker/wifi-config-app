#ifndef URL_DECODE_H
#define URL_DECODE_H

#include <cstdlib>
#include <cstring>

/**
 * URL-decode a source string into a destination buffer.
 *
 * Decodes %XX hex sequences and '+' (as space) per application/x-www-form-urlencoded.
 * Output is null-terminated and truncated if dstSize is insufficient.
 *
 * Extracted from web_server.cpp for host-side testing.
 */
static void urlDecode(const char* src, char* dst, int dstSize) {
    int di = 0;
    int si = 0;
    while (src[si] != '\0' && di < dstSize - 1) {
        if (src[si] == '%' && src[si + 1] != '\0' && src[si + 2] != '\0') {
            // Decode %XX hex sequence
            char hex[3] = { src[si + 1], src[si + 2], '\0' };
            char* endPtr;
            long val = strtol(hex, &endPtr, 16);
            if (endPtr == hex + 2) {
                dst[di++] = (char)val;
                si += 3;
            } else {
                // Malformed hex - pass '%' through as-is
                dst[di++] = src[si++];
            }
        } else if (src[si] == '+') {
            // '+' represents a space in form-urlencoded data
            dst[di++] = ' ';
            si++;
        } else {
            // Regular character pass-through
            dst[di++] = src[si++];
        }
    }
    dst[di] = '\0';
}

/**
 * Parse a form-urlencoded body string, extracting "ssid" and "password" fields.
 *
 * Extracted from web_server.cpp for host-side testing.
 */
static void parseFormBody(const char* body, char* ssid, int ssidSize, char* password, int passSize) {
    // Initialize outputs
    ssid[0] = '\0';
    password[0] = '\0';

    // Parse "ssid=value&password=value" format
    const char* ptr = body;
    while (*ptr != '\0') {
        // Find the key
        const char* eqPos = strchr(ptr, '=');
        if (eqPos == NULL) {
            break;
        }

        int keyLen = eqPos - ptr;
        const char* valStart = eqPos + 1;

        // Find end of value (& or end of string)
        const char* ampPos = strchr(valStart, '&');
        int valLen;
        if (ampPos != NULL) {
            valLen = ampPos - valStart;
        } else {
            valLen = strlen(valStart);
        }

        // Extract the raw (still encoded) value into a temp buffer
        char rawValue[128];
        if (valLen >= (int)sizeof(rawValue)) {
            valLen = sizeof(rawValue) - 1;
        }
        strncpy(rawValue, valStart, valLen);
        rawValue[valLen] = '\0';

        // Match key and URL-decode value into the appropriate output
        if (keyLen == 4 && strncmp(ptr, "ssid", 4) == 0) {
            urlDecode(rawValue, ssid, ssidSize);
        } else if (keyLen == 8 && strncmp(ptr, "password", 8) == 0) {
            urlDecode(rawValue, password, passSize);
        }

        // Advance past this key=value pair
        if (ampPos != NULL) {
            ptr = ampPos + 1;
        } else {
            break;
        }
    }
}

#endif // URL_DECODE_H
