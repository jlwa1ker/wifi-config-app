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
#include "running_average.h"
#include <Arduino.h>
#include "include/running_average_logic.h"

// Module-level static state
static RunningAverageState state;

void runningAverage_init() {
  runningAverage_init(state);
}

void runningAverage_addSample(float temp_c, float humidity_pct) {
  runningAverage_addSample(state, temp_c, humidity_pct, millis());
}

bool runningAverage_get(float& avg_temp_c, float& avg_humidity_pct) {
  return runningAverage_getAverage(state, avg_temp_c, avg_humidity_pct, millis());
}

int runningAverage_sampleCount() {
  return runningAverage_sampleCount(state, millis());
}
