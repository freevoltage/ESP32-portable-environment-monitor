#pragma once
#include "sensor_manager.h"
#include "storage_manager.h"
#include "rtc_manager.h"
#include "data_structures.h"
#include "utils.h"

class DataService {
    public:
        DataService(SensorManager* sensor, StorageManager* storage, RTCManager* rtc);

        // Core Data Operations
        // Retrieves Reading From Sensor
        bool collectCurrentReading(); 
        SensorReading getCurrentReading() const;
        bool hasCurrentReading() const;

        // Store last reading to SD card
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

        SensorReading _lastReading;
        bool _hasValidData = false;


        // Helper Methods
        TemperatureStats calculateStatsFromReadings(const std::vector<SensorReading>& readings);
};