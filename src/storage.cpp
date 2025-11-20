// storage.cpp
#include "hardware/storage.h"
#include "utils.h"

StorageManager::StorageManager() : _cs(SD_CS), _initialized(false), _sd_card_present(false) {
    _filename = "/datalog.csv";
}
StorageManager::StorageManager(uint8_t cs) 
    : _initialized(false), _sd_card_present(false), _cs(cs) {
        _filename = "/datalog.csv";
    }

bool StorageManager::begin() {
    if (_initialized) {
        Serial.println("StorageManager already initialized");
        return true;
    }
    
    Serial.print("Initializing SD card...");
    
    if (!SD.begin(_cs)) {
        logError("SD card initialization failed");
        return false;
    }
    
    _sd_card_present = true;
    
    // Create header if needed
    if (!createHeaderIfNeeded()) {
        logError("Failed to create CSV header");
        return false;
    }

    _initialized = true;
    Serial.println("SD card initialized successfully");
    return true;
}

bool StorageManager::isReady(){
    return _initialized && _sd_card_present;
}
bool StorageManager::storeReading(const SensorReading &reading, const String &dateTime)
{
    if (!_initialized) {
        return initializationError();
    }
    
    File dataFile = SD.open(_filename.c_str(), FILE_APPEND);
    if (!dataFile) {
        logError("Failed to open file for writing");
        return false;
    }
    
    // Write CSV data: DateTime, Temperature, Humidity, Pressure
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

// Add to storage.cpp
bool StorageManager::getAllReadings(std::vector<SensorReading>& readings) {
    // Just call getReadingsSince with timestamp 0 to get all readings
    return getReadingsSince(0, readings);
}

bool StorageManager::getReadingsSince(time_t timestamp, std::vector<SensorReading>& readings) {
    readings.clear();
    
    if (!isReady()) return false;
    
    File dataFile = SD.open(_filename, FILE_READ);
    if (!dataFile) {
        logError("Failed to open file for reading");
        return false;
    }
    
    // Skip header
    if (dataFile.available()) {
        dataFile.readStringUntil('\n');
    }
    
    // Parse and filter readings
    while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        line.trim();
        
        if (line.length() > 0) {
            SensorReading reading = parseReading(line); // StorageManager parses everything
            
            if (reading.isValid && reading.timestamp >= timestamp) {
                readings.push_back(reading);
            }
        }
    }
    
    dataFile.close();
    Serial.printf("Retrieved %d readings since %lu\n", readings.size(), timestamp);
    return true;
}

bool StorageManager::fileExists(const String &filename)
{
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

bool StorageManager::deleteFile(const String &filename)
{
    if(!fileExists(filename)){
        return false;
    }

    return SD.remove(filename);
}

bool StorageManager::testConnection() {
    if (!_initialized) return false;
    
    // Try to open and close the root directory
    File root = SD.open("/");
    if (!root) return false;
    
    root.close();
    return true;
}

/* Return the Space used by the datalog file. */
uint32_t StorageManager::getUsedSpace(){
    return getFileSize(_filename);
}

/* Remove reading older than 30 days */
void StorageManager::cleanup(){
    // TODO Add adjustable cleanup_period
    uint32_t cutoffTime = millis() - (30UL * 24UL * 60UL* 60UL * 1000UL);

    // The Cleanup procedure would require to rewrite the file to remove old entries
    // and is therefore not implemented for now
    Serial.printf("Cleanup: would remove readings older than %lu\n", cutoffTime);
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
    logError("SD card not initialized");
    return false;
}

// Helper function to parse the reading of the CSV Line. 
// It is not very professionally implemented
SensorReading StorageManager::parseReading(const String &line) {
    SensorReading reading;
    
    // Example Parse: "2024-01-15 10:30:45,23.5,65.2,1013.25"
    int first = line.indexOf(',');
    int second = line.indexOf(',', first + 1);
    int third = line.indexOf(',', second + 1);
    
    if (first == -1 || second == -1 || third == -1) {
        return reading; // isValid = false
    }
    
    String dateTimeStr = line.substring(0, first);
    reading.timestamp = DateTimeUtils::parseDateTime(dateTimeStr);      // Example: 2024-01-15 10:30:45
    reading.temperature = line.substring(first + 1, second).toFloat();  // Example: 23.5
    reading.humidity = line.substring(second + 1, third).toFloat();     // Example: 65.2
    reading.pressure = line.substring(third + 1).toFloat();             // Example: 1013.25
    reading.isValid = (reading.timestamp > 0); // Valid if datetime parsed
    
    return reading;
}

