// storage.cpp
#include "storage_manager.h"
#include <logger.h>

StorageManager::StorageManager() : _cs(SD_CS), _initialized(false), _sd_card_present(false)
{
    _filename = SD_FILENAME;
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
}
StorageManager::StorageManager(uint8_t cs)
    : _initialized(false), _sd_card_present(false), _cs(cs)
{
    _filename = SD_FILENAME;
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
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

    // Ensure file exists with header
    result = createHeaderIfNeeded();

    if (!result) {
        LOG_ERROR("Failed to ensure CSV file exists");
        _initialized = false;
        return false;
    }

    _initialized = true;
    LOG_INFO("SD initialized successfully");
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
        // Close all files before ending
        File root = SD.open("/");
        if (root) {
            root.close();
        }
        SD.end();
        delay(50);
    }
    pinMode(_cs, OUTPUT);
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
        LOG_ERROR("Initialization Error");
        return false;
    }

    //* If it doesn't exist it simply writes to it, creates it and doesn't creates the header.
    if(!fileExists(_filename)){
        LOG_WARN("File doesn't exist.");
        if(!createHeaderIfNeeded()){
            LOG_ERROR("Failed to create File");
            return false;
        }
    }

    // Open file for writing
    delay(50);
    File dataFile = SD.open(_filename.c_str(), FILE_APPEND);
    if (!dataFile)
    {
        LOG_ERROR("Failed to open file for writing");
        return false;
    }

    // Write CSV data: Timestamp, Temperature, Humidity, Pressure
    // Pls note that the Timestamp is written as time_t variable
    // Example CSV Line:
    // 1710513045,20.50,55.30,1013.25,
    dataFile.printf("%lu,%.2f,%.2f,%.2f\n",
                    reading.timestamp,
                    reading.temperature,
                    reading.humidity,
                    reading.pressure);

    dataFile.close();

    // LOG_INFO("Data logged to SD card, File: %s", _filename.c_str());
    LOG_INFO("Logged: %lu, %.2f°C %.0f%% %.0fhPa -> %s",
             reading.timestamp,
             reading.temperature,
             reading.humidity,
             reading.pressure,
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

    /// The next section is there to determine the HEADER SIZE and BYTES PER LINE

    size_t startPos = file.position();
    String header = file.readStringUntil('\n');
    const size_t HEADER_SIZE = file.position() - startPos;

    LOG_DEBUG("Header: %s, size: %d", header.c_str(), HEADER_SIZE);

    if (!file.available())
    {
        LOG_WARN("File is empty.");
        file.close();
        return true; // Because reading an empty file is expected to be true
    }

    size_t lineStartPos = file.position();
    String firstLine = file.readStringUntil('\n');
    size_t BYTES_PER_LINE = file.position() - lineStartPos;

    LOG_DEBUG("First line: %s, size: %d", firstLine.c_str(), BYTES_PER_LINE);

    size_t fileSize = file.size();
    size_t dataSize = fileSize - HEADER_SIZE;
    size_t totalLines = dataSize / BYTES_PER_LINE;

    LOG_DEBUG("File size: %d, dataSize: %d, Total lines: %d", fileSize, dataSize, totalLines);

    if (totalLines == 0)
    {
        file.close();
        return true;
    }

    // Step 4: Seek to position of the last N lines
    uint16_t linesToRead = (totalLines < maxCount) ? totalLines : maxCount;

    // Seek to the Last Line and then Read backwards
    for(int i=0; i<linesToRead; i++)
    {
        size_t lineIndex = totalLines - 1 - i; // Last Line = totalLines - 1;
        size_t seekPos = HEADER_SIZE + lineIndex * BYTES_PER_LINE;

        LOG_DEBUG("Reading line %d/%d at position %d", i+1, linesToRead, seekPos);

        file.seek(seekPos);
        String line = file.readStringUntil('\n');
        line.trim();

        if(line.length() == 0){
            LOG_WARN("Empty line at position %d", seekPos);
            continue;
        }

        LOG_DEBUG("Parsing: '%s'", line.c_str());
        SensorReading reading = parseReading(line);
        readings.push_back(reading);
    }

    file.close();
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
    if(fileExists(filename));
    return false;
}

