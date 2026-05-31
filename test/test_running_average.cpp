#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include <cmath>
#include <vector>
#include <algorithm>

#include "running_average_logic.h"

/**
 * Feature: hygrometer-reporting, Property 1: Running average computes correct windowed mean
 *
 * Validates: Requirements 2.1, 2.2, 2.3
 *
 * For any sequence of temperature and humidity samples with associated timestamps,
 * the running average SHALL equal the arithmetic mean of only those samples whose
 * timestamps fall within the most recent 60 seconds, and samples older than 60 seconds
 * SHALL NOT contribute to the computed average.
 */
TEST_CASE("Property 1: Running average computes correct windowed mean", "[property][running_average]") {
    rc::check("average equals arithmetic mean of samples within 60-second window",
        []() {
            RunningAverageState state;
            runningAverage_init(state);

            // Generate a sequence of 10 to 60 samples (need at least 10 for getAverage to return true)
            const auto sampleCount = *rc::gen::inRange(10, 61);

            // Start time: arbitrary base timestamp
            const unsigned long baseTime = 100000UL;

            // Generate samples with ~1 second intervals and some variation
            struct TestSample {
                float temp_c;
                float humidity_pct;
                unsigned long timestamp_ms;
            };
            std::vector<TestSample> samples;

            unsigned long currentTime = baseTime;
            for (int i = 0; i < sampleCount; i++) {
                // Generate random temperature in [-40, 85] range (AHT20 operating range)
                float temp = *rc::gen::map(
                    rc::gen::inRange(-4000, 8501),
                    [](int v) { return v / 100.0f; }
                );
                // Generate random humidity in [0, 100] range
                float humidity = *rc::gen::map(
                    rc::gen::inRange(0, 10001),
                    [](int v) { return v / 100.0f; }
                );
                // Advance time by 800-1200ms (simulating ~1 second intervals with jitter)
                unsigned long interval = *rc::gen::inRange(800, 1201);
                currentTime += interval;

                samples.push_back({temp, humidity, currentTime});
                runningAverage_addSample(state, temp, humidity, currentTime);
            }

            // The "now" time is the timestamp of the last sample added
            unsigned long now = currentTime;

            // Compute expected average: only samples within 60 seconds of 'now'
            float expectedTempSum = 0.0f;
            float expectedHumiditySum = 0.0f;
            int expectedCount = 0;

            for (const auto& s : samples) {
                unsigned long age = now - s.timestamp_ms;
                if (age <= RUNNING_AVG_WINDOW_MS) {
                    expectedTempSum += s.temp_c;
                    expectedHumiditySum += s.humidity_pct;
                    expectedCount++;
                }
            }

            // With 10-60 samples at ~1s intervals (max ~60s total), all should be within window
            RC_ASSERT(expectedCount >= RUNNING_AVG_MIN_SAMPLES);

            float expectedTempAvg = expectedTempSum / (float)expectedCount;
            float expectedHumidityAvg = expectedHumiditySum / (float)expectedCount;

            // Get the actual average from the module
            float actualTempAvg = 0.0f;
            float actualHumidityAvg = 0.0f;
            bool result = runningAverage_getAverage(state, actualTempAvg, actualHumidityAvg, now);

            RC_ASSERT(result == true);
            // Allow small floating-point tolerance
            RC_ASSERT(std::fabs(actualTempAvg - expectedTempAvg) < 0.001f);
            RC_ASSERT(std::fabs(actualHumidityAvg - expectedHumidityAvg) < 0.001f);
        }
    );

    rc::check("samples older than 60 seconds are excluded from the average",
        []() {
            RunningAverageState state;
            runningAverage_init(state);

            // Generate "old" samples that will be outside the 60-second window
            const auto oldSampleCount = *rc::gen::inRange(5, 20);
            // Generate "recent" samples that will be inside the window (need at least 10)
            const auto recentSampleCount = *rc::gen::inRange(10, 40);

            unsigned long baseTime = 100000UL;
            unsigned long currentTime = baseTime;

            // Add old samples (these will be > 60 seconds old by the time we query)
            for (int i = 0; i < oldSampleCount; i++) {
                float temp = *rc::gen::map(
                    rc::gen::inRange(-4000, 8501),
                    [](int v) { return v / 100.0f; }
                );
                float humidity = *rc::gen::map(
                    rc::gen::inRange(0, 10001),
                    [](int v) { return v / 100.0f; }
                );
                currentTime += *rc::gen::inRange(800, 1201);
                runningAverage_addSample(state, temp, humidity, currentTime);
            }

            // Jump forward in time so old samples are outside the 60-second window
            currentTime += RUNNING_AVG_WINDOW_MS + 1000UL;

            // Add recent samples (these will be within the 60-second window)
            std::vector<float> recentTemps;
            std::vector<float> recentHumidities;

            for (int i = 0; i < recentSampleCount; i++) {
                float temp = *rc::gen::map(
                    rc::gen::inRange(-4000, 8501),
                    [](int v) { return v / 100.0f; }
                );
                float humidity = *rc::gen::map(
                    rc::gen::inRange(0, 10001),
                    [](int v) { return v / 100.0f; }
                );
                currentTime += *rc::gen::inRange(800, 1201);
                runningAverage_addSample(state, temp, humidity, currentTime);
                recentTemps.push_back(temp);
                recentHumidities.push_back(humidity);
            }

            unsigned long now = currentTime;

            // Compute expected average from only recent samples
            float expectedTempSum = 0.0f;
            float expectedHumiditySum = 0.0f;
            for (size_t i = 0; i < recentTemps.size(); i++) {
                expectedTempSum += recentTemps[i];
                expectedHumiditySum += recentHumidities[i];
            }
            float expectedTempAvg = expectedTempSum / (float)recentTemps.size();
            float expectedHumidityAvg = expectedHumiditySum / (float)recentHumidities.size();

            // Get the actual average
            float actualTempAvg = 0.0f;
            float actualHumidityAvg = 0.0f;
            bool result = runningAverage_getAverage(state, actualTempAvg, actualHumidityAvg, now);

            RC_ASSERT(result == true);
            // The average should only reflect recent samples, not old ones
            RC_ASSERT(std::fabs(actualTempAvg - expectedTempAvg) < 0.001f);
            RC_ASSERT(std::fabs(actualHumidityAvg - expectedHumidityAvg) < 0.001f);
        }
    );
}

