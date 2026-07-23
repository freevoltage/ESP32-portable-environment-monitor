// storage.cpp
#ifndef MOCK
#include "storage_manager.h"
#include <logger.h>

/* StorageManager::StorageManager() : _cs(SD_CS), _initialized(false), _sd_card_present(false)
{
    _filename = SD_FILENAME;
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
} */

StorageManager::StorageManager(const String& filename, uint8_t cs)
    : _filename(filename)
    , _cs(cs)
    ,_initialized(false)
    , _sd_card_present(false)
{
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
    LOG_DEBUG("Storage Manager allocated. Filename: '%s'", _filename.c_str());
}

bool StorageManager::begin()
{
    LOG_INFO("StorageManager begin()");
    reset();

    bool result = SD.begin(_cs);
    
    if (!result){
        LOG_ERROR("SD card not accessible");
        return false;
    }

    _sd_card_present = true;

    result = createHeaderIfNeeded(_filename);
    if (!result) {
        return false;
    }

    _initialized = true;
    LOG_INFO("SD begin initialized successfully");
    return true;
}

bool StorageManager::isReady()
{
    return _initialized && _sd_card_present;
}

void StorageManager::reset()
{
    if (_initialized)
    {
        delay(50);
        SD.end();
    }
    pinMode(_cs, OUTPUT); // Just in Case
    digitalWrite(_cs, HIGH);
    delay(50);
    _initialized = false;
    _sd_card_present = false;

    LOG_DEBUG("StorageManager reset");
}

bool StorageManager::storeReading(const SensorReading &reading)
{
    if (!_initialized)
    {
        LOG_ERROR("Storage not initialized. Writing not possible");
        return false;
    }

    if(!fileExists(_filename)){
        LOG_ERROR("File does not exist: '%s'. Writing not possible.", _filename.c_str());
        return false;
    }

    if(!reading.isValid){
        LOG_WARN("Attempt to store invalid reading");
        return false;
    }

    // Open file for writing
    File dataFile = SD.open(_filename.c_str(), FILE_APPEND);
    if (!dataFile)
    {
        LOG_ERROR("Failed to open file for writing");
        return false;
    }

    // Write CSV data: Timestamp, Temperature, Humidity, Pressure, Altitude
    dataFile.printf("%lu,%.2f,%.2f,%.2f,%.2f\n",
                    reading.timestamp,
                    reading.temperature,
                    reading.humidity,
                    reading.pressure,
                    reading.altitude);

    dataFile.close();

    LOG_INFO("Logged: %lu, %.2f°C %.0f%% %.0fhPa %.0fm -> %s",
             reading.timestamp,
             reading.temperature,
             reading.humidity,
             reading.pressure,
             reading.altitude,
             _filename.c_str());
    return true;
}

bool StorageManager::appendLine(const String &csvLine)
{
    if (!_initialized)
        return false;

    File dataFile = SD.open(_filename.c_str(), FILE_APPEND);
    if (!dataFile)
    {
        LOG_ERROR("Failed to open file");
        return false;
    }

    dataFile.println(csvLine);
    dataFile.close();
    return true;
}

