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
#ifndef READING_CACHE_H
#define READING_CACHE_H

#define READING_CACHE_MAX_SIZE 96

struct CachedReading {
    char timestamp[26];    // ISO 8601 UTC string (e.g., "2024-01-15T10:30:00+00:00")
    float temperature_f;   // Fahrenheit, rounded to 1 decimal
    float humidity_pct;    // Relative humidity percentage
    bool occupied;         // Slot is in use
};

// Initialize the reading cache (clear all slots)
void readingCache_init();

// Add a new reading to the cache. If full, overwrites the oldest entry.
void readingCache_add(const char* timestamp, float temperature_f, float humidity_pct);

// Get the number of readings currently in the cache
int readingCache_count();

// Get a pointer to the internal readings array for iteration.
// outCount receives the number of valid entries.
// outHead receives the index of the oldest entry.
// Iterate in FIFO order: for (int i = 0; i < outCount; i++) { int idx = (outHead + i) % READING_CACHE_MAX_SIZE; }
const CachedReading* readingCache_getAll(int& outCount, int& outHead);

// Remove the oldest N readings from the cache (after successful transmission)
void readingCache_removeOldest(int n);

// Clear all readings from the cache
void readingCache_clear();

#endif
