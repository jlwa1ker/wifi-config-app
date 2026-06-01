#ifndef RUNNING_AVERAGE_LOGIC_H
#define RUNNING_AVERAGE_LOGIC_H

/**
 * Extracted pure logic for the running average module.
 * Maintains a 60-second sliding window of temperature and humidity samples.
 *
 * This header contains all logic inline so it can be included in test files
 * without linking issues. No Arduino dependencies.
 *
 * Requirements: 2.1, 2.2, 2.3, 2.5
 */

// Maximum samples in the window (1 per second x 60 seconds)
#define RUNNING_AVG_WINDOW_SIZE 60

// Minimum samples required before the average is considered valid
#define RUNNING_AVG_MIN_SAMPLES 10

// Window duration in milliseconds
#define RUNNING_AVG_WINDOW_MS 60000UL

struct AverageSample {
    float temp_c;
    float humidity_pct;
    unsigned long timestamp_ms;
};

struct RunningAverageState {
    AverageSample samples[RUNNING_AVG_WINDOW_SIZE];
    int head;   // Next write position
    int count;  // Number of valid samples currently in buffer
};

/**
 * Initialize/reset the running average state.
 */
inline void runningAverage_init(RunningAverageState& state) {
    state.head = 0;
    state.count = 0;
}

/**
 * Expire samples older than 60 seconds relative to the given current time.
 * Removes expired samples from the logical window by adjusting count.
 *
 * Since the circular buffer stores samples in insertion order, expired samples
 * are always at the tail (oldest entries). We scan from the tail forward and
 * discard any that fall outside the 60-second window.
 */
inline void runningAverage_expireOld(RunningAverageState& state, unsigned long now_ms) {
    while (state.count > 0) {
        // Tail index: the oldest sample in the buffer
        int tail = (state.head - state.count + RUNNING_AVG_WINDOW_SIZE) % RUNNING_AVG_WINDOW_SIZE;
        unsigned long age = now_ms - state.samples[tail].timestamp_ms;
        if (age > RUNNING_AVG_WINDOW_MS) {
            state.count--;
        } else {
            break;
        }
    }
}

/**
 * Add a new sample to the running average window.
 * Automatically expires samples older than 60 seconds before adding.
 *
 * @param state       The running average state
 * @param temp_c      Temperature in Celsius
 * @param humidity_pct Relative humidity percentage
 * @param now_ms      Current timestamp in milliseconds (e.g., millis())
 */
inline void runningAverage_addSample(RunningAverageState& state, float temp_c, float humidity_pct, unsigned long now_ms) {
    // Expire old samples first
    runningAverage_expireOld(state, now_ms);

    // Write new sample at head position
    state.samples[state.head].temp_c = temp_c;
    state.samples[state.head].humidity_pct = humidity_pct;
    state.samples[state.head].timestamp_ms = now_ms;

    // Advance head (circular)
    state.head = (state.head + 1) % RUNNING_AVG_WINDOW_SIZE;

    // Increase count, capped at window size
    if (state.count < RUNNING_AVG_WINDOW_SIZE) {
        state.count++;
    }
}

/**
 * Get the current average of samples in the window.
 * Returns false if fewer than RUNNING_AVG_MIN_SAMPLES exist in the window.
 * On success, populates avg_temp_c and avg_humidity_pct with the arithmetic mean.
 *
 * @param state          The running average state
 * @param avg_temp_c     Output: average temperature in Celsius
 * @param avg_humidity_pct Output: average relative humidity percentage
 * @param now_ms         Current timestamp for expiration check
 * @return true if average is valid (>= 10 samples), false otherwise
 */
inline bool runningAverage_getAverage(RunningAverageState& state, float& avg_temp_c, float& avg_humidity_pct, unsigned long now_ms) {
    // Expire old samples before computing average
    runningAverage_expireOld(state, now_ms);

    if (state.count < RUNNING_AVG_MIN_SAMPLES) {
        return false;
    }

    float sum_temp = 0.0f;
    float sum_humidity = 0.0f;

    // Iterate over all valid samples in the buffer
    for (int i = 0; i < state.count; i++) {
        int idx = (state.head - state.count + i + RUNNING_AVG_WINDOW_SIZE) % RUNNING_AVG_WINDOW_SIZE;
        sum_temp += state.samples[idx].temp_c;
        sum_humidity += state.samples[idx].humidity_pct;
    }

    avg_temp_c = sum_temp / (float)state.count;
    avg_humidity_pct = sum_humidity / (float)state.count;

    return true;
}

/**
 * Get the current number of valid samples in the window.
 * Expires old samples before returning the count.
 *
 * @param state  The running average state
 * @param now_ms Current timestamp for expiration check
 * @return Number of samples currently in the window
 */
inline int runningAverage_sampleCount(RunningAverageState& state, unsigned long now_ms) {
    runningAverage_expireOld(state, now_ms);
    return state.count;
}

#endif // RUNNING_AVERAGE_LOGIC_H