/// @brief Retrieves the last N sensor readings from storage
///
/// Reads the entire storage file and returns up to maxCount of the most recent
/// readings. Uses a circular buffer internally to efficiently maintain only the
/// last N readings while processing the file sequentially.
///
/// Memory efficiency: If the file contains fewer readings than maxCount, only
/// the actual number of readings will be stored (e.g., 10 readings in vector
/// with capacity for 100).
///
/// @param readings Vector to populate with sensor readings. Will be cleared before
///                 adding new readings. Automatically resized as needed.
/// @param maxCount Maximum number of readings to retrieve (must be > 0).
///                 If file contains fewer readings, returns all available readings.
///
/// @return true if readings were successfully retrieved (even if count < maxCount),
///         false if maxCount is 0, insufficient memory, or file cannot be opened
///
/// @note This function must read the entire file to determine which readings are
///       the "last N", so it has O(total_readings) time complexity regardless of N.
/// @note Performs memory availability check before reading to prevent allocation failures.
///
/// Example:
/// @code
/// std::vector<SensorReading> readings;
/// if (storageManager.getLastNReadings(readings, 50)) {
///     // readings contains up to 50 most recent readings
///     Serial.printf("Retrieved %d readings\n", readings.size());
/// }
/// @endcode
bool StorageManager::getLastNReadings(std::vector<SensorReading> &readings, uint16_t maxCount)
{
    if (!_initialized)
    {
        LOG_ERROR("Not initialized");
        return false;
    }

    LOG_INFO("maxCount: %u", maxCount);
    if (maxCount == 0)
        return false;

    if (!hasEnoughMemory(maxCount))
    {
        LOG_ERROR("Insufficient memory");
        return false;
    }

    File file = SD.open(_filename, FILE_READ);

    // Skip header line
    size_t startPos = file.position();
    String header = file.readStringUntil('\n');
    const size_t HEADER_SIZE = file.position() - startPos;

    LOG_DEBUG("Header: %s, size: %d", header.c_str(), HEADER_SIZE);

    if (!file.available())
    {
        LOG_WARN("File is empty.");
        file.close();
        return true;
    }

    // Read all data lines sequentially (handles variable-width lines)
    std::vector<String> allLines;
    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0)
        {
            allLines.push_back(line);
        }
    }
    file.close();

    size_t totalLines = allLines.size();
    LOG_DEBUG("Total data lines: %d", totalLines);

    if (totalLines == 0)
    {
        return true;
    }

    // Return the last N lines, newest first
    uint16_t linesToRead = (totalLines < maxCount) ? totalLines : maxCount;
    size_t startIndex = totalLines - linesToRead;

    for (uint16_t i = 0; i < linesToRead; i++)
    {
        LOG_DEBUG("Parsing: '%s'", allLines[startIndex + i].c_str());
        SensorReading reading = parseReading(allLines[startIndex + i]);
        readings.push_back(reading);
    }

    LOG_INFO("Retrieved %d readings (newest first)", readings.size());

    return true;
}

bool StorageManager::getAllReadings(std::vector<SensorReading> &readings)
{
    return processReadings(readings, includeAllReadings, neverStop, nullptr);
}

/// @brief Retrieves all sensor readings since a specific timestamp
///
/// Filters readings by timestamp and returns up to maxCount readings that were
/// recorded at or after the specified time. Readings are processed in chronological
/// order (oldest to newest).
///
/// @param readings Vector to populate with sensor readings. Will be cleared before
///                 adding new readings. Automatically resized as needed.
/// @param timestamp Unix timestamp (seconds since epoch). Only readings with
///                  timestamps >= this value will be included.
/// @param maxCount Maximum number of readings to retrieve. Use 0 for unlimited
///                 (retrieve all matching readings).
///
/// @return true if operation completed successfully (even if no readings match),
///         false if insufficient memory or file cannot be opened
///
/// @note Time complexity is O(total_readings) as entire file must be scanned.
/// @note If maxCount is reached before end of file, remaining readings are skipped.
/// @note Performs memory availability check before reading to prevent allocation failures.
///
/// Example:
/// @code
/// std::vector<SensorReading> readings;
/// time_t oneHourAgo = now() - 3600;
///
/// // Get all readings from last hour
/// if (storageManager.getReadingsSince(readings, oneHourAgo, 0)) {
///     Serial.printf("Found %d readings in last hour\n", readings.size());
/// }
///
/// // Get max 100 readings since yesterday
/// time_t yesterday = now() - 86400;
/// if (storageManager.getReadingsSince(readings, yesterday, 100)) {
///     Serial.printf("Retrieved %d readings (max 100)\n", readings.size());
/// }
/// @endcode
bool StorageManager::getReadingsSince(time_t timestamp, std::vector<SensorReading> &readings, uint16_t maxCount)
{
    if (maxCount == 0){
        LOG_WARN("Called with maxCount = 0");
        maxCount = estimateLineCount();
    }

    if (!hasEnoughMemory(maxCount))
    {
        LOG_ERROR("Insufficient memory");
        return false;
    }

    readings.reserve(maxCount);
    SinceContext ctx = {timestamp, maxCount};
    return processReadings(readings, includeRecentReadings, neverStop, &ctx);
}

