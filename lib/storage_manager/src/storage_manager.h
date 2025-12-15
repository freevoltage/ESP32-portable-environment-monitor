// storage_manager.h
#pragma once
#ifndef MOCK
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#endif
#include <config.h>
#include <data_structures.h>
#include <utils.h>
#include <stdint.h>

class StorageManager
{
public:
    StorageManager(); // This uses the SD_CS definition in config.h
    StorageManager(uint8_t csPin);

    // Core functionality
    bool begin();
    bool isReady();
    void reset();

    /* Storage Operations */
    bool storeReading(const SensorReading &reading);
    bool appendLine(const String& csvLine); // TODO Add a Test for this function, although it is pretty easy

    bool getAllReadings(std::vector<SensorReading> &readings);
    bool getLastNReadings(std::vector<SensorReading> &readings, uint16_t maxCount = 10);
    // Returns the readings since the given timestamp, writes it to the given readings vector by overwriting all of its content.
    bool getReadingsSince(time_t timestamp, std::vector<SensorReading> &readings, uint16_t maxCount = 1000);

    /* File operations */
    bool createFile(const String &filename); // TODO Implement me
    bool fileExists(const String &filename);
    uint32_t getFileSize(const String &filename);
    bool deleteFile(const String &filename);
    bool resetFile(); // Remove from File (Delete and Recreate)

    /* Utility Methods */
    // Tests the connection to the SD card by trying to open and close the root directory
    bool testConnection();
    bool testSDCardHealth(); // Creates and Deletes a File

    // TODO Implement getFreeSpace or throw away
    uint32_t getFreeSpace(); // NOTE: SD Library doesn't provide free space function. So this function is not implemented
    uint32_t getUsedSpace();
    // Remove reading older than 30 days.
    void cleanup();

    /* Diagnostic functions */
    void printCardInfo();
    void listFiles();
    // Read the entire file into the buffer
    bool readFile(const String &filename, String &buff, size_t maxSize);

    uint16_t estimateLineCount(); // TODO Move to private later

private:
    uint8_t _cs;
    String _filename;
    bool _initialized;
    bool _sd_card_present;

    // Helper functions
    bool processReadings(std::vector<SensorReading> &readings,
                         bool (*shouldInclude)(const SensorReading &, void *),
                         bool (*shouldStop)(const std::vector<SensorReading> &, void *),
                         void *context);

    // Creates the CSV file header if not already present
    bool createHeaderIfNeeded();
    void listDirectory(const String &path, uint8_t levels = 1);

    // Parse a line from the SD card andmaxCount save it into a SensorReading struct.
    SensorReading parseReading(const String &line);

    // Memory safety helpers
    size_t estimateMemoryNeeded(uint16_t maxCount);
    bool hasEnoughMemory(uint16_t maxCount);
};

// TODO The MAXCOUNT Variable should be a class variable initialized by the constructor. 
// TODO It doesn't make sense to always specify it.  (And its net a SetMaxCount Method)

// Context structures for processReadings callbacks
struct MaxCountContext
{
    uint16_t maxCount;
};

struct SinceContext
{
    time_t timestamp;
    uint16_t maxCount;
};

// Include Filter Functions
bool includeAllReadings(const SensorReading &reading, void *context);
bool includeRecentReadings(const SensorReading &reading, void *context);
bool stopAfterNReadings(const std::vector<SensorReading> &readings, void *context);
bool neverStop(const std::vector<SensorReading> &readings, void *context);