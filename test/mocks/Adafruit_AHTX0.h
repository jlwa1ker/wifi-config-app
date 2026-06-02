#ifndef MOCK_ADAFRUIT_AHTX0_H
#define MOCK_ADAFRUIT_AHTX0_H

// Mock Adafruit_AHTX0 for host-based testing.
// Provides minimal stubs so that sensor_poller.cpp can compile without Arduino hardware.

#include <cstdint>

// Mock Adafruit Unified Sensor types
typedef struct {
    float temperature;
    float relative_humidity;
} sensors_event_data_t;

typedef struct {
    sensors_event_data_t data;
    uint32_t timestamp;
} sensors_event_t;

// Mock sensor object returned by getTemperatureSensor() / getHumiditySensor()
class Adafruit_Sensor {
public:
    bool getEvent(sensors_event_t* event) {
        if (event) {
            event->timestamp = 0;
            event->data.temperature = mock_temperature_;
            event->data.relative_humidity = mock_humidity_;
        }
        return mock_success_;
    }

    // Test helpers to control mock behavior
    void setMockTemperature(float temp) { mock_temperature_ = temp; }
    void setMockHumidity(float hum) { mock_humidity_ = hum; }
    void setMockSuccess(bool success) { mock_success_ = success; }

private:
    float mock_temperature_ = 22.0f;
    float mock_humidity_ = 50.0f;
    bool mock_success_ = true;
};

class Adafruit_AHTX0 {
public:
    Adafruit_AHTX0() : initialized_(false) {}

    bool begin() {
        initialized_ = mock_begin_success_;
        return mock_begin_success_;
    }

    Adafruit_Sensor* getTemperatureSensor() {
        return &temp_sensor_;
    }

    Adafruit_Sensor* getHumiditySensor() {
        return &humidity_sensor_;
    }

    // Test helpers to control mock behavior
    void setMockBeginSuccess(bool success) { mock_begin_success_ = success; }
    Adafruit_Sensor& getMockTempSensor() { return temp_sensor_; }
    Adafruit_Sensor& getMockHumiditySensor() { return humidity_sensor_; }

private:
    bool initialized_;
    bool mock_begin_success_ = true;
    Adafruit_Sensor temp_sensor_;
    Adafruit_Sensor humidity_sensor_;
};

#endif
