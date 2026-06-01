#ifndef TEMPERATURE_CONVERT_H
#define TEMPERATURE_CONVERT_H

#include <cmath>

/**
 * Convert a temperature from Celsius to Fahrenheit, rounded to 1 decimal place.
 *
 * Uses the standard conversion formula: (temp_c * 9/5) + 32
 * The result is rounded to one decimal place for storage in the reading cache.
 *
 * Extracted as a pure inline function for host-based testing.
 *
 * @param temp_c  Temperature in degrees Celsius
 * @return Temperature in degrees Fahrenheit, rounded to 1 decimal place
 */
static inline float celsiusToFahrenheit(float temp_c) {
    float raw = (temp_c * 9.0f / 5.0f) + 32.0f;
    return roundf(raw * 10.0f) / 10.0f;
}

#endif // TEMPERATURE_CONVERT_H
