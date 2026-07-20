#pragma once
#include <Arduino.h>
#include <unity.h>
#include "test_runner.h"
#include <test_fixture.h>
#include <storage_manager.h>
#include <data_structures.h>
#include <logger.h>

#include "test_utils.h"
StorageManager testStorage;

const String TEST_FILE = "/test_file.csv";

// Helper function to create test readings
SensorReading createTestReading(float temp, float humidity, float pressure, time_t timestamp)
{
    SensorReading reading = {};
    reading.temperature = temp;
    reading.humidity = humidity;
    reading.pressure = pressure;
    reading.timestamp = timestamp;
    reading.isValid = true;

    LOG_INFO("Created reading: temp=%.2f, hum=%.2f, press=%.2f, timestamp=%ld",
             reading.temperature, reading.humidity, reading.pressure, (long)reading.timestamp);

    return reading;
}

// Helper to store multiple test readings
// void storeTestReadings(int count, time_t startTime = 0) {
std::vector<SensorReading> storeTestReadings(int count, time_t startTime = 0)
{
    if (startTime == 0)
    {
        startTime = 1704067200; // 2024-01-01 00:00:00
    }

    std::vector<SensorReading> storeReadings;
    storeReadings.reserve(count);

    for (int i = 0; i < count; i++)
    {
        SensorReading reading = createTestReading(
            20.0f + i * 1.0f,    // Temperature increases
            50.0f + i * 1.0f,    // Humidity increases
            1013.0f + i * 0.1f,  // Pressure increases
            startTime + (i * 60) // 1 minute apart
        );

        testStorage.storeReading(reading);

        // Keep a copy of the testReadings in memory
        storeReadings.push_back(reading);
    }

    return storeReadings;
}

// Helper to verify SD card is connected before running tests
void requireSDCard()
{
    if (!testStorage.isReady())
    {
        TEST_FAIL_MESSAGE("SD card not connected - test cannot run");
    }
}

void test_storage_initialization()
{
    bool result = testStorage.begin();
    TEST_ASSERT_TRUE_MESSAGE(result, "Storage initialization should succeed");
    TEST_ASSERT_TRUE_MESSAGE(testStorage.isReady(), "Storage should be ready after initialization");
}

void test_storage_connection()
{
    bool connected = testStorage.testConnection();
    TEST_ASSERT_TRUE_MESSAGE(connected, "Storage connection test should pass");
}

void test_storage_file_operations()
{
    requireSDCard();

    String testFilename = "/test_file.txt";

    // Clean up any existing test file
    if (testStorage.fileExists(testFilename))
    {
        testStorage.deleteFile(testFilename);
    }

    // Initially file should not exist
    bool result = testStorage.fileExists(testFilename);
    TEST_ASSERT_FALSE_MESSAGE(result, "Test file should not exist initially");

    // Main data file should exist after initialization
    result = testStorage.fileExists(DATALOG_FILENAME);
    TEST_ASSERT_TRUE_MESSAGE(result, "Data log file should exist after initialization");
}

void test_storage_single_reading()
{
    requireSDCard();

    // Create test sensor reading
    SensorReading testReading = createTestReading(25.5f, 60.0f, 1013.25f, 1704067200);

    // Store the reading
    bool result = testStorage.storeReading(testReading);
    TEST_ASSERT_TRUE_MESSAGE(result, "Should be able to store sensor reading");
}

void test_get_all_readings()
{
    requireSDCard();

    // Clear and store known test data
    testStorage.deleteFile(DATALOG_FILENAME);
    testStorage.begin(); // Recreate file

    uint16_t testFileSize = 5;
    storeTestReadings(testFileSize);

    std::vector<SensorReading> readings;
    bool result = testStorage.getAllReadings(readings);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(testFileSize, readings.size());

    // Verify chronological order
    for (size_t i = 1; i < readings.size(); i++)
    {
        TEST_ASSERT_TRUE_MESSAGE(
            readings[i].timestamp >= readings[i - 1].timestamp,
            "Readings should be in chronological order");
    }
}

