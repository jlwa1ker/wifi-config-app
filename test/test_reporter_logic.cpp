#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>

#include "include/reporter_logic.h"

/*
 * Tests for the reporting cadence state machine (issue #9).
 *
 * The reporter owns the 15-minute reporting rhythm and clock-slot alignment,
 * separated from NTP time management so a stalled resync cannot stall reporting.
 */

TEST_CASE("alignment delay is zero exactly on a boundary", "[reporter]") {
    // 15-minute interval; epoch at midnight UTC is on a boundary.
    REQUIRE(reporter_alignmentDelayMs(0, REPORTER_INTERVAL_MS) == 0);
    // 15 minutes past midnight is also on a boundary.
    REQUIRE(reporter_alignmentDelayMs(15UL * 60UL, REPORTER_INTERVAL_MS) == 0);
    // Exactly one hour past midnight.
    REQUIRE(reporter_alignmentDelayMs(3600UL, REPORTER_INTERVAL_MS) == 0);
}

TEST_CASE("alignment delay points to the next 15-minute slot", "[reporter]") {
    // 1 minute past midnight -> 14 minutes until the :15-aligned slot.
    REQUIRE(reporter_alignmentDelayMs(60UL, REPORTER_INTERVAL_MS) == 14UL * 60000UL);
    // 14 minutes past -> 1 minute remaining.
    REQUIRE(reporter_alignmentDelayMs(14UL * 60UL, REPORTER_INTERVAL_MS) == 1UL * 60000UL);
    // 16 minutes past -> 14 minutes until the :30 slot.
    REQUIRE(reporter_alignmentDelayMs(16UL * 60UL, REPORTER_INTERVAL_MS) == 14UL * 60000UL);
}

TEST_CASE("init aligns the first report to the next boundary", "[reporter]") {
    ReporterStateData s;
    // now = 1,000,000 ms; epoch 1 minute past a boundary -> 14 min to first slot.
    reporter_init(s, 1000000UL, 60UL);
    REQUIRE(s.state == REPORTER_WAIT_INTERVAL);

    unsigned long delay = reporter_alignmentDelayMs(60UL, REPORTER_INTERVAL_MS); // 14 min
    // The interval elapses exactly when now advances by `delay`.
    REQUIRE(reporter_intervalElapsed(s, 1000000UL + delay - 1) == false);
    REQUIRE(reporter_intervalElapsed(s, 1000000UL + delay) == true);
}

TEST_CASE("init on a boundary makes the first report due immediately", "[reporter]") {
    ReporterStateData s;
    reporter_init(s, 500000UL, 0UL); // epoch on a boundary -> delay 0
    REQUIRE(reporter_intervalElapsed(s, 500000UL) == true);
}

TEST_CASE("report lifecycle transitions WAIT -> SNAPSHOT -> SENDING -> WAIT", "[reporter]") {
    ReporterStateData s;
    reporter_init(s, 0UL, 0UL);
    REQUIRE(s.state == REPORTER_WAIT_INTERVAL);

    reporter_beginReport(s, 1000UL);
    REQUIRE(s.state == REPORTER_SNAPSHOT);

    reporter_snapshotDone(s);
    REQUIRE(s.state == REPORTER_SENDING);

    reporter_sendDone(s);
    REQUIRE(s.state == REPORTER_WAIT_INTERVAL);
}

TEST_CASE("beginReport resets the interval reference to the new report time", "[reporter]") {
    ReporterStateData s;
    reporter_init(s, 0UL, 0UL);

    unsigned long reportTime = 2000000UL;
    reporter_beginReport(s, reportTime);
    reporter_snapshotDone(s);
    reporter_sendDone(s);

    // Next slot is measured from reportTime, so it fires one full interval later.
    REQUIRE(reporter_intervalElapsed(s, reportTime + REPORTER_INTERVAL_MS - 1) == false);
    REQUIRE(reporter_intervalElapsed(s, reportTime + REPORTER_INTERVAL_MS) == true);
}

/*
 * Property: the computed alignment delay always lands the next report on a
 * clock boundary (a multiple of the interval since midnight) and is always
 * strictly less than one full interval.
 */
TEST_CASE("Property: alignment delay yields a boundary-aligned next report", "[reporter][property]") {
    rc::check("aligned epoch+delay is a multiple of the interval; delay < interval",
        [](unsigned int epochSeconds) {
            unsigned long epoch = epochSeconds;
            unsigned long delayMs = reporter_alignmentDelayMs(epoch, REPORTER_INTERVAL_MS);
            unsigned long intervalSec = REPORTER_INTERVAL_MS / 1000UL;

            RC_ASSERT(delayMs < REPORTER_INTERVAL_MS);
            RC_ASSERT(delayMs % 60000UL == 0); // whole minutes

            unsigned long alignedEpoch = epoch + (delayMs / 1000UL);
            unsigned long minutesSinceMidnight = (alignedEpoch % 86400UL) / 60UL;
            unsigned long intervalMinutes = intervalSec / 60UL;
            RC_ASSERT(minutesSinceMidnight % intervalMinutes == 0);
        }
    );
}