bool StorageManager::createFile(const String &filename)
{
    if(fileExists(filename)){
        LOG_WARN("File '%s' already exist", filename.c_str());
        return true;
    }

    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if(!dataFile){
        LOG_ERROR("File creation failed!");
        return false;
    }
    dataFile.println("HELLO");  // Write empty line (or just a newline)

    dataFile.flush();
    dataFile.close();

    //SD.end();
    delay(100);
    //SD.begin(_cs);

    // Verify file was created
    if(!fileExists(filename)){
        LOG_ERROR("File creation verification failed");
        return false;
    }

    LOG_INFO("File created successfully: '%s'", filename.c_str());
    return true;
}

bool StorageManager::fileExists(const String &filename) const
{
    return SD.exists(filename.c_str());
}

size_t StorageManager::getFileSize(const String &filename) const
{
    if (!fileExists(filename))
    {
        return 0;
    }

    File file = SD.open(filename.c_str(), FILE_READ);
    if (!file)
    {
        return 0;
    }

    size_t size = file.size();
    file.close();
    return size;
}

//? Improve this function.
void StorageManager::printCardInfo()
{
    if (!_initialized)
    {
        LOG_ERROR("SD card not initialized");
        return;
    }

    LOG_INFO("\n=== SD Card Information ===");

    // Basic SD card info (what's actually available in ESP32 SD library)
    LOG_INFO("SD Card: Detected and functional");

    // Check if our log file exists
    if (fileExists(_filename))
    {
        uint32_t logSize = getFileSize(_filename);
        LOG_INFO("Log file: %s (%u bytes)\n", _filename.c_str(), logSize);

        // Estimate number of readings (rough calculation)
        if (logSize > 50)
        {                                                     // Account for header
            uint32_t estimatedReadings = (logSize - 50) / 30; // ~30 bytes per reading
            LOG_INFO("Estimated readings: %u\n", estimatedReadings);
        }
    }
    else
    {
        LOG_WARN("Log file: Not created yet");
    }

    LOG_INFO("============================\n");
}


void StorageManager::listFiles()
{
    if (!_initialized)
    {
        LOG_ERROR("Initialization Error");
        return;
    }

    LOG_INFO("\n=== SD Card Files ===");
    listDirectory("/", 2);
    LOG_INFO("====================\n");
}

// Reads the bare file and saves it into the buffer.
// TODO Again I am confused on the logic. This is becoming spaghetti code.
bool StorageManager::readFile(const String &filename, String &buff, size_t maxSize)
{
    // TODO This file Reading Operation is also not very good
    if (!_initialized)
    {
        LOG_ERROR("Initialization Error");
        return false;
    }

    if (!fileExists(filename))
    {
        LOG_ERROR("File doesn't exist: %s", filename.c_str());
        return false;
    }

    File dataFile = SD.open(filename.c_str(), FILE_READ);
    if (!dataFile)
    {
        LOG_ERROR("Failed to open file: %s", filename.c_str());
        return false;
    }

    size_t fileSize = dataFile.size();
    size_t readSize = std::min(fileSize, maxSize);

    // Check if we have enough memory (with safety margin)
    if (!hasEnoughMemory(readSize))
    {
        LOG_ERROR("Not enough memory to read file. Need: %u, Available: %u", readSize, ESP.getFreeHeap());
        dataFile.close();
        return false;
    }

    // Reserve space for the string
    buff.reserve(readSize);
    buff = "";

    // TODO The Logic is also

    // Read in chunks to avoid memory fragmentation
    const size_t CHUNK_SIZE = 512;
    char chunk[CHUNK_SIZE + 1]; // +1 for null terminator
    size_t totalRead = 0;

    while (dataFile.available() && totalRead < readSize)
    {
        size_t toRead = std::min(CHUNK_SIZE, readSize - totalRead);
        size_t bytesRead = dataFile.readBytes(chunk, toRead);

        if (bytesRead == 0)
            break; // End of file or error

        chunk[bytesRead] = '\0'; // Null terminate
        buff += chunk;
        totalRead += bytesRead;
    }

    dataFile.close();

    if (fileSize > maxSize)
    {
        LOG_WARN("Warning: File truncated. Read %d of %d bytes from %s\n",
                 totalRead, fileSize, filename.c_str());
    }
    else
    {
        LOG_INFO("Successfully read %d bytes from %s\n", totalRead, filename.c_str());
    }

    return totalRead > 0;
}