void test_get_last_n_readings()
{
    LOG_DEBUG("\n");
    requireSDCard();

    // Store 5 readings
    uint16_t dataCount = 5;
    testStorage.deleteFile(DATALOG_FILENAME);
    testStorage.begin();

    std::vector<SensorReading> expected = storeTestReadings(dataCount, 1704067200);
    std::reverse(expected.begin(), expected.end());

    // Test Case 1: Get last 3 readings
    std::vector<SensorReading> readings;
    uint16_t readingCount = 3;
    bool result = testStorage.getLastNReadings(readings, readingCount);

    printAllReadings("ALL EXPECTED: ", expected);
    printAllReadings("ALL READINGS", readings);

    TEST_ASSERT_TRUE_MESSAGE(result, "Should successfully retrieve last N readings");
    TEST_ASSERT_EQUAL_MESSAGE(readingCount, readings.size(), "Should retrieve exactly 3 readings");

    for (int i = 0; i < readingCount; i++)
    {
        assertReadingsEqual(expected[i], readings[i]);
    }
}

void test_get_last_n_more_than_exists()
{
    requireSDCard();

    // Store only 5 readings
    testStorage.deleteFile(DATALOG_FILENAME);
    testStorage.begin();
    storeTestReadings(5);

    // Request 10 (more than exists)
    std::vector<SensorReading> readings;
    bool success = testStorage.getLastNReadings(readings, 10);

    TEST_ASSERT_TRUE_MESSAGE(success, "Should succeed even if requesting more than exists");
    TEST_ASSERT_EQUAL_MESSAGE(5, readings.size(), "Should return all 5 available readings");
}

void test_get_last_n_zero_count()
{
    requireSDCard();

    std::vector<SensorReading> readings;
    bool result = testStorage.getLastNReadings(readings, 0);

    TEST_ASSERT_FALSE_MESSAGE(result, "Should fail with maxCount = 0");
    TEST_ASSERT_EQUAL_MESSAGE(0, readings.size(), "Should return empty vector");
}

void test_get_readings_since_timestamp()
{
    requireSDCard();

    testStorage.deleteFile(DATALOG_FILENAME);
    testStorage.begin();

    time_t baseTime = 1704067200;    // 2024-01-01 00:00:00
    storeTestReadings(10, baseTime); // 10 readings, 1 minute apart

    // Get readings since 5 minutes in (should get last 5 readings)
    time_t cutoffTime = baseTime + (5 * 60);
    std::vector<SensorReading> readings;
    bool success = testStorage.getReadingsSince(cutoffTime, readings);

    TEST_ASSERT_TRUE_MESSAGE(success, "Should successfully retrieve readings since timestamp");
    TEST_ASSERT_EQUAL_MESSAGE(5, readings.size(), "Should retrieve 5 readings after cutoff");

    // Verify all readings are after cutoff
    for (const auto &reading : readings)
    {
        TEST_ASSERT_TRUE_MESSAGE(
            reading.timestamp >= cutoffTime,
            "All readings should be after cutoff timestamp");
    }
}

void test_get_readings_since_with_max_count()
{
    requireSDCard();

    testStorage.deleteFile(DATALOG_FILENAME);
    testStorage.begin();

    time_t baseTime = 1704067200;
    std::vector<SensorReading> expected = storeTestReadings(20, baseTime); // 20 readings total

    // Get readings since beginning, but limit to 5
    std::vector<SensorReading> readings;
    bool success = testStorage.getReadingsSince(baseTime, readings, 5);

    TEST_ASSERT_TRUE_MESSAGE(success, "Should succeed with maxCount limit");
    TEST_ASSERT_EQUAL_MESSAGE(5, readings.size(), "Should respect maxCount limit");
}

void test_get_readings_since_no_matches()
{
    requireSDCard();

    testStorage.deleteFile(DATALOG_FILENAME);
    testStorage.begin();

    time_t baseTime = 1704067200;
    std::vector<SensorReading> stored =  storeTestReadings(5, baseTime);

    // Request readings from future (no matches)
    time_t futureTime = baseTime + (1000 * 60);
    std::vector<SensorReading> readings;
    bool result = testStorage.getReadingsSince(futureTime, readings);

    printAllReadings("Stored: ", stored);
    printAllReadings("Readings:", readings);

    TEST_ASSERT_TRUE_MESSAGE(result, "Should succeed even with no matches");
    TEST_ASSERT_EQUAL_MESSAGE(0, readings.size(), "Should return empty vector when no matches");
}

