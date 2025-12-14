#pragma once

// Mock control functions for testing
extern "C" {
    void setMockSensorReading(float temp, float humidity, float pressure);
    void setMockSensorShouldFail(bool shouldFail);
    void setMockConnectionOK(bool connectionOK);
    void resetMockSensor();
}
