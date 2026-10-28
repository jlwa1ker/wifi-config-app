#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>

#include "include/ntp_sync_logic.h"

/*
 * Tests for the NTP resync state machine (issue #9).
 *
 * The regression: a failed resync left the "needs resync" condition true,
 * causing a tight busy-loop and starving reporting. The state machine must
 * instead back off to a short retry interval after a failure and use the
 * normal 24-hour interval after a success — never becoming immediately-due
 * again right after a failure.
 */

TEST_CASE("init starts waiting to sync", "[ntp]") {
    NtpSyncState s;
    ntpSync_init(s);
    REQUIRE(s.state == NTP_WAITING_TO_SYNC);
    REQUIRE(s.everSynced == false);
    REQUIRE(s.lastAttemptFailed == false);
}

TEST_CASE("success returns to IDLE with the 24-hour interval", "[ntp]") {
    NtpSyncState s;
    ntpSync_init(s);

    ntpSync_onRequestSent(s, 1000);
    REQUIRE(s.state == NTP_AWAITING_RESPONSE);

    ntpSync_onSuccess(s, 1500);
    REQUIRE(s.state == NTP_IDLE);
    REQUIRE(s.everSynced == true);
    REQUIRE(s.lastAttemptFailed == false);
    REQUIRE(ntpSync_dueInterval(s) == NTP_RESYNC_INTERVAL_MS);

    // Not due until 24 hours elapse.
    REQUIRE(ntpSync_isDue(s, 1500 + NTP_RESYNC_INTERVAL_MS - 1) == false);
    REQUIRE(ntpSync_isDue(s, 1500 + NTP_RESYNC_INTERVAL_MS) == true);
}

TEST_CASE("failure backs off and is NOT immediately due (issue #9)", "[ntp]") {
    NtpSyncState s;
    ntpSync_init(s);

    ntpSync_onRequestSent(s, 1000);
    ntpSync_onFailure(s, 3000);

    // Must route to BACKOFF, not back to a due state.
    REQUIRE(s.state == NTP_BACKOFF);
    REQUIRE(s.lastAttemptFailed == true);
    REQUIRE(ntpSync_dueInterval(s) == NTP_RESYNC_RETRY_MS);

    // Critically: immediately after failure it is NOT due again (no busy-loop).
    REQUIRE(ntpSync_isDue(s, 3000) == false);
    ntpSync_advanceTimers(s, 3000);
    REQUIRE(s.state == NTP_BACKOFF);

    // Becomes due only after the 5-minute retry interval.
    REQUIRE(ntpSync_isDue(s, 3000 + NTP_RESYNC_RETRY_MS - 1) == false);
    REQUIRE(ntpSync_isDue(s, 3000 + NTP_RESYNC_RETRY_MS) == true);
}

TEST_CASE("backoff transitions to waiting-to-sync when retry is due", "[ntp]") {
    NtpSyncState s;
    ntpSync_init(s);
    ntpSync_onRequestSent(s, 1000);
    ntpSync_onFailure(s, 2000);

    ntpSync_advanceTimers(s, 2000 + NTP_RESYNC_RETRY_MS);
    REQUIRE(s.state == NTP_WAITING_TO_SYNC);
}

TEST_CASE("idle transitions to waiting-to-sync when interval elapses", "[ntp]") {
    NtpSyncState s;
    ntpSync_init(s);
    ntpSync_onRequestSent(s, 0);
    ntpSync_onSuccess(s, 100);

    ntpSync_advanceTimers(s, 100 + NTP_RESYNC_INTERVAL_MS);
    REQUIRE(s.state == NTP_WAITING_TO_SYNC);
}

TEST_CASE("response timeout detection", "[ntp]") {
    NtpSyncState s;
    ntpSync_init(s);
    ntpSync_onRequestSent(s, 5000);

    REQUIRE(ntpSync_responseTimedOut(s, 5000 + NTP_RESPONSE_TIMEOUT_MS - 1) == false);
    REQUIRE(ntpSync_responseTimedOut(s, 5000 + NTP_RESPONSE_TIMEOUT_MS) == true);
}

/*
 * Property: after any failure, the next attempt is scheduled exactly
 * NTP_RESYNC_RETRY_MS later and never sooner — so repeated failures retry on a
 * bounded 5-minute cadence rather than busy-looping.
 */
TEST_CASE("Property: failure always schedules retry at the 5-minute cadence", "[ntp][property]") {
    rc::check("retry due only after NTP_RESYNC_RETRY_MS following a failure",
        [](unsigned int sentOffset, unsigned int failOffset, unsigned int probe) {
            NtpSyncState s;
            ntpSync_init(s);

            unsigned long sent = sentOffset;
            unsigned long fail = sent + (failOffset % NTP_RESPONSE_TIMEOUT_MS) + 1;

            ntpSync_onRequestSent(s, sent);
            ntpSync_onFailure(s, fail);

            RC_ASSERT(s.state == NTP_BACKOFF);

            // At the failure instant and any time before the retry window, not due.
            unsigned long early = fail + (probe % NTP_RESYNC_RETRY_MS);
            RC_ASSERT(ntpSync_isDue(s, early) == false);

            // Exactly at and after the retry window, due.
            RC_ASSERT(ntpSync_isDue(s, fail + NTP_RESYNC_RETRY_MS) == true);
        }
    );
}
