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
#ifndef REPORTER_H
#define REPORTER_H

#include "include/reporter_logic.h"
#include "server_reporter.h"

// What happened during a reporter_tick() call. The sketch uses this to update
// the OLED and emit serial diagnostics; the reporter itself stays independent
// of the display.
enum ReporterEvent {
    REPORTER_EVENT_NONE,          // nothing happened this tick
    REPORTER_EVENT_SKIPPED,       // report slot reached but running average not ready
    REPORTER_EVENT_UPLOAD_OK,     // upload succeeded
    REPORTER_EVENT_UPLOAD_FAILED  // upload attempted but failed
};

struct ReporterOutcome {
    ReporterEvent event;
    int removedCount;        // readings removed from cache (UPLOAD_OK)
    int pendingCount;        // readings remaining in cache after the tick
    ReportResult result;     // transport result (valid for UPLOAD_* events)
};

// Initialize the reporting state machine, aligning the first report to the next
// clock boundary. Requires a valid NTP epoch (call after the boot sync).
void reporter_begin(unsigned long epochNow);

// Advance the reporting state machine. Call once per loop() in STA mode.
// Non-blocking with respect to timing; the actual upload inside
// serverReporter_send() performs blocking network I/O when a report fires,
// which happens at most once per reporting interval.
ReporterOutcome reporter_tick();

// Current reporting state (for status/logging).
ReporterState reporter_state();

// Readings currently pending in the cache.
int reporter_pendingCount();

#endif
