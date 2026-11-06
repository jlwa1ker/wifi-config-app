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
 */
#include "reporter.h"
#include <Arduino.h>
#include "running_average.h"
#include "reading_cache.h"
#include "ntp_client.h"
#include "server_reporter.h"
#include "include/reporter_logic.h"
#include "include/temperature_convert.h"

// Module-level reporting cadence state (pure logic in include/reporter_logic.h).
static ReporterStateData fsm;

void reporter_begin(unsigned long epochNow) {
  reporter_init(fsm, millis(), epochNow);
}

ReporterState reporter_state() {
  return fsm.state;
}

int reporter_pendingCount() {
  return readingCache_count();
}

ReporterOutcome reporter_tick() {
  ReporterOutcome outcome;
  outcome.event = REPORTER_EVENT_NONE;
  outcome.removedCount = 0;
  outcome.pendingCount = readingCache_count();
  outcome.result = REPORT_SUCCESS;

  // Only act when a reporting slot is reached.
  if (fsm.state != REPORTER_WAIT_INTERVAL) {
    return outcome;
  }
  if (!reporter_intervalElapsed(fsm, millis())) {
    return outcome;
  }

  // Slot reached: begin the report cycle.
  reporter_beginReport(fsm, millis());

  // SNAPSHOT: require a valid running average and a usable clock offset.
  float avg_temp_c, avg_humidity_pct;
  if (!ntpClient_timeValid() || !runningAverage_get(avg_temp_c, avg_humidity_pct)) {
    // Not enough samples (or no time): skip this cycle, return to waiting.
    reporter_sendDone(fsm);
    outcome.event = REPORTER_EVENT_SKIPPED;
    outcome.pendingCount = readingCache_count();
    return outcome;
  }

  float temp_f = celsiusToFahrenheit(avg_temp_c);
  char timestamp[26];
  ntpClient_formatISO8601(timestamp, sizeof(timestamp));
  readingCache_add(timestamp, temp_f, avg_humidity_pct);
  reporter_snapshotDone(fsm);

  // SENDING: attempt to upload all cached readings.
  int removalCount = 0;
  ReportResult result = serverReporter_send(removalCount);
  reporter_sendDone(fsm);

  outcome.result = result;
  if (result == REPORT_SUCCESS) {
    readingCache_removeOldest(removalCount);
    outcome.event = REPORTER_EVENT_UPLOAD_OK;
    outcome.removedCount = removalCount;
  } else {
    // Retain readings in cache for the next attempt.
    outcome.event = REPORTER_EVENT_UPLOAD_FAILED;
  }
  outcome.pendingCount = readingCache_count();
  return outcome;
}