/**
 * Feature: hygrometer-reporting, Property 2: Minimum sample threshold gates average validity
 *
 * Validates: Requirements 2.5
 *
 * For any number of samples N added to the running average window,
 * getAverage() SHALL return false when N < 10 and SHALL return true when N >= 10.
 */
TEST_CASE("Property 2: Minimum sample threshold gates average validity", "[property][running_average]") {
    rc::check("getAverage returns false when count < 10, true when count >= 10",
        []() {
            // Generate a random sample count in [0, 60]
            const auto sampleCount = *rc::gen::inRange(0, 61);

            RunningAverageState state;
            runningAverage_init(state);

            // Add exactly sampleCount samples, all within the 60-second window
            // Use a base timestamp and increment by 500ms so none expire
            unsigned long baseTime = 100000UL;
            for (int i = 0; i < sampleCount; i++) {
                float temp = 20.0f + (float)(i % 10);
                float humidity = 50.0f + (float)(i % 5);
                unsigned long timestamp = baseTime + (unsigned long)(i * 500);
                runningAverage_addSample(state, temp, humidity, timestamp);
            }

            // Query the average at a time still within the window of all samples
            unsigned long queryTime = baseTime + (unsigned long)(sampleCount * 500);
            float avg_temp = 0.0f;
            float avg_humidity = 0.0f;
            bool result = runningAverage_getAverage(state, avg_temp, avg_humidity, queryTime);

            if (sampleCount < RUNNING_AVG_MIN_SAMPLES) {
                RC_ASSERT(result == false);
            } else {
                RC_ASSERT(result == true);
            }
        }
    );
}
