#pragma once

extern "C" {
    void setMockSensorReading(float temp, float humidity, float pressure);
    void setMockSensorShouldFail(bool shouldFail);
    void setMockConnectionOK(bool connectionOK);
    void resetMockSensor();
    void resetMockStorage();
    void setMockEpoch(time_t epoch);
    void resetMockRtc();
    void setMockWifiShouldFail(bool shouldFail);
    void setMockNtpShouldFail(bool shouldFail);
    void resetMockWifi();
}
