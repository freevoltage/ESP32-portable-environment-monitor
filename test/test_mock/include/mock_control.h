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
    void setMockBatteryVoltage(float voltage);
    void setMockBatteryPercent(float percent);
    void setMockBatteryShouldFail(bool shouldFail);
    void resetMockBattery();
    void setMockTimeSyncMode(int mode);
    void setMockBLESyncShouldFail(bool shouldFail);
    void setMockWiFiSyncShouldFail(bool shouldFail);
    void setMockTimeSyncShouldFail(bool shouldFail);
    void resetMockTimeSync();
}
