#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include <ArduinoJson.h>
#include <cstring>
#include <string>
#include <vector>

#include "reading_cache_logic.h"
#include "json_serializer.h"

// Generator for ISO 8601-like timestamp strings (26 chars max)
static rc::Gen<std::string> genTimestamp() {
    return rc::gen::apply(
        [](int year, int month, int day, int hour, int minute, int second) {
            char buf[26];
            snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d+00:00",
                     year, month, day, hour, minute, second);
            return std::string(buf);
        },
        rc::gen::inRange(2020, 2100),
        rc::gen::inRange(1, 13),
        rc::gen::inRange(1, 29),
        rc::gen::inRange(0, 24),
        rc::gen::inRange(0, 60),
        rc::gen::inRange(0, 60)
    );
}

// Generator for temperature in Fahrenheit range
static rc::Gen<float> genTemperature() {
    return rc::gen::map(rc::gen::inRange(-400, 1850), [](int x) {
        return static_cast<float>(x) / 10.0f;
    });
}

// Generator for humidity percentage
static rc::Gen<float> genHumidity() {
    return rc::gen::map(rc::gen::inRange(0, 1001), [](int x) {
        return static_cast<float>(x) / 10.0f;
    });
}

// Generator for location strings (alphanumeric, 1-20 chars)
static rc::Gen<std::string> genLocation() {
    return rc::gen::map(
        rc::gen::inRange(1, 21),
        [](int len) {
            std::string result;
            const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789-";
            const int charsetLen = sizeof(charset) - 1;
            for (int i = 0; i < len; i++) {
                result.push_back(charset[i % charsetLen]);
            }
            return result;
        }
    );
}

/**
 * Feature: hygrometer-reporting, Property 6: JSON serialization contains all required fields
 *
 * Validates: Requirements 5.2
 *
 * For any array of CachedReading structs with valid data, the serialized JSON
 * output SHALL be a valid JSON object containing a "readings" array where each
 * element has "timestamp" (string), "temperature_f" (number), "humidity_pct"
 * (number), and "location" (string) fields with values matching the input structs.
 */
TEST_CASE("Property 6: JSON serialization contains all required fields", "[property][json_serializer]") {
    rc::check("serialized JSON is valid and contains all required fields with correct values",
        []() {
            // Generate a random number of readings (1 to 96)
            const auto numReadings = *rc::gen::inRange(1, 97);
            const auto location = *genLocation();

            ReadingCache cache;
            readingCache_init(cache);

            // Store the expected values for verification
            std::vector<std::string> expectedTimestamps;
            std::vector<float> expectedTemps;
            std::vector<float> expectedHumidities;

            for (int i = 0; i < numReadings; i++) {
                const auto timestamp = *genTimestamp();
                const auto temperature = *genTemperature();
                const auto humidity = *genHumidity();

                readingCache_add(cache, timestamp.c_str(), temperature, humidity);

                expectedTimestamps.push_back(timestamp);
                expectedTemps.push_back(temperature);
                expectedHumidities.push_back(humidity);
            }

            // Serialize the cache to JSON
            // Buffer size: generous allocation for up to 96 readings
            const size_t bufferSize = 96 * 256;
            std::vector<char> buffer(bufferSize);

            size_t written = serializeReadingsToJson(cache, location.c_str(),
                                                     buffer.data(), bufferSize);

            // Verify serialization succeeded
            RC_ASSERT(written > 0);

            // Parse the JSON output
            DynamicJsonDocument doc(bufferSize);
            DeserializationError err = deserializeJson(doc, buffer.data(), written);

            // Verify JSON is valid
            RC_ASSERT(err == DeserializationError::Ok);

            // Verify "readings" array exists
            RC_ASSERT(doc.containsKey("readings"));
            RC_ASSERT(doc["readings"].is<JsonArray>());

            JsonArray readings = doc["readings"].as<JsonArray>();

            // Verify array length matches cache count
            RC_ASSERT(static_cast<int>(readings.size()) == numReadings);

            // Verify each entry has all required fields with correct values
            for (int i = 0; i < numReadings; i++) {
                JsonObject entry = readings[i].as<JsonObject>();

                // Verify all required fields exist
                RC_ASSERT(entry.containsKey("timestamp"));
                RC_ASSERT(entry.containsKey("temperature_f"));
                RC_ASSERT(entry.containsKey("humidity_pct"));
                RC_ASSERT(entry.containsKey("location"));

                // Verify field types
                RC_ASSERT(entry["timestamp"].is<const char*>());
                RC_ASSERT(entry["temperature_f"].is<float>());
                RC_ASSERT(entry["humidity_pct"].is<float>());
                RC_ASSERT(entry["location"].is<const char*>());

                // Verify values match input (FIFO order)
                RC_ASSERT(std::string(entry["timestamp"].as<const char*>()) == expectedTimestamps[i]);
                RC_ASSERT(entry["temperature_f"].as<float>() == expectedTemps[i]);
                RC_ASSERT(entry["humidity_pct"].as<float>() == expectedHumidities[i]);
                RC_ASSERT(std::string(entry["location"].as<const char*>()) == location);
            }
        }
    );
}
