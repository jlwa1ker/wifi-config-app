#ifndef REPORTER_LOGIC_H
#define REPORTER_LOGIC_H

/**
 * Pure, non-blocking state-machine logic for the reading-reporting cadence.
 *
 * This header contains all decision logic inline with no Arduino dependencies,
 * so it can be unit-tested on the host and compiled into the firmware. The
 * Arduino module (reporter.cpp) owns the I/O (sensor, cache, HTTP, millis())
 * and drives this state machine.
 *
 * Design: wifi-config-app issue #9 — reporting cadence is separated from NTP
 * time management so a stalled NTP resync cannot block or starve uploads.
 * Reporting depends only on a still-valid clock offset for timestamps, not on
 * NTP being currently healthy.
 *
 * States:
 *   REPORTER_WAIT_INTERVAL  counting down to the next clock-aligned report slot
 *   REPORTER_SNAPSHOT        snapshot the running average, timestamp, cache it
 *   REPORTER_SENDING         attempt the upload; apply success/failure outcome
 */

// Reporting cadence (15 minutes). Mirrors REPORT_INTERVAL_MS in config.h; kept
// here so the pure logic is self-contained and host-testable.
#define REPORTER_INTERVAL_MS 900000UL

enum ReporterState {
    REPORTER_WAIT_INTERVAL,
    REPORTER_SNAPSHOT,
    REPORTER_SENDING
};

struct ReporterStateData {
    ReporterState state;
    unsigned long lastReportMillis;  // reference point for the interval check
};

/**
 * Compute the delay, in milliseconds, from the current epoch to the next
 * clock-aligned reporting slot.
 *
 * Reports are aligned to wall-clock boundaries that are multiples of the
 * reporting interval since midnight UTC (e.g. :00, :15, :30, :45 for a 15-minute
 * interval). If the current time is exactly on a boundary, the delay is zero.
 *
 * Extracted from the alignment logic previously inlined in setup().
 *
 * @param epoch         Current Unix epoch seconds (UTC)
 * @param intervalMs    Reporting interval in milliseconds
 * @return milliseconds until the next aligned slot (0 if on a boundary)
 */
inline unsigned long reporter_alignmentDelayMs(unsigned long epoch, unsigned long intervalMs) {
    unsigned long secondsSinceMidnight = epoch % 86400UL;
    unsigned long minutesSinceMidnight = secondsSinceMidnight / 60UL;
    unsigned long frequencyMinutes = intervalMs / 60000UL;
    if (frequencyMinutes == 0) {
        return 0;
    }
    unsigned long remainder = minutesSinceMidnight % frequencyMinutes;
    if (remainder == 0) {
        return 0;
    }
    return (frequencyMinutes - remainder) * 60000UL;
}

/**
 * Initialize the reporter to WAIT_INTERVAL, aligning the first report to the
 * next clock boundary. lastReportMillis is back-dated so that the interval
 * check (now - lastReportMillis >= interval) first fires at the aligned time.
 *
 * @param s        Reporter state to initialize
 * @param now_ms   Current millis()
 * @param epoch    Current Unix epoch seconds (UTC), from a valid NTP sync
 */
inline void reporter_init(ReporterStateData& s, unsigned long now_ms, unsigned long epoch) {
    unsigned long delayMs = reporter_alignmentDelayMs(epoch, REPORTER_INTERVAL_MS);
    s.state = REPORTER_WAIT_INTERVAL;
    // Back-date so the first interval boundary lands delayMs in the future.
    s.lastReportMillis = now_ms - (REPORTER_INTERVAL_MS - delayMs);
}

/**
 * Whether the reporting interval has elapsed and a new report is due.
 */
inline bool reporter_intervalElapsed(const ReporterStateData& s, unsigned long now_ms) {
    return (now_ms - s.lastReportMillis) >= REPORTER_INTERVAL_MS;
}

/**
 * Transition WAIT_INTERVAL -> SNAPSHOT when the interval has elapsed. Resets the
 * interval reference point so the next slot is measured from this report.
 */
inline void reporter_beginReport(ReporterStateData& s, unsigned long now_ms) {
    s.lastReportMillis = now_ms;
    s.state = REPORTER_SNAPSHOT;
}

/**
 * Transition SNAPSHOT -> SENDING after a reading has been cached.
 */
inline void reporter_snapshotDone(ReporterStateData& s) {
    s.state = REPORTER_SENDING;
}

/**
 * Transition SENDING -> WAIT_INTERVAL after an upload attempt completes.
 * The outcome (which cached readings to remove) is applied by the caller; the
 * state machine returns to waiting either way so reporting continues.
 */
inline void reporter_sendDone(ReporterStateData& s) {
    s.state = REPORTER_WAIT_INTERVAL;
}

#endif // REPORTER_LOGIC_H