/// @brief 
/// @param filename 
/// @return 
bool StorageManager::deleteFile(const String &filename)
{
    if (!fileExists(filename))
    {
        return false;
    }
    LOG_INFO("Delete %s", filename.c_str());

    bool result = SD.remove(filename);
    
    // SD.remove() only marks a file for deletion
    // Physical deletion happens on SD.end() or timeout
    // SD.exist() might check cached state

    if(!result){
        LOG_ERROR("DeleteFile '%s' failed!", filename.c_str());
    }
    // Force Filesystem Resync
    SD.end();
    delay(100);
    SD.begin(_cs);

    // Verify deletion
    if(fileExists(filename)){
        LOG_ERROR("File still exists after delete");
        return false;
    }
    LOG_INFO("File deleted: '%s'", filename.c_str());
    return true;
}

bool StorageManager::clearFile()
{
    if (!_initialized)
    {
        LOG_ERROR("Initialization Error");
        return false;
    }

    // Delete file if it exists
    if (fileExists(_filename))
    {
        if (!SD.remove(_filename.c_str()))
        {
            LOG_ERROR("Failed to delete file");
            return false;
        }
        LOG_INFO("File deleted successfully");
    }

    // **KEY FIX**: Reinitialize SD card after file deletion
    // TODO This should just call a method to do so and not duplicate the code here.
    SD.end();
    delay(100);

    _initialized = false;
    _sd_card_present = false;

    // Reinitialize
    if (!begin())
    {
        LOG_ERROR("Failed to reinitialize after reset");
        return false;
    }

    return true;
}

bool StorageManager::testConnection()
{
    uint8_t cardType = SD.cardType();
    
    if (cardType == CARD_NONE) {
        LOG_ERROR("No SD card detected");
        return false;
    }
    
    LOG_INFO("SD Card Type: %s", 
             cardType == CARD_MMC ? "MMC" :
             cardType == CARD_SD ? "SDSC" :
             cardType == CARD_SDHC ? "SDHC" : "UNKNOWN");
    return true;
}

bool StorageManager::testSDCardHealth()
{
    // Try to create and delete a test file
    File test = SD.open("/_test.tmp", FILE_WRITE);
    if (!test) {
        LOG_ERROR("SD health check failed: cannot create file");
        return false;
    }
    test.close();
    delay(50);
    
    // Try to delete file
    if (!SD.remove("/_test.tmp")) {
        LOG_ERROR("SD health check failed: cannot delete file");
        
        return false;
    }
    
    LOG_INFO("SD health check: OK");
    return true;
}

/* Return the Space used by the datalog file. */
size_t StorageManager::getUsedSpace()
{
    return getFileSize(_filename);
}

/* Remove readings older than period_days using RTC timestamps */
void StorageManager::cleanup(time_t now, uint32_t period_days)
{
    if (!_initialized)
    {
        LOG_ERROR("Storage not initialized. Cleanup not possible");
        return;
    }

    if (!fileExists(_filename))
    {
        LOG_INFO("No datalog file to clean up");
        return;
    }

    time_t cutoff = now - (period_days * 24UL * 60UL * 60UL);
    String tempFilename = _filename + ".tmp";

    // Open original file for reading
    File readFile = SD.open(_filename.c_str(), FILE_READ);
    if (!readFile)
    {
        LOG_ERROR("Failed to open datalog for cleanup");
        return;
    }

    // Skip header line
    if (readFile.available())
        readFile.readStringUntil('\n');

    // Open temp file for writing
    File writeFile = SD.open(tempFilename.c_str(), FILE_WRITE);
    if (!writeFile)
    {
        LOG_ERROR("Failed to create temp file for cleanup");
        readFile.close();
        return;
    }

    // Write header to temp file
    writeFile.println("Timestamp,Temperature,Humidity,Pressure,Altitude");

    uint32_t kept = 0;
    uint32_t removed = 0;

    while (readFile.available())
    {
        String line = readFile.readStringUntil('\n');
        line.trim();
        if (line.length() == 0)
            continue;

        // Parse timestamp (first field)
        time_t timestamp = 0;
        sscanf(line.c_str(), "%lu", &timestamp);

        if (timestamp >= cutoff)
        {
            writeFile.println(line);
            kept++;
        }
        else
        {
            removed++;
        }
    }

    readFile.close();
    writeFile.close();

    // Delete original and rename temp
    deleteFile(_filename);
    SD.rename(tempFilename.c_str(), _filename.c_str());

    LOG_INFO("Cleanup: kept %lu, removed %lu readings (cutoff %lu days)",
             kept, removed, period_days);
}

