#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>

#include "include/display_power_logic.h"

/*
 * Tests for the OLED power timeout and wake-button debounce.
 *
 * The screen must be dark whenever it is not in use (panel wear), stay on for
 * the whole boot and configuration sequences, and otherwise stay on for a
 * configurable duration after a wake event.
 */

static const unsigned long DURATION = 15000UL;

TEST_CASE("power-up: screen is on and held with no timer", "[display]") {
    DisplayPowerState s;
    displayPower_init(s);
    REQUIRE(s.on == true);
    REQUIRE(s.held == true);
    REQUIRE(s.timerRunning == false);
}

TEST_CASE("a held screen never times out", "[display]") {
    DisplayPowerState s;
    displayPower_init(s);
    // Boot sequence can take a long time (blocking NTP sync is ~50 s).
    REQUIRE(displayPower_tick(s, 0, DURATION) == true);
    REQUIRE(displayPower_tick(s, 60000UL, DURATION) == true);
    REQUIRE(displayPower_tick(s, 3600000UL, DURATION) == true);
}

TEST_CASE("starting the timer ends the hold and expires after the duration", "[display]") {
    DisplayPowerState s;
    displayPower_init(s);
    displayPower_startTimer(s, 5000);

    REQUIRE(s.held == false);
    REQUIRE(displayPower_tick(s, 5000, DURATION) == true);
    REQUIRE(displayPower_tick(s, 5000 + DURATION - 1, DURATION) == true);
    REQUIRE(displayPower_tick(s, 5000 + DURATION, DURATION) == false);
    // Stays off afterwards.
    REQUIRE(displayPower_tick(s, 5000 + DURATION + 100000UL, DURATION) == false);
}

TEST_CASE("the duration is configurable", "[display]") {
    DisplayPowerState s;
    displayPower_init(s);
    displayPower_startTimer(s, 0);
    REQUIRE(displayPower_tick(s, 29999, 30000UL) == true);
    REQUIRE(displayPower_tick(s, 30000, 30000UL) == false);

    displayPower_startTimer(s, 0);
    REQUIRE(displayPower_tick(s, 4999, 5000UL) == true);
    REQUIRE(displayPower_tick(s, 5000, 5000UL) == false);
}

TEST_CASE("a button press is ignored while held (cannot cut boot short)", "[display]") {
    DisplayPowerState s;
    displayPower_init(s);
    displayPower_buttonPressed(s, 1000);

    REQUIRE(s.held == true);
    REQUIRE(s.timerRunning == false);
    REQUIRE(displayPower_tick(s, 1000 + DURATION * 10, DURATION) == true);
}

TEST_CASE("a button press restarts a running timer", "[display]") {
    DisplayPowerState s;
    displayPower_init(s);
    displayPower_startTimer(s, 0);

    // Press 10 s in: the screen must now last until 25 s, not 15 s.
    displayPower_buttonPressed(s, 10000);
    REQUIRE(displayPower_tick(s, DURATION, DURATION) == true);
    REQUIRE(displayPower_tick(s, 10000 + DURATION - 1, DURATION) == true);
    REQUIRE(displayPower_tick(s, 10000 + DURATION, DURATION) == false);
}

TEST_CASE("a button press wakes a screen that is off", "[display]") {
    DisplayPowerState s;
    displayPower_init(s);
    displayPower_startTimer(s, 0);
    REQUIRE(displayPower_tick(s, DURATION, DURATION) == false);

    displayPower_buttonPressed(s, 40000);
    REQUIRE(displayPower_tick(s, 40000, DURATION) == true);
    REQUIRE(displayPower_tick(s, 40000 + DURATION, DURATION) == false);
}

TEST_CASE("hold turns the screen on and cancels a running timer", "[display]") {
    // Configuration received while the screen is off or counting down.
    DisplayPowerState s;
    displayPower_init(s);
    displayPower_startTimer(s, 0);
    REQUIRE(displayPower_tick(s, DURATION, DURATION) == false);

    displayPower_hold(s);
    REQUIRE(s.on == true);
    REQUIRE(displayPower_tick(s, DURATION * 100, DURATION) == true);

    // Reaching run mode ends the hold and starts the countdown.
    displayPower_startTimer(s, 2000000UL);
    REQUIRE(displayPower_tick(s, 2000000UL + DURATION - 1, DURATION) == true);
    REQUIRE(displayPower_tick(s, 2000000UL + DURATION, DURATION) == false);
}

TEST_CASE("the timer survives millis() wrap-around", "[display]") {
    DisplayPowerState s;
    displayPower_init(s);
    unsigned long start = 0xFFFFFFFFUL - 5000UL;   // wraps 5 s into the countdown
    displayPower_startTimer(s, start);

    REQUIRE(displayPower_tick(s, start + 14999UL, DURATION) == true);   // wrapped value
    REQUIRE(displayPower_tick(s, start + 15000UL, DURATION) == false);
}

/* ---- Button debounce ---- */

TEST_CASE("a clean press registers once, after the debounce time", "[display][button]") {
    ButtonDebounceState b;
    buttonDebounce_init(b);

    REQUIRE(buttonDebounce_update(b, true, 0) == false);
    REQUIRE(buttonDebounce_update(b, false, 100) == false);   // pressed, not stable yet
    REQUIRE(buttonDebounce_update(b, false, 100 + DISPLAY_BUTTON_DEBOUNCE_MS - 1) == false);
    REQUIRE(buttonDebounce_update(b, false, 100 + DISPLAY_BUTTON_DEBOUNCE_MS) == true);
    // Held down: no repeats.
    REQUIRE(buttonDebounce_update(b, false, 5000) == false);
}

TEST_CASE("contact bounce on press registers a single press", "[display][button]") {
    ButtonDebounceState b;
    buttonDebounce_init(b);
    int presses = 0;

    // Bouncing for ~10 ms, then settled low.
    const bool bounce[] = {false, true, false, true, false, false, false};
    unsigned long t = 100;
    for (bool level : bounce) {
        presses += buttonDebounce_update(b, level, t) ? 1 : 0;
        t += 2;
    }
    for (; t < 300; t += 5) {
        presses += buttonDebounce_update(b, false, t) ? 1 : 0;
    }
    REQUIRE(presses == 1);
}

TEST_CASE("releasing the button is not a press, and a later press registers", "[display][button]") {
    ButtonDebounceState b;
    buttonDebounce_init(b);

    buttonDebounce_update(b, false, 0);
    REQUIRE(buttonDebounce_update(b, false, 50) == true);

    buttonDebounce_update(b, true, 100);
    REQUIRE(buttonDebounce_update(b, true, 200) == false);    // release settles, no press

    buttonDebounce_update(b, false, 300);
    REQUIRE(buttonDebounce_update(b, false, 400) == true);    // second press
}

/*
 * Property: a low glitch shorter than the debounce time never registers a
 * press, however it is positioned.
 */
TEST_CASE("Property: glitches shorter than the debounce time are ignored", "[display][button][property]") {
    rc::check("short low pulse produces no press",
        [](unsigned int startOffset, unsigned int pulse) {
            unsigned long glitch = pulse % DISPLAY_BUTTON_DEBOUNCE_MS;   // 0..29 ms
            unsigned long start = 1000UL + (startOffset % 100000UL);

            ButtonDebounceState b;
            buttonDebounce_init(b);

            int presses = 0;
            for (unsigned long t = start - 50; t < start + glitch + 200; t++) {
                bool low = (t >= start && t < start + glitch);
                presses += buttonDebounce_update(b, !low, t) ? 1 : 0;
            }
            RC_ASSERT(presses == 0);
        }
    );
}
