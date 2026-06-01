#include <catch2/catch_test_macros.hpp>
#include <rapidcheck.h>
#include <cmath>

#include "include/temperature_convert.h"

/**
 * Feature: hygrometer-reporting, Property 8: Celsius to Fahrenheit conversion with rounding
 *
 * Validates: Requirements 6.1, 6.2
 *
 * For any float value representing a temperature in Celsius (within the AHT20's
 * operating range of -40°C to 85°C), celsiusToFahrenheit() SHALL produce a value
 * equal to round((temp_c × 9 / 5) + 32, 1 decimal place), and the result
 * multiplied by 10 SHALL be an integer within floating-point tolerance.
 */
TEST_CASE("Property 8: Celsius to Fahrenheit conversion with rounding", "[property][temperature_convert]") {
    rc::check("conversion matches formula and result has at most 1 decimal place",
        []() {
            // Generate a random float in the AHT20 sensor range [-40, 85]
            const auto temp_c = *rc::gen::map(
                rc::gen::inRange(-40000, 85001),
                [](int milli) { return static_cast<float>(milli) / 1000.0f; }
            );

            // Compute expected value using the conversion formula with rounding
            float raw = (temp_c * 9.0f / 5.0f) + 32.0f;
            float expected = roundf(raw * 10.0f) / 10.0f;

            // Call the function under test
            float actual = celsiusToFahrenheit(temp_c);

            // Verify the conversion matches the expected formula result
            RC_ASSERT(actual == expected);

            // Verify the result has at most 1 decimal place:
            // result * 10 should be an integer (within floating-point tolerance)
            float scaled = actual * 10.0f;
            float rounded_scaled = roundf(scaled);
            RC_ASSERT(std::fabs(scaled - rounded_scaled) < 0.001f);
        }
    );
}