bool StorageManager::storeComfortLog(const ComfortLog &log)
{
    if (!_initialized)
    {
        LOG_ERROR("Storage not initialized");
        return false;
    }

    const String comfortFile = COMFORT_FILENAME;

    // Create comfort file with header if it doesn't exist
    if (!fileExists(comfortFile))
    {
        File headerFile = SD.open(comfortFile.c_str(), FILE_WRITE);
        if (!headerFile)
        {
            LOG_ERROR("Failed to create comfort file");
            return false;
        }
        headerFile.println("Timestamp,ComfortLevel");
        headerFile.flush();
        headerFile.close();
    }

    File dataFile = SD.open(comfortFile.c_str(), FILE_APPEND);
    if (!dataFile)
    {
        LOG_ERROR("Failed to open comfort file for writing");
        return false;
    }

    dataFile.printf("%lu,%d\n", static_cast<unsigned long>(log.timestamp), static_cast<int>(log.level));
    dataFile.close();

    LOG_INFO("Comfort logged: %lu, level=%d", static_cast<unsigned long>(log.timestamp), static_cast<int>(log.level));
    return true;
}

bool StorageManager::logDebug(const char* tag, const char* message)
{
    if (!_initialized) return false;

    File file = SD.open(DEBUG_LOG_FILENAME, FILE_APPEND);
    if (!file) return false;

    file.printf("[%lu] [%s] %s\n", millis() / 1000, tag, message);
    file.close();
    return true;
}

bool StorageManager::getComfortLogsSince(time_t timestamp, std::vector<ComfortLog> &logs)
{
    logs.clear();

    if (!_initialized)
    {
        LOG_ERROR("Storage not initialized");
        return false;
    }

    const String comfortFile = COMFORT_FILENAME;

    if (!fileExists(comfortFile))
    {
        return true; // No comfort logs yet, empty is valid
    }

    File file = SD.open(comfortFile.c_str(), FILE_READ);
    if (!file)
    {
        LOG_ERROR("Failed to open comfort file");
        return false;
    }

    // Skip header
    if (file.available())
    {
        file.readStringUntil('\n');
    }

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        ComfortLog log;
        unsigned long ts;
        int level;
        if (sscanf(line.c_str(), "%lu,%d", &ts, &level) == 2)
        {
            log.timestamp = static_cast<time_t>(ts);
            log.level = static_cast<ComfortLevel>(level);
            if (log.timestamp >= timestamp)
            {
                logs.push_back(log);
            }
        }
    }

    file.close();
    LOG_INFO("Retrieved %d comfort logs since %lu", logs.size(), static_cast<unsigned long>(timestamp));
    return true;
}

