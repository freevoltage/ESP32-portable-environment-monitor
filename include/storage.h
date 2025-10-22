#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "sensor.h"
#include "config.h"

class StorageManager {
public:
    StorageManager();
    ~StorageManager();
    
    // Core functionality
    bool begin();
    bool writeReading(const SensorReading& reading, const String& dateTime);
    
    // File operations
    bool fileExists(const String& filename);
    uint32_t getFileSize(const String& filename);
    
    // Diagnostic functions
    void printCardInfo();
    void listFiles();
    bool readFile(const String &filename, String &buff);


private:
    String _filename;
    bool _initialized;
    
    // Helper functions
    bool createHeaderIfNeeded();
    void logError(const String& message);
    void listDirectory(const String& path, uint8_t levels = 1);
    bool initializationError();
};

#endif
