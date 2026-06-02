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
#ifndef RUNNING_AVERAGE_H
#define RUNNING_AVERAGE_H

// Initialize/reset the running average state
void runningAverage_init();

// Add a new sample. Automatically expires samples older than 60 seconds.
void runningAverage_addSample(float temp_c, float humidity_pct);

// Get the current average. Returns true if at least 10 samples exist in the window.
// On success, populates avg_temp_c and avg_humidity_pct.
bool runningAverage_get(float& avg_temp_c, float& avg_humidity_pct);

// Get the current sample count in the window
int runningAverage_sampleCount();

#endif