/* This function is the core reading function helper used whenever a read operation happens.
- For shouldInclude possibilities are:
    - Include only Recent Readings
    - Include All Readings

- For shouldStop possibilities are:
    - Only N Readings
    - All Readings
*/
bool StorageManager::processReadings(std::vector<SensorReading> &readings,
                                     bool (*shouldInclude)(const SensorReading &, void *),
                                     bool (*shouldStop)(const std::vector<SensorReading> &, void *),
                                     void *context)
{
    readings.clear();

    if (!_initialized)
    {
        LOG_ERROR("Initialization Error");
        return false;
    }

    File dataFile = SD.open(_filename.c_str(), FILE_READ);
    if (!dataFile)
    {
        // TODO I want have the error handling uniform and important: memory efficient. the strings should be only defined once in the program.
        // TODO But I will optimize such things later. For know I want to focus on core functionality.
        LOG_INFO("Failed to open file for reading");
        return false;
    }

    if (readings.capacity() == 0)
    {
        readings.reserve(100); // TODO Some random amount for now.
        LOG_DEBUG("Reserved capacitor: %u", readings.capacity());
    }

    // DEBUG: Check file size
    LOG_DEBUG("File size: %u bytes", dataFile.size());

    // Skip header
    if (dataFile.available())
    {
        // dataFile.readStringUntil('\n');
        String header = dataFile.readStringUntil('\n');
        LOG_DEBUG("Header skipped '%s'", header.c_str());
    }

    // DEBUG: Check position after header
    LOG_DEBUG("File position after header: %u, available: %d",
              dataFile.position(), dataFile.available());

    String line;
    size_t insertPos = 0; // Circular Buffer Insert Position

    // TODO This while loop got never executed in my example.
    while (dataFile.available())
    {
        line = dataFile.readStringUntil('\n');
        LOG_DEBUG("Current Line: %s", line.c_str());
        line.trim();

        if (line.length() == 0){
            LOG_DEBUG("LINE SKIPPED"); 
            continue;
        }

        SensorReading reading = parseReading(line);

        LOG_DEBUG("Reading parsed");
        // OKAY I GOT IT. // TODO I completely remove the valid in the future
        if (!reading.isValid){
            LOG_DEBUG("LINE SKIPPED"); 
            continue;
        }
        if (!shouldInclude(reading, context))
            continue;

        LOG_DEBUG("reading included.");

        if (readings.size() < readings.capacity())
        {
            readings.push_back(reading);
            LOG_DEBUG("Pushback: ts=%lu, temp=%.2f°C, hum=%.2f%%, press=%.2fhPa",
                      reading.timestamp,
                      reading.temperature,
                      reading.humidity,
                      reading.pressure);
        }
        else
        {
            readings[insertPos] = reading;
            insertPos = (insertPos + 1) % readings.capacity();
        }

        // Use Lambda function shouldStop to decide when to stop.
        if (shouldStop && shouldStop(readings, context))
        {
            break; // Stop early if we have enough
        }
    }

    dataFile.close();

    LOG_INFO("Retrieved %u readings", readings.size());

    return true;
}

// Private helper functions
bool StorageManager::createHeaderIfNeeded(const String &filename)
{
    //if(!fileExists(filename)){
    //    LOG_ERROR("Can not create Header on non existing file.");
    //    return false;
    //}

     if (fileExists(_filename))
    {
        LOG_INFO("Log file exists, header already present");
        return true;
    }

    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if(!dataFile){
        LOG_ERROR("Can not open File for writing.");
        return false;
    }

    // Write CSV header
    LOG_INFO("DateTime,Temperature(C),Humidity(%),Pressure(hPa),Altitude(m)");
    dataFile.println("Timestamp,Temperature,Humidity,Pressure,Altitude");
    dataFile.flush();
    dataFile.close();

    delay(50);

    LOG_INFO("File '%s' created: Success!", _filename.c_str());
    return true;
}

void StorageManager::listDirectory(const String &path, uint8_t levels)
{
    // TODO This function is a little to long for my taste.
    File root = SD.open(path.c_str());
    if (!root)
    {
        LOG_ERROR("Failed to open directory: %s", path);
        return;
    }

    if (!root.isDirectory())
    {
        LOG_ERROR("Path is not a directory: %s", path);
        root.close();
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        // Print indentation based on depth
        for (uint8_t i = 0; i < (2 - levels); i++)
        {
            LOG_INFO("  ");
        }

        if (file.isDirectory())
        {
            LOG_INFO("[FILE] %s (%u bytes)", file.name(), file.size());

            // Recursively list subdirectories if levels > 0
            if (levels > 0)
            {
                String subPath = path;
                if (!subPath.endsWith("/"))
                {
                    subPath += "/";
                }
                subPath += file.name();
                listDirectory(subPath, levels - 1);
            }
        }
        else
        {
            LOG_INFO("[FILE] %s (%u bytes)", file.name(), file.size());
        }

        file = root.openNextFile();
    }

    root.close();
}

