#pragma once
#include "sensor.h"
#include "storage.h"
#include "rtc_manager.h"
#include "data_structures.h"
#include "utils.h"

//#include "sensor/src/sensor.h"


class DataService {
    public:
        DataService(SensorManager* sensor, StorageManager* storage, RTCManager* rtc);

        // Core Data Operations
        // TODO What is the purpose of this? Why is there an collect and a get?
        bool collectCurrentReading(); 
        SensorReading getCurrentReading() const;
        bool hasCurrentReading() const;

        // Historical Data & Analysis
        // TODO According to the name it is not clear why there are different get and store operations.
        bool storeCurrentReading();

        // This function asks the Storage Manager for Data using the "getReadingSince" Method
        // Then it performs the calculations on it to get the stats.
        TemperatureStats calculateStats(int hoursBack);
        std::vector<SensorReading> getRecentReadings(int count = 10);

        // Data Validation
        bool isReadingValid(const SensorReading& reading);

        // System Status        
        bool isDataStale() const;
        unsigned long getTimeSinceLastReading() const;

    private:
        SensorManager* sensorManager;
        StorageManager* storageManager;
        RTCManager* rtcManager;

        SensorReading lastReading;
        bool hasValidData = false;


        // Helper Methods
        TemperatureStats calculateStatsFromReadings(const std::vector<SensorReading>& readings);
};