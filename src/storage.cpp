#include "storage.h"

StorageManager::StorageManager() 
    : _filename("/datalog.csv"), _initialized(false) {
}

StorageManager::~StorageManager() {
    // Cleanup if needed
}

bool StorageManager::begin() {
    if (_initialized) {
        Serial.println("StorageManager already initialized");
        return true;
    }
    
    Serial.print("Initializing SD card...");
    
    if (!SD.begin(SD_CS)) {
        logError("SD card initialization failed");
        return false;
    }
    
    _initialized = true;
    Serial.println(" Success!");
    
    // Create header if needed
    if (!createHeaderIfNeeded()) {
        logError("Failed to create CSV header");
        return false;
    }
    
    return true;
}

bool StorageManager::writeReading(const SensorReading& reading, const String& dateTime) {
    if (!_initialized) {
        return initializationError();
    }
    
    File dataFile = SD.open(_filename.c_str(), FILE_APPEND);
    if (!dataFile) {
        logError("Failed to open file for writing");
        return false;
    }
    
    // Write CSV data: DateTime,Temperature,Humidity,Pressure
    dataFile.print(dateTime);
    dataFile.print(",");
    dataFile.print(reading.temperature, 2);
    dataFile.print(",");
    dataFile.print(reading.humidity, 2);
    dataFile.print(",");
    dataFile.println(reading.pressure, 2);
    
    dataFile.close();
    
    Serial.println("Data logged to SD card");
    return true;
}

bool StorageManager::fileExists(const String& filename) {
    return SD.exists(filename.c_str());
}

uint32_t StorageManager::getFileSize(const String& filename) {
    if (!fileExists(filename)) {
        return 0;
    }
    
    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file) {
        return 0;
    }
    
    uint32_t size = file.size();
    file.close();
    return size;
}

void StorageManager::printCardInfo() {
    if (!_initialized) {
        initializationError();
        return;
    }
    
    Serial.println("\n=== SD Card Information ===");
    
    // Basic SD card info (what's actually available in ESP32 SD library)
    Serial.println("SD Card: Detected and functional");
    
    // Check if our log file exists
    if (fileExists(_filename)) {
        uint32_t logSize = getFileSize(_filename);
        Serial.printf("Log file: %s (%u bytes)\n", _filename.c_str(), logSize);
        
        // Estimate number of readings (rough calculation)
        if (logSize > 50) { // Account for header
            uint32_t estimatedReadings = (logSize - 50) / 30; // ~30 bytes per reading
            Serial.printf("Estimated readings: %u\n", estimatedReadings);
        }
    } else {
        Serial.println("Log file: Not created yet");
    }
    
    Serial.println("============================\n");
}

void StorageManager::listFiles() {
    if (!_initialized) {
        initializationError();
        return;
    }
    
    Serial.println("\n=== SD Card Files ===");
    listDirectory("/", 2);
    Serial.println("====================\n");
}

bool StorageManager::readFile(const String &filename, String &buff)
{
    if(!_initialized){
        return initializationError();
    }

    if(!fileExists(filename)){
        Serial.printf("File doesn't exist: %s\n", filename);
        return false;
    }

    File dataFile = SD.open(filename.c_str(), FILE_READ);
    if (!dataFile) {
        logError("Failed to open file: " + filename);
        return false;
    }

    // Read entire File into the buffer
    buff = dataFile.readString();
    dataFile.close();

    Serial.printf("Successfully read %d bytes from %s\n", buff.length(), filename.c_str());
    return true;
}

// Private helper functions
bool StorageManager::createHeaderIfNeeded() {
    if (fileExists(_filename)) {
        Serial.println("Log file exists, header already present");
        return true;
    }
    
    Serial.print("Creating new log file with header...");
    
    File dataFile = SD.open(_filename.c_str(), FILE_WRITE);
    if (!dataFile) {
        return false;
    }
    
    // Write CSV header
    dataFile.println("DateTime,Temperature(C),Humidity(%),Pressure(hPa)");
    dataFile.close();
    
    Serial.println(" Success!");
    return true;
}

void StorageManager::logError(const String& message) {
    Serial.print("[SD ERROR] ");
    Serial.println(message);
}

void StorageManager::listDirectory(const String& path, uint8_t levels) {
    File root = SD.open(path.c_str());
    if (!root) {
        logError("Failed to open directory: " + path);
        return;
    }
    
    if (!root.isDirectory()) {
        logError("Path is not a directory: " + path);
        root.close();
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        // Print indentation based on depth
        for (uint8_t i = 0; i < (2 - levels); i++) {
            Serial.print("  ");
        }
        
        if (file.isDirectory()) {
            Serial.print("[DIR]  ");
            Serial.println(file.name());
            
            // Recursively list subdirectories if levels > 0
            if (levels > 0) {
                String subPath = path;
                if (!subPath.endsWith("/")) {
                    subPath += "/";
                }
                subPath += file.name();
                listDirectory(subPath, levels - 1);
            }
        } else {
            Serial.print("[FILE] ");
            Serial.print(file.name());
            Serial.print(" (");
            Serial.print(file.size());
            Serial.println(" bytes)");
        }
        
        file = root.openNextFile();
    }
    
    root.close();
}

bool StorageManager::initializationError()
{
    Serial.println("SD card not initialized");
    return false;
}