// Helper function to parse the reading of the CSV Line.
// It is not very professionally implemented
SensorReading StorageManager::parseReading(const String &line)
{
    LOG_INFO("Parse: %s", line.c_str());
    SensorReading reading;

    const char *str = line.c_str();

    // Try parsing 5 fields (with altitude) first
    int parsed = sscanf(str, "%lu,%f,%f,%f,%f",
                        &reading.timestamp,
                        &reading.temperature,
                        &reading.humidity,
                        &reading.pressure,
                        &reading.altitude);

    // Fall back to 4 fields (legacy format without altitude)
    if (parsed < 4) {
        parsed = sscanf(str, "%lu,%f,%f,%f",
                        &reading.timestamp,
                        &reading.temperature,
                        &reading.humidity,
                        &reading.pressure);
        reading.altitude = 0;
    }

    reading.isValid = (parsed >= 4) &&
                      (reading.timestamp > 0) &&
                      (reading.temperature > -50 && reading.temperature < 100) &&
                      (reading.humidity >= 0 && reading.humidity <= 100) &&
                      (reading.pressure > 900 && reading.pressure < 1100);

    LOG_DEBUG("PARSE RESULT: valid=%s, fields=%d | ts=%lu, temp=%.2f°C, hum=%.2f%%, press=%.2fhPa, alt=%.0fm",
              reading.isValid ? "YES" : "NO",
              parsed,
              reading.timestamp,
              reading.temperature,
              reading.humidity,
              reading.pressure,
              reading.altitude);
    return reading;
}

size_t StorageManager::estimateMemoryNeeded(uint16_t readingCount)
{
    // Include some Overhead to the memory estimation.
    return readingCount * sizeof(SensorReading) + 1024;
}

bool StorageManager::checkMemoryAvailable(size_t requiredBytes)
{
    size_t freeHeap = ESP.getFreeHeap();
    LOG_INFO("Available Memory: %d, Requested: %d", freeHeap, requiredBytes);

    if(!freeHeap < requiredBytes){
        LOG_ERROR("Insufficient memory: need %u, have %u", requiredBytes, freeHeap);
        return false;
    }
    return true;
}

[[DEPRECATED]]
bool StorageManager::hasEnoughMemory(uint16_t readingCount)
{
    size_t needed = estimateMemoryNeeded(readingCount);
    // ESP32 specific function to read the free memory. Might be changed in the future to be platform independent.
    size_t available = ESP.getFreeHeap();
    LOG_INFO("Available Memory: %d, Needed: %d, requested Count: %d", available, needed, readingCount);
    return available > (needed * 1.2); // Include a 50% safety margin.
}

uint16_t StorageManager::estimateLineCount()
{
    size_t fileSize = getFileSize(_filename);
    if (fileSize == 0) return 0;

    // Measure header size
    File file = SD.open(_filename, FILE_READ);
    if (!file) return 0;
    
    size_t headerStart = file.position();
    file.readStringUntil('\n');
    size_t headerSize = file.position() - headerStart;
    file.close();
    
    // Calculate data size
    if (fileSize <= headerSize) return 0;
    size_t dataSize = fileSize - headerSize;
    
    // actual line format: "1704067200,20.00,50.00,1013.00\n"
    // = 10 + 1 + 5 + 1 + 5 + 1 + 7 + 1 = 31 bytes typical
    const size_t BYTES_PER_LINE = 31;
    
    uint16_t lineCount = dataSize / BYTES_PER_LINE;
    
    LOG_DEBUG("File: %u B, header: %u B, data: %u B → %u lines", 
              fileSize, headerSize, dataSize, lineCount);
    
    return lineCount;
}

// shouldInclude filters
bool includeAllReadings(const SensorReading &reading, void *context)
{
    LOG_DEBUG("");
    return true; // Include everything
}

bool includeRecentReadings(const SensorReading &reading, void *context)
{
    LOG_DEBUG("");
    SinceContext *ctx = (SinceContext *)context;
    return reading.timestamp >= ctx->timestamp;
}

// shouldStop filters
bool stopAfterNReadings(const std::vector<SensorReading> &readings, void *context)
{
    LOG_DEBUG("");
    MaxCountContext *ctx = (MaxCountContext *)context;
    return readings.size() >= ctx->maxCount;
}

bool neverStop(const std::vector<SensorReading> &readings, void *context)
{
    LOG_DEBUG("");
    return false; // Keep going until end of file
}

#endif // !MOCK