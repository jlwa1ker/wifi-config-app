#ifndef DISPLAY_POWER_LOGIC_H
#define DISPLAY_POWER_LOGIC_H

/**
 * Pure logic for the OLED power timeout and the wake button.
 *
 * The OLED degrades with lit-pixel hours, so the screen must be dark whenever
 * it is not in use. The panel cannot be unpowered from a GPIO (too much
 * current), so "off" means every pixel is off (SSD1306 DISPLAYOFF).
 *
 * No Arduino dependencies, so it can be unit-tested on the host. The sketch
 * owns the I/O (millis(), the button pin, the SSD1306 commands).
 *
 * Screen states:
 *   held   on, no timer. Used for the whole boot sequence and for the
 *          configuration sequence. Ends only by releasing the hold.
 *   timed  on, countdown running. Turns off when the duration expires.
 *   off    all pixels off.
 *
 * Button presses start or restart the countdown, but are ignored while held
 * so that a press during boot cannot cut the boot display short.
 */

// Debounce time for the wake button (no hardware debounce).
#define DISPLAY_BUTTON_DEBOUNCE_MS 30UL

struct DisplayPowerState {
    bool on;              // screen should currently be lit
    bool held;            // on with no timer (boot / configuration)
    bool timerRunning;    // countdown in progress
    unsigned long timerStartMs;
};

/**
 * Initialize at power-up: the screen is on and held for the boot sequence.
 */
inline void displayPower_init(DisplayPowerState& s) {
    s.on = true;
    s.held = true;
    s.timerRunning = false;
    s.timerStartMs = 0;
}

/**
 * Turn the screen on and keep it on with no timer (boot, configuration).
 */
inline void displayPower_hold(DisplayPowerState& s) {
    s.on = true;
    s.held = true;
    s.timerRunning = false;
}

/**
 * End a hold (or restart a running countdown): the screen stays on and the
 * countdown starts from now. Used when the device reaches run mode, enters AP
 * mode, or stops with an error.
 */
inline void displayPower_startTimer(DisplayPowerState& s, unsigned long now_ms) {
    s.on = true;
    s.held = false;
    s.timerRunning = true;
    s.timerStartMs = now_ms;
}

/**
 * A debounced button press. Starts the countdown, or resets it to the start
 * time if it is already running. Ignored while held.
 */
inline void displayPower_buttonPressed(DisplayPowerState& s, unsigned long now_ms) {
    if (s.held) {
        return;
    }
    displayPower_startTimer(s, now_ms);
}

/**
 * Expire the countdown. Call every loop().
 *
 * @param duration_ms  How long the screen stays on after the timer starts
 * @return true if the screen should be lit
 */
inline bool displayPower_tick(DisplayPowerState& s, unsigned long now_ms, unsigned long duration_ms) {
    if (s.timerRunning && (now_ms - s.timerStartMs) >= duration_ms) {
        s.timerRunning = false;
        s.on = false;
    }
    return s.on;
}

/**
 * Debounce state for an active-low button with a pull-up.
 */
struct ButtonDebounceState {
    bool stableHigh;        // debounced level (true = released)
    bool lastRawHigh;       // last raw reading
    unsigned long lastChangeMs;  // when the raw reading last changed
};

inline void buttonDebounce_init(ButtonDebounceState& s) {
    s.stableHigh = true;
    s.lastRawHigh = true;
    s.lastChangeMs = 0;
}

/**
 * Feed a raw reading. Returns true exactly once per press, when a new level
 * has been stable for DISPLAY_BUTTON_DEBOUNCE_MS and that level is "pressed"
 * (low). Releases and bounces return false.
 *
 * @param rawHigh  true when the pin reads HIGH (button released)
 */
inline bool buttonDebounce_update(ButtonDebounceState& s, bool rawHigh, unsigned long now_ms) {
    if (rawHigh != s.lastRawHigh) {
        s.lastRawHigh = rawHigh;
        s.lastChangeMs = now_ms;
    }
    if (s.lastRawHigh != s.stableHigh &&
        (now_ms - s.lastChangeMs) >= DISPLAY_BUTTON_DEBOUNCE_MS) {
        s.stableHigh = s.lastRawHigh;
        return !s.stableHigh;
    }
    return false;
}

#endif // DISPLAY_POWER_LOGIC_H