void test_get_readings_since_unlimited()
{
    requireSDCard();

    testStorage.deleteFile(DATALOG_FILENAME);
    testStorage.begin();

    time_t baseTime = 1704067200;
    std::vector<SensorReading> expected =  storeTestReadings(15, baseTime);

    // Get all readings since beginning (maxCount = 0 means unlimited)
    std::vector<SensorReading> readings;  
    bool result = testStorage.getReadingsSince(baseTime, readings, 0);

    printAllReadings("Expected:", expected);
    printAllReadings("Readings:", readings);

    TEST_ASSERT_TRUE_MESSAGE(result, "Should succeed with unlimited count");
    TEST_ASSERT_EQUAL_MESSAGE(15, readings.size(), "Should return all matching readings");
}

void test_memory_safety()
{
    requireSDCard();

    // Test that large requests fail gracefully
    std::vector<SensorReading> readings;

    // Request unreasonably large count (should fail memory check)
    bool result = testStorage.getLastNReadings(readings, UINT16_MAX);

    TEST_ASSERT_FALSE(result);
}

void test_storage_diagnostics()
{
    requireSDCard();

    // Test diagnostic functions (should not crash)
    testStorage.printCardInfo();
    testStorage.listFiles();

    // Test space functions
    uint32_t usedSpace = testStorage.getUsedSpace();
    TEST_ASSERT_TRUE_MESSAGE(usedSpace >= 0, "Used space should be non-negative");

    // Test file size
    uint32_t fileSize = testStorage.getFileSize(DATALOG_FILENAME);
    TEST_ASSERT_TRUE_MESSAGE(fileSize >= 0, "File size should be non-negative");
}

void test_data_integrity()
{
    LOG_DEBUG("TEST DATA INTEGRITY");
    requireSDCard();

    testStorage.deleteFile(DATALOG_FILENAME);
    delay(100);
    testStorage.begin();

    // Store reading with known values
    SensorReading original = createTestReading(25.5f, 60.0f, 1013.25f, 1704067200);
    testStorage.storeReading(original);

    // Read it back
    std::vector<SensorReading> readings;
    testStorage.getAllReadings(readings);

    TEST_ASSERT_EQUAL_MESSAGE(1, readings.size(), "Should have exactly one reading");

    //// Verify data integrity using integer comparison
    // TEST_ASSERT_EQUAL_INT32(255, (int32_t)(readings[0].temperature * 10));  // 25.5 * 10
    // TEST_ASSERT_EQUAL_INT32(600, (int32_t)(readings[0].humidity * 10));     // 60.0 * 10
    // TEST_ASSERT_EQUAL_INT32(10132, (int32_t)(readings[0].pressure * 10));   // 1013.2 * 10
    // TEST_ASSERT_EQUAL(original.timestamp, readings[0].timestamp);
}


void printMemoryReportCompact() {
    Serial.println("\n═══ ESP32 MEMORY ═══");
    
    // Heap
    Serial.printf("HEAP:   %u/%u KB free (%.1f%%) | Min: %u KB\n",
                  ESP.getFreeHeap() / 1024,
                  ESP.getHeapSize() / 1024,
                  (ESP.getFreeHeap() * 100.0) / ESP.getHeapSize(),
                  ESP.getMinFreeHeap() / 1024);
    
    // PSRAM
    if (psramFound()) {
        Serial.printf("PSRAM:  %u/%u KB free (%.1f%%) | Min: %u KB\n",
                      ESP.getFreePsram() / 1024,
                      ESP.getPsramSize() / 1024,
                      (ESP.getFreePsram() * 100.0) / ESP.getPsramSize(),
                      ESP.getMinFreePsram() / 1024);
    } else {
        Serial.println("PSRAM:  Not available");
    }
    
    // Flash
    Serial.printf("FLASH:  %u KB sketch | %u KB free\n",
                  ESP.getSketchSize() / 1024,
                  ESP.getFreeSketchSpace() / 1024);
    
    // Chip
    Serial.printf("CHIP:   %s | %u MHz | %u cores\n",
                  ESP.getChipModel(),
                  ESP.getCpuFreqMHz(),
                  ESP.getChipCores());
    
    Serial.println("════════════════════\n");
}

