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
#include "reading_cache.h"
#include <string.h>

// Module-level static state
static CachedReading readings[READING_CACHE_MAX_SIZE];
static int head;   // Index of the oldest entry
static int count;  // Number of valid entries

void readingCache_init() {
    head = 0;
    count = 0;
    for (int i = 0; i < READING_CACHE_MAX_SIZE; i++) {
        readings[i].occupied = false;
        readings[i].timestamp[0] = '\0';
        readings[i].temperature_f = 0.0f;
        readings[i].humidity_pct = 0.0f;
    }
}

void readingCache_add(const char* timestamp, float temperature_f, float humidity_pct) {
    int insertIdx = (head + count) % READING_CACHE_MAX_SIZE;

    if (count == READING_CACHE_MAX_SIZE) {
        // Buffer is full — overwrite the oldest entry and advance head
        insertIdx = head;
        head = (head + 1) % READING_CACHE_MAX_SIZE;
    } else {
        count++;
    }

    strncpy(readings[insertIdx].timestamp, timestamp, sizeof(readings[insertIdx].timestamp) - 1);
    readings[insertIdx].timestamp[sizeof(readings[insertIdx].timestamp) - 1] = '\0';
    readings[insertIdx].temperature_f = temperature_f;
    readings[insertIdx].humidity_pct = humidity_pct;
    readings[insertIdx].occupied = true;
}

int readingCache_count() {
    return count;
}

const CachedReading* readingCache_getAll(int& outCount, int& outHead) {
    outCount = count;
    outHead = head;
    return readings;
}

void readingCache_removeOldest(int n) {
    if (n >= count) {
        // Remove all — reset to empty state
        for (int i = 0; i < count; i++) {
            int idx = (head + i) % READING_CACHE_MAX_SIZE;
            readings[idx].occupied = false;
        }
        head = 0;
        count = 0;
    } else {
        for (int i = 0; i < n; i++) {
            readings[head].occupied = false;
            head = (head + 1) % READING_CACHE_MAX_SIZE;
        }
        count -= n;
    }
}

void readingCache_clear() {
    for (int i = 0; i < READING_CACHE_MAX_SIZE; i++) {
        readings[i].occupied = false;
    }
    head = 0;
    count = 0;
}
