// data_structures.h
#pragma once
#ifdef MOCK
    #include <cstdint>
    #include <ctime>
#else
    #include <Arduino.h>
#endif
// This causes a compilation fail when we are in a MOCK but it seems that time_t is imported with arduino.h

/**
 * @brief The header defines all shared data structures used between the modules
 * This ensures that a module does not need any dependency to another module just for the 
 * data structure.
 */


 // Structure holding a RAW sensor reading.
struct SensorReading {
    float temperature;
    float humidity;
    float pressure;
    time_t timestamp; // Seconds since Epoch
    bool isValid;
    
    // Default Constructor
    SensorReading() : temperature(0), humidity(0), pressure(0), timestamp(0), isValid(false) {}

    // Specific Constructor
    SensorReading(float temp, float hum, float press, time_t time) 
        : temperature(temp), humidity(hum), pressure(press), timestamp(time), isValid(false) {}
};

struct TemperatureStats {
    float min;
    float max;
    
    float average;
    int sampleCount;
    bool isValid;
    
    // Default Constructor
    TemperatureStats() : min(999), max(-999), average(0), sampleCount(0), isValid(false) {}
};

// Used only for Report
struct SystemStatus {
    bool sensorOk;
    bool displayOk;
    bool storageOk;
    bool rtcOk;
    bool wifiOk;
    uint32_t freeMemory;
    uint32_t uptime;
    
    SystemStatus() : sensorOk(false), displayOk(false), storageOk(false), rtcOk(false), wifiOk(false), freeMemory(0), uptime(0) {}
};


enum class SystemMode {
    NORMAL_OPERATION,
    FIRST_BOOT,
    TIME_SYNC_REQUIRED,
    ERROR_STATE
};

enum class ConnectivityStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    TIME_SYNCING,
    SYNC_COMPLETE
};

//#endif // DATA_STRUCTURES_H