bool StorageManager::fileExists(const String &filename)
{
    return SD.exists(filename.c_str());
}

uint32_t StorageManager::getFileSize(const String &filename)
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

    uint32_t size = file.size();
    file.close();
    return size;
}

void StorageManager::printCardInfo()
{
    if (!_initialized)
    {
        LOG_ERROR("Initialization Error");
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

    return true;
}

bool StorageManager::resetFile()
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

// Try to open and close the root directory
bool StorageManager::testConnection()
{
    if (!_initialized)
        return false;

    File root = SD.open("/");
    if (!root)
        return false;

    root.close();
    return true;
}

// TODO Add a Test for this method
bool StorageManager::testSDCardHealth()
{
    // Try to create and delete a test file
    File test = SD.open("/_test.tmp", FILE_WRITE);
    test.close();
    if (!test) {
        LOG_ERROR("SD health check failed: cannot create file");
        return false;
    }
    //test.close();
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
uint32_t StorageManager::getUsedSpace()
{
    return getFileSize(_filename);
}

/* Remove reading older than 30 days */
void StorageManager::cleanup()
{
    // TODO Add adjustable cleanup_period
    uint32_t cutoffTime = millis() - (30UL * 24UL * 60UL * 60UL * 1000UL);

    // The Cleanup procedure would require to rewrite the file to remove old entries
    // and is therefore not implemented for now
    // TODO The Cleanup Function is not implemented yet. Needs implementation
    LOG_INFO("Cleanup: would remove readings older than %lu\n", cutoffTime);
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
bool StorageManager::createHeaderIfNeeded()
{
    if (fileExists(_filename))
    {
        LOG_INFO("Log file exists, header already present");
        return true;
    }

    LOG_INFO("Create new '%s' file with header:", _filename.c_str());

    // Create the new file
    File dataFile = SD.open(_filename.c_str(), FILE_WRITE);
    if (!dataFile)
    {
        LOG_ERROR("File creation failed!");
        return false;
    }

    // Write CSV header
    LOG_INFO("DateTime,Temperature(C),Humidity(%),Pressure(hPa)");
    dataFile.println("Timestemp,Temperature,Humidity,Pressure");
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

    // Example Parse: 1710513045,20.50,55.30,1013.25,
    //  Convert to C string for safer parsing
    const char *str = line.c_str();

    // Parse using sscanf (much safer on ESP32)
    int parsed = sscanf(str, "%lu,%f,%f,%f",
                        &reading.timestamp,
                        &reading.temperature,
                        &reading.humidity,
                        &reading.pressure);
    // reading.isValid = true; //** I really need this during development. I might want to change this from the SensorData */

    // TODO Refactor this into its own function
    reading.isValid = (parsed == 4) &&
                      (reading.timestamp > 0) &&
                      (reading.temperature > -50 && reading.temperature < 100) && // Sanity check
                      (reading.humidity >= 0 && reading.humidity <= 100) &&
                      (reading.pressure > 900 && reading.pressure < 1100);

    LOG_DEBUG("PARSE RESULT: valid=%s, fields=%d | ts=%lu, temp=%.2f°C, hum=%.2f%%, press=%.2fhPa",
              reading.isValid ? "YES" : "NO",
              parsed,
              reading.timestamp,
              reading.temperature,
              reading.humidity,
              reading.pressure);
    return reading;
}

size_t StorageManager::estimateMemoryNeeded(uint16_t readingCount)
{
    // Include some Overhead to the memory estimation.
    return readingCount * sizeof(SensorReading) + 1024;
}

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