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
#include "sensor_poller.h"
#include <Adafruit_AHTX0.h>

// Module-level static state
static Adafruit_AHTX0 aht;
static bool initialized = false;

bool sensorPoller_init() {
  initialized = aht.begin();
  return initialized;
}

bool sensorPoller_read(float& temp_c, float& humidity_pct) {
  if (!initialized) {
    return false;
  }

  sensors_event_t tempEvent;
  sensors_event_t humidityEvent;

  aht.getTemperatureSensor()->getEvent(&tempEvent);
  aht.getHumiditySensor()->getEvent(&humidityEvent);

  temp_c = tempEvent.data.temperature;
  humidity_pct = humidityEvent.data.relative_humidity;

  return true;
}
