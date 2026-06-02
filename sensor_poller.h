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
#ifndef SENSOR_POLLER_H
#define SENSOR_POLLER_H

// Initialize the AHT20 sensor. Returns true if sensor detected on I2C bus.
bool sensorPoller_init();

// Read the sensor. Returns true if a valid reading was obtained.
// On success, populates temp_c and humidity_pct.
// On failure, outputs are unchanged and the sample is discarded.
bool sensorPoller_read(float& temp_c, float& humidity_pct);

#endif
