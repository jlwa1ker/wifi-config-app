#ifndef NTP_SYNC_LOGIC_H
#define NTP_SYNC_LOGIC_H

/**
 * Pure, non-blocking state-machine logic for NTP time synchronization.
 *
 * This header contains all decision logic inline with no Arduino dependencies,
 * so it can be unit-tested on the host and compiled into the firmware. The
 * Arduino module (ntp_client.cpp) owns the I/O (UDP sockets, millis()) and
 * drives this state machine.
 *
 * Design: wifi-config-app issue #9 — a failed 24-hour resync must not busy-loop
 * or block reporting. Failures back off to a short retry interval; the clock
 * offset obtained by the last successful sync remains valid throughout.
 *
 * States:
 *   NTP_IDLE             synced and within the normal interval; nothing to do
 *   NTP_WAITING_TO_SYNC  a sync is due; the next tick should send a request
 *   NTP_AWAITING_RESPONSE request sent; polling for a reply until the deadline
 *   NTP_BACKOFF          last attempt failed; waiting out the retry interval
 */

// Normal cadence between successful syncs (24 hours).
#define NTP_RESYNC_INTERVAL_MS 86400000UL
// Shorter retry cadence after a failed sync (5 minutes).
#define NTP_RESYNC_RETRY_MS 300000UL
// How long to wait for a reply to a single request before declaring failure.
#define NTP_RESPONSE_TIMEOUT_MS 2000UL

enum NtpState {
    NTP_IDLE,
    NTP_WAITING_TO_SYNC,
    NTP_AWAITING_RESPONSE,
    NTP_BACKOFF
};

struct NtpSyncState {
    NtpState state;
    bool everSynced;            // true once at least one sync has succeeded
    bool lastAttemptFailed;     // true if the most recent completed attempt failed
    unsigned long lastAttemptMillis;  // millis() when the last attempt completed
    unsigned long requestSentMillis;  // millis() when the in-flight request was sent
};

/**
 * Initialize the NTP sync state. Starts in WAITING_TO_SYNC so the first tick
 * kicks off an initial sync.
 */
inline void ntpSync_init(NtpSyncState& s) {
    s.state = NTP_WAITING_TO_SYNC;
    s.everSynced = false;
    s.lastAttemptFailed = false;
    s.lastAttemptMillis = 0;
    s.requestSentMillis = 0;
}

/**
 * Return the interval that must elapse after the last attempt before the next
 * sync is due: the short retry interval after a failure, otherwise the normal
 * 24-hour interval.
 */
inline unsigned long ntpSync_dueInterval(const NtpSyncState& s) {
    return s.lastAttemptFailed ? NTP_RESYNC_RETRY_MS : NTP_RESYNC_INTERVAL_MS;
}

/**
 * Whether a new sync is due given the current time. Used while IDLE or in
 * BACKOFF to decide when to transition back to WAITING_TO_SYNC.
 */
inline bool ntpSync_isDue(const NtpSyncState& s, unsigned long now_ms) {
    return (now_ms - s.lastAttemptMillis) >= ntpSync_dueInterval(s);
}

/**
 * Whether the in-flight request has exceeded the response timeout.
 */
inline bool ntpSync_responseTimedOut(const NtpSyncState& s, unsigned long now_ms) {
    return (now_ms - s.requestSentMillis) >= NTP_RESPONSE_TIMEOUT_MS;
}

/**
 * Record that a request packet was just sent. Transitions to AWAITING_RESPONSE.
 */
inline void ntpSync_onRequestSent(NtpSyncState& s, unsigned long now_ms) {
    s.state = NTP_AWAITING_RESPONSE;
    s.requestSentMillis = now_ms;
}

/**
 * Record a successful sync. Clears the failure flag and returns to IDLE with
 * the normal 24-hour cadence.
 */
inline void ntpSync_onSuccess(NtpSyncState& s, unsigned long now_ms) {
    s.state = NTP_IDLE;
    s.everSynced = true;
    s.lastAttemptFailed = false;
    s.lastAttemptMillis = now_ms;
}

/**
 * Record a failed attempt. Routes to BACKOFF so the next attempt waits out the
 * short retry interval. The existing clock offset remains valid.
 */
inline void ntpSync_onFailure(NtpSyncState& s, unsigned long now_ms) {
    s.state = NTP_BACKOFF;
    s.lastAttemptFailed = true;
    s.lastAttemptMillis = now_ms;
}

/**
 * Advance timer-driven transitions that do not require I/O:
 *   IDLE    -> WAITING_TO_SYNC when the normal interval has elapsed
 *   BACKOFF -> WAITING_TO_SYNC when the retry interval has elapsed
 *
 * The AWAITING_RESPONSE timeout and the WAITING_TO_SYNC send are handled by the
 * caller (they require I/O), using ntpSync_responseTimedOut / ntpSync_onFailure
 * and ntpSync_onRequestSent respectively.
 */
inline void ntpSync_advanceTimers(NtpSyncState& s, unsigned long now_ms) {
    if (s.state == NTP_IDLE || s.state == NTP_BACKOFF) {
        if (ntpSync_isDue(s, now_ms)) {
            s.state = NTP_WAITING_TO_SYNC;
        }
    }
}

#endif // NTP_SYNC_LOGIC_H