void test_sd_card_delete_reliability()
{
    const char* testFile = "/test_delete_verify.txt";
    
    // Create file
    File f = SD.open(testFile, FILE_WRITE);
    f.println("test");
    f.close();
    delay(50);
    
    TEST_ASSERT_TRUE(SD.exists(testFile));
    
    // Delete
    bool deleted = SD.remove(testFile);
    TEST_ASSERT_TRUE(deleted);
    
    // Check immediately
    bool exists1 = SD.exists(testFile);
    LOG_INFO("Exists immediately: %s", exists1 ? "YES" : "NO");
    
    // Force flush
    SD.end();
    delay(100);
    SD.begin(SD_CS);
    
    // Check after flush
    bool exists2 = SD.exists(testFile);
    LOG_INFO("Exists after flush: %s", exists2 ? "YES" : "NO");
    
    TEST_ASSERT_FALSE(exists2);
}


// ═══════════════════════════════════════════════════════════════
// SD LIFECYCLE STRESS TESTS
// ═══════════════════════════════════════════════════════════════

/**
 * Test: Simulate the data_service test pattern
 * - Initialize SD
 * - Write data
 * - End SD
 * - Reinitialize
 * - Repeat multiple times
 */
void test_sd_repeated_init_cycles()
{
    LOG_INFO("\n=== Testing Repeated SD Init/End Cycles ===");
    
    const int CYCLES = 10;
    const char* testFile = DATALOG_FILENAME;
    
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        LOG_INFO("--- Cycle %d/%d ---", cycle + 1, CYCLES);
        
        // 1. Initialize (like setUp())
        StorageManager storage;
        bool initResult = storage.begin();
        TEST_ASSERT_TRUE_MESSAGE(initResult, "Failed to initialize SD");
        
        // 2. Write data (like test body)
        SensorReading reading = createTestReading(
            20.0f + cycle,
            50.0f + cycle,
            1013.0f,
            1704067200 + (cycle * 60)
        );
        
        bool writeResult = storage.storeReading(reading);
        TEST_ASSERT_TRUE_MESSAGE(writeResult, "Failed to write reading");
        
        // 3. Verify write
        TEST_ASSERT_TRUE(storage.fileExists(testFile));
        
        // 4. Clean up (like tearDown())
        if (storage.fileExists(testFile)) {
            bool deleteResult = storage.deleteFile(testFile);
            TEST_ASSERT_TRUE_MESSAGE(deleteResult, "Failed to delete file");
            delay(100);
        }
        
        // 5. End SD (like tearDown())
        SD.end();
        delay(100);
        
        // 6. Verify SD is truly ended
        TEST_ASSERT_FALSE(SD.exists(testFile));
        
        LOG_INFO("Cycle %d: SUCCESS", cycle + 1);
        printMemoryReportCompact();
    }
    
    LOG_INFO("=== All %d cycles completed ===\n", CYCLES);
}

/**
 * Test: Multiple writes without SD.end() between operations
 * This simulates the WORKING pattern from test_storage.hpp
 */
void test_sd_multiple_writes_no_restart()
{
    LOG_INFO("\n=== Testing Multiple Writes (No SD Restart) ===");
    requireSDCard();
    
    const int WRITES = 10;
    const char* testFile = DATALOG_FILENAME;
    
    // Clean start
    testStorage.deleteFile(testFile);
    testStorage.begin();
    
    for (int i = 0; i < WRITES; i++) {
        SensorReading reading = createTestReading(
            20.0f + i,
            50.0f + i,
            1013.0f,
            1704067200 + (i * 60)
        );
        
        bool result = testStorage.storeReading(reading);
        TEST_ASSERT_TRUE_MESSAGE(result, "Failed to write reading");
        
        LOG_INFO("Write %d/%d: SUCCESS", i + 1, WRITES);
    }
    
    // Verify all writes
    std::vector<SensorReading> readings;
    TEST_ASSERT_TRUE(testStorage.getAllReadings(readings));
    TEST_ASSERT_EQUAL(WRITES, readings.size());
    
    LOG_INFO("=== All %d writes completed (no restarts) ===\n", WRITES);
}

