// storage.h
#pragma once
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "config.h"
#include "data_structures.h"

class StorageManager {
public:

    StorageManager(); // This uses the SD_CS definition in config.h
    StorageManager(uint8_t csPin);
    
    // Core functionality
    bool begin();
    bool isReady();

    // Storage Operations
    bool storeReading(const SensorReading& reading, const String& dateTime);
    // TODO How can I figure out how much memory I need to reserve for the readings array?
    // Returns the readings since the given timestamp, 

    // TODO Add a max count number to this function.
    bool getAllReadings(std::vector<SensorReading>& readings);
    bool getReadingsSince(time_t timestamp, std::vector<SensorReading>& readings);

    // TODO This will be removed form the storage class. It should only contain simple operations
    //bool getReadingsSince(uint32_t timestamp, SensorReading* readings, uint16_t max_count, uint16_t& actual_count);
    
    // File operations
    bool fileExists(const String& filename);
    uint32_t getFileSize(const String& filename);
    bool deleteFile(const String &filename);

    // Give The statistics 
    //bool getTemperatureStats(int hoursBack, time_t currentTime, TemperatureStats &stats);

    // TODO LowestTemperature is Responsibility of the DataService Class
    //float getLowestTemperature(int hoursBack, time_t currentTime);

    // Utility Methods
    bool testConnection();
    uint32_t getFreeSpace(); // NOTE: SD Library doesn't provide free space function
    uint32_t getUsedSpace();
    void cleanup();

    // Diagnostic functions
    void printCardInfo();
    void listFiles();
    bool readFile(const String &filename, String &buff);

private:
    uint8_t _cs;
    String _filename;
    bool _initialized;
    bool _sd_card_present;
    
    // Helper functions
    bool createHeaderIfNeeded();
    void logError(const String& message);
    void listDirectory(const String& path, uint8_t levels = 1);
    bool initializationError();

    SensorReading parseReading(const String& line);
};