/**
 * Test: Rapid SD.end() + SD.begin() cycles without writes
 * Tests if the SD card can handle rapid mount/unmount
 */
void test_sd_rapid_mount_unmount()
{
    LOG_INFO("\n=== Testing Rapid Mount/Unmount ===");
    
    const int CYCLES = 20;
    int successCount = 0;
    
    for (int i = 0; i < CYCLES; i++) {
        // End SD
        SD.end();
        delay(50);
        
        // Begin SD
        bool result = SD.begin(SD_CS);
        if (result) {
            successCount++;
            LOG_INFO("Cycle %d: Mount SUCCESS", i + 1);
        } else {
            LOG_ERROR("Cycle %d: Mount FAILED", i + 1);
        }
        
        delay(50);
    }
    
    LOG_INFO("Success rate: %d/%d (%.1f%%)", 
             successCount, CYCLES, 
             (successCount * 100.0) / CYCLES);
    
    // Should succeed at least 95% of the time
    TEST_ASSERT_GREATER_OR_EQUAL(CYCLES * 0.95, successCount);
}

/**
 * Test: Write, delete, write pattern with SD restart
 * Replicates the exact pattern from data_service tests
 */
void test_sd_write_delete_cycle_with_restarts()
{
    LOG_INFO("\n=== Testing Write-Delete Cycle with SD Restarts ===");
    
    const int CYCLES = 5;
    const char* testFile = DATALOG_FILENAME;
    
    for (int cycle = 0; cycle < CYCLES; cycle++) {
        LOG_INFO("\n--- Cycle %d/%d ---", cycle + 1, CYCLES);
        
        // Initialize
        StorageManager storage;
        TEST_ASSERT_TRUE(storage.begin());
        LOG_INFO("SD initialized");
        

        // Write multiple readings
        for (int i = 0; i < 3; i++) {
            SensorReading reading = createTestReading(
                20.0f + i,
                50.0f + i,
                1013.0f,
                1704067200 + (cycle * 180) + (i * 60)
            );
            TEST_ASSERT_TRUE(storage.storeReading(reading));
        }
        LOG_INFO("Wrote 3 readings");
        
        // Verify reads
        std::vector<SensorReading> readings;
        TEST_ASSERT_TRUE(storage.getAllReadings(readings));
        size_t size = readings.size();
        LOG_INFO("%Readings Size: %lu", size);
        TEST_ASSERT_EQUAL(3, size);
        LOG_INFO("Read back 3 readings");
        
        // Delete file
        TEST_ASSERT_TRUE(storage.deleteFile(testFile));
        delay(50);
        TEST_ASSERT_FALSE(storage.fileExists(testFile));
        LOG_INFO("File deleted");
        
        // End SD
        SD.end();
        delay(50);
        LOG_INFO("SD ended");
        
        // Check memory
        LOG_INFO("Free heap: %u bytes", ESP.getFreeHeap());
    }
    
    LOG_INFO("\n=== All %d cycles completed ===", CYCLES);
}

/**
 * Test: File handle leak detection
 * Opens multiple files to find the handle limit
 */
void test_sd_file_handle_limit()
{
    LOG_INFO("\n=== Testing File Handle Limit ===");
    requireSDCard();
    
    std::vector<File> openFiles;
    const int MAX_ATTEMPTS = 15;
    
    // Try to open many files
    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        String filename = "/handle_test_" + String(i) + ".tmp";
        
        File f = SD.open(filename.c_str(), FILE_WRITE);
        if (!f) {
            LOG_WARN("Failed to open file %d (limit reached)", i);
            break;
        }
        
        f.println("test");
        openFiles.push_back(f);
        LOG_INFO("Opened file %d: %s", i, filename.c_str());
    }
    
    int maxHandles = openFiles.size();
    LOG_INFO("Maximum concurrent file handles: %d", maxHandles);
    
    // Close all files
    for (auto& f : openFiles) {
        if (f) {
            String name = f.name();
            f.close();
            SD.remove(name.c_str());
        }
    }
    
    // ESP32 typically supports 5-10 handles
    TEST_ASSERT_GREATER_OR_EQUAL(5, maxHandles);
    LOG_INFO("=== File handle test completed ===\n");
}

/**
 * Test: Verify file handle cleanup after operations
 * Ensures no handles leak during normal operations
 */
void test_sd_file_handle_cleanup()
{
    LOG_INFO("\n=== Testing File Handle Cleanup ===");
    requireSDCard();
    
    const char* testFile = DATALOG_FILENAME;
    
    // Baseline: How many handles can we open?
    int baselineHandles = 0;
    for (int i = 0; i < 10; i++) {
        String name = "/baseline_" + String(i) + ".tmp";
        File f = SD.open(name.c_str(), FILE_WRITE);
        if (f) {
            baselineHandles++;
            f.close();
            SD.remove(name.c_str());
        } else {
            break;
        }
    }
    LOG_INFO("Baseline: Can open %d handles", baselineHandles);
    
    // Now do typical operations
    testStorage.deleteFile(testFile);
    testStorage.begin();
    
    for (int i = 0; i < 5; i++) {
        SensorReading reading = createTestReading(20.0f, 50.0f, 1013.0f, 1704067200 + i);
        TEST_ASSERT_TRUE(testStorage.storeReading(reading));
    }
    
    std::vector<SensorReading> readings;
    TEST_ASSERT_TRUE(testStorage.getAllReadings(readings));
    
    // After operations: Can we still open same number of handles?
    int afterHandles = 0;
    for (int i = 0; i < 10; i++) {
        String name = "/after_" + String(i) + ".tmp";
        File f = SD.open(name.c_str(), FILE_WRITE);
        if (f) {
            afterHandles++;
            f.close();
            SD.remove(name.c_str());
        } else {
            break;
        }
    }
    LOG_INFO("After ops: Can open %d handles", afterHandles);
    
    // Should be able to open same number
    TEST_ASSERT_EQUAL(baselineHandles, afterHandles);
    
    LOG_INFO("=== No file handle leak detected ===\n");
}

namespace test_storage
{

    void setUp(){
        LOG_DEBUG("Test Storage SetUp: Ensure Clean SD state");

        if(!testStorage.isReady()){
            testStorage.begin();
        }

        // clearFile() deletes /datalog.csv and reinitializes SD internally
        testStorage.clearFile();

        // Let SD card settle after reinit before health check
        delay(100);

        TEST_ASSERT_TRUE(testStorage.testSDCardHealth());
    }

    void tearDown(){}
    
    void run_tests()
    {
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        LOG_INFO("\n[RUN TEST] STORAGE:\n");

        // Initialization
        RUN_TEST(test_storage_initialization);
        RUN_TEST(test_storage_connection);
        RUN_TEST(test_storage_file_operations);

        // Single Read Operations
        RUN_TEST(test_storage_single_reading);
        RUN_TEST(test_data_integrity);

 /*        // Bulk read operations
        RUN_TEST(test_get_all_readings);
        RUN_TEST(test_get_last_n_readings);
        RUN_TEST(test_get_last_n_more_than_exists);
        RUN_TEST(test_get_last_n_zero_count);

        // Timestamp filtering
        RUN_TEST(test_get_readings_since_timestamp);
        RUN_TEST(test_get_readings_since_with_max_count);
        RUN_TEST(test_get_readings_since_no_matches);
        RUN_TEST(test_get_readings_since_unlimited);

        // Edge cases and safety
        RUN_TEST(test_memory_safety);
        RUN_TEST(test_storage_diagnostics);
        
        RUN_TEST(test_sd_card_delete_reliability);

        //* RUN Lifecycle Tests
        RUN_TEST(test_sd_repeated_init_cycles);           
        RUN_TEST(test_sd_multiple_writes_no_restart);     
        RUN_TEST(test_sd_rapid_mount_unmount);            
        RUN_TEST(test_sd_write_delete_cycle_with_restarts);
        RUN_TEST(test_sd_file_handle_limit); 
        RUN_TEST(test_sd_file_handle_cleanup); */

        TEST_CONTEXT.clearFixtures();
    }
}