#pragma once
#undef MOCK
#include <Arduino.h>
#include <unity.h>
#include "test_runner.h"
#include <test_fixture.h>
#include <sensor_manager.h>
#include <storage_manager.h>
#include <display_manager.h>
#include <display_service.h>
#include <rtc_manager.h>
#include <wifi_manager.h>
#include <data_structures.h>
#include <config.h>
#include <logger.h>
#include "test_utils.h"

// ── Hardware instances ────────────────────────────────────────────────
// testSensor, testDisplay, testStorage are defined in test_sensor.hpp,
// test_display.hpp, test_storage.hpp (included before this file).
WiFiManager testWifi;
RTCManager testRtc;
extern SensorManager testSensor;
extern DisplayManager testDisplay;
extern StorageManager testStorage;
DisplayService* testDisplayService = nullptr;

// ── Helpers ───────────────────────────────────────────────────────────

const char* comfortLevelToString(ComfortLevel level) {
    switch (level) {
        case ComfortLevel::TOO_COLD:    return "Too cold";
        case ComfortLevel::COLD:        return "Cold";
        case ComfortLevel::COMFORTABLE: return "Comfortable";
        case ComfortLevel::WARM:        return "Warm";
        case ComfortLevel::TOO_WARM:    return "Too warm";
        default:                        return "Unknown";
    }
}

const char* menuToString(DisplayMenu menu) {
    switch (menu) {
        case DisplayMenu::GRAPH_TEMP:      return "Graph Temp";
        case DisplayMenu::GRAPH_HUMIDITY:  return "Graph Humidity";
        case DisplayMenu::GRAPH_ALTITUDE:  return "Graph Altitude";
        case DisplayMenu::SETTINGS:        return "Settings";
        case DisplayMenu::OTA:             return "OTA";
        case DisplayMenu::SYNC_TIME:       return "Sync Time";
        case DisplayMenu::SLEEP:           return "Sleep";
        default:                           return "Unknown";
    }
}

SensorReading createHikingReading(float temp, float hum, float press, float alt, time_t ts) {
    SensorReading r;
    r.temperature = temp;
    r.humidity = hum;
    r.pressure = press;
    r.altitude = alt;
    r.timestamp = ts;
    r.isValid = true;
    return r;
}

// ── Phase 2: Sensor + Storage ─────────────────────────────────────────

void test_hiking_sensor_with_altitude() {
    Serial.println("\n[HIKING] test_hiking_sensor_with_altitude");

    SensorReading reading = testSensor.getReading();

    Serial.printf("  Temperature:  %.1f C\n", reading.temperature);
    Serial.printf("  Humidity:     %.1f%%\n", reading.humidity);
    Serial.printf("  Pressure:     %.1f hPa\n", reading.pressure);
    Serial.printf("  Altitude:     %.1f m\n", reading.altitude);
    Serial.printf("  Timestamp:    %ld\n", (long)reading.timestamp);
    Serial.printf("  Valid:        %s\n", reading.isValid ? "YES" : "NO");

    TEST_ASSERT_TRUE_MESSAGE(reading.isValid, "Reading should be valid");
    TEST_ASSERT_TRUE_MESSAGE(reading.temperature > -50.0f && reading.temperature < 100.0f,
                             "Temperature in range");
    TEST_ASSERT_TRUE_MESSAGE(reading.humidity >= 0.0f && reading.humidity <= 100.0f,
                             "Humidity in range");
    TEST_ASSERT_TRUE_MESSAGE(reading.pressure > 800.0f && reading.pressure < 1200.0f,
                             "Pressure in range");
    TEST_ASSERT_TRUE_MESSAGE(reading.altitude > -500.0f && reading.altitude < 10000.0f,
                             "Altitude in range");
    Serial.println("  -> PASS");
}

void test_hiking_fresh_start() {
    Serial.println("\n[HIKING] test_hiking_fresh_start");

    // Storage was already begun in run_tests — fresh start already happened.
    // Verify the file has the right header.
    String buff;
    bool ok = testStorage.readFile(DATALOG_FILENAME, buff, 128);
    TEST_ASSERT_TRUE_MESSAGE(ok, "Should read datalog file");

    Serial.printf("  Header content: %s", buff.c_str());

    // Check header contains "Altitude"
    TEST_ASSERT_TRUE_MESSAGE(buff.indexOf("Altitude") >= 0,
                             "CSV header should contain 'Altitude'");

    // Verify file is small (fresh = header only, no data rows yet)
    size_t fileSize = testStorage.getFileSize(DATALOG_FILENAME);
    Serial.printf("  File size: %u bytes (fresh = header only)\n", (unsigned)fileSize);
    TEST_ASSERT_TRUE_MESSAGE(fileSize < 200, "Fresh file should be small (header only)");

    Serial.println("  -> PASS");
}

void test_hiking_csv_roundtrip() {
    Serial.println("\n[HIKING] test_hiking_csv_roundtrip");

    time_t ts = testRtc.getEpochTime();
    SensorReading stored = createHikingReading(21.5f, 55.0f, 1012.3f, 442.7f, ts);
    Serial.printf("  Storing:       T=%.1f H=%.1f P=%.1f Alt=%.1f ts=%ld\n",
                  stored.temperature, stored.humidity, stored.pressure,
                  stored.altitude, (long)stored.timestamp);

    bool writeOk = testStorage.storeReading(stored);
    TEST_ASSERT_TRUE_MESSAGE(writeOk, "Store should succeed");

    // Read back the last reading
    std::vector<SensorReading> readings;
    bool readOk = testStorage.getLastNReadings(readings, 1);
    TEST_ASSERT_TRUE_MESSAGE(readOk, "Read should succeed");
    TEST_ASSERT_EQUAL_MESSAGE(1, readings.size(), "Should have 1 reading");

    SensorReading retrieved = readings[0];
    Serial.printf("  Retrieved:     T=%.1f H=%.1f P=%.1f Alt=%.1f ts=%ld\n",
                  retrieved.temperature, retrieved.humidity, retrieved.pressure,
                  retrieved.altitude, (long)retrieved.timestamp);

    assertFloatsEqual(stored.temperature, retrieved.temperature);
    assertFloatsEqual(stored.humidity, retrieved.humidity);
    assertFloatsEqual(stored.pressure, retrieved.pressure);
    assertFloatsEqual(stored.altitude, retrieved.altitude);
    TEST_ASSERT_EQUAL_MESSAGE((long)stored.timestamp, (long)retrieved.timestamp,
                              "Timestamp should match");

    Serial.println("  -> PASS");
}

void test_hiking_measurement_cycle() {
    Serial.println("\n[HIKING] test_hiking_measurement_cycle");

    unsigned long start = millis();

    // Simulate a measurement wake: read sensor, store, read back
    SensorReading reading = testSensor.getReading();
    TEST_ASSERT_TRUE_MESSAGE(reading.isValid, "Sensor reading should be valid");

    bool stored = testStorage.storeReading(reading);
    TEST_ASSERT_TRUE_MESSAGE(stored, "Storage should succeed");

    std::vector<SensorReading> verify;
    testStorage.getLastNReadings(verify, 1);
    TEST_ASSERT_EQUAL_MESSAGE(1, verify.size(), "Should retrieve 1 reading");

    unsigned long elapsed = millis() - start;

    Serial.printf("  Sensor + store + read: %lu ms\n", elapsed);
    Serial.printf("  Reading: T=%.1f H=%.1f P=%.1f Alt=%.1f\n",
                  reading.temperature, reading.humidity,
                  reading.pressure, reading.altitude);

    TEST_ASSERT_TRUE_MESSAGE(elapsed < 2000, "Measurement cycle should be under 2 seconds");

    Serial.println("  -> PASS");
}

// ── Phase 3: Comfort logging ──────────────────────────────────────────

void test_hiking_comfort_single() {
    Serial.println("\n[HIKING] test_hiking_comfort_single");

    time_t ts = testRtc.getEpochTime();
    ComfortLog log(ts, ComfortLevel::COMFORTABLE);

    Serial.printf("  Storing: ts=%ld level=%s\n", (long)ts,
                  comfortLevelToString(log.level));

    bool ok = testStorage.storeComfortLog(log);
    TEST_ASSERT_TRUE_MESSAGE(ok, "Comfort log store should succeed");

    std::vector<ComfortLog> logs;
    bool retrieved = testStorage.getComfortLogsSince(ts - 1, logs);
    TEST_ASSERT_TRUE_MESSAGE(retrieved, "Comfort log query should succeed");
    TEST_ASSERT_TRUE_MESSAGE(logs.size() >= 1, "Should have at least 1 comfort log");

    ComfortLog& first = logs.back();
    Serial.printf("  Retrieved: ts=%ld level=%s\n", (long)first.timestamp,
                  comfortLevelToString(first.level));

    TEST_ASSERT_EQUAL_MESSAGE((long)ts, (long)first.timestamp, "Timestamp should match");
    TEST_ASSERT_EQUAL_MESSAGE((int)ComfortLevel::COMFORTABLE, (int)first.level,
                              "Level should be COMFORTABLE");

    Serial.println("  -> PASS");
}

void test_hiking_comfort_multiple() {
    Serial.println("\n[HIKING] test_hiking_comfort_multiple");

    time_t base = testRtc.getEpochTime();
    ComfortLevel levels[] = {
        ComfortLevel::TOO_COLD, ComfortLevel::COLD, ComfortLevel::COMFORTABLE,
        ComfortLevel::WARM, ComfortLevel::TOO_WARM
    };

    for (int i = 0; i < 5; i++) {
        ComfortLog log(base + (i * 60), levels[i]);
        testStorage.storeComfortLog(log);
        Serial.printf("  Stored [%d]: ts=%ld level=%s\n", i,
                      (long)(base + i * 60), comfortLevelToString(levels[i]));
    }

    // Query since the 3rd entry (index 2) — should get 3 results
    time_t cutoff = base + (2 * 60);
    std::vector<ComfortLog> filtered;
    testStorage.getComfortLogsSince(cutoff, filtered);

    Serial.printf("  Query since %ld (entry #2) → %d results\n", (long)cutoff, (int)filtered.size());

    for (size_t i = 0; i < filtered.size(); i++) {
        Serial.printf("    [%d] ts=%ld level=%s\n", (int)i,
                      (long)filtered[i].timestamp,
                      comfortLevelToString(filtered[i].level));
    }

    TEST_ASSERT_TRUE_MESSAGE(filtered.size() >= 3, "Should have at least 3 results");

    // Verify all results are >= cutoff
    for (const auto& l : filtered) {
        TEST_ASSERT_TRUE_MESSAGE(l.timestamp >= cutoff,
                                 "All results should be >= cutoff timestamp");
    }

    Serial.println("  -> PASS");
}

// ── Phase 4: Display ──────────────────────────────────────────────────

void test_hiking_display_graph() {
    Serial.println("\n[HIKING] test_hiking_display_graph");

    // Build sample data: 10 temperature readings
    std::vector<float> temps = {18.0, 18.5, 19.2, 20.1, 20.5, 19.8, 19.0, 18.3, 17.5, 17.0};
    std::vector<time_t> timestamps;
    time_t base = testRtc.getEpochTime() - 3600; // 1 hour ago
    for (int i = 0; i < 10; i++) {
        timestamps.push_back(base + (i * 360));
    }

    Serial.printf("  Graph: TEMPERATURE, 10 points\n");
    Serial.printf("  Min: %.1f  Max: %.1f\n", 17.0f, 20.5f);
    for (size_t i = 0; i < temps.size(); i++) {
        Serial.printf("    [%d] %.1f C @ %ld\n", (int)i, temps[i], (long)timestamps[i]);
    }

    testDisplay.drawGraph("TEMPERATURE", "C", temps, timestamps, 16.0f, 22.0f);

    // If we got here without crash, the graph rendered
    TEST_ASSERT_TRUE_MESSAGE(testDisplay.isReady(), "Display should still be ready after graph");

    Serial.println("  Graph rendered on TFT");
    Serial.println("  -> PASS");
}

void test_hiking_display_menu() {
    Serial.println("\n[HIKING] test_hiking_display_menu");

    DisplayMenu items[] = {
        DisplayMenu::GRAPH_TEMP, DisplayMenu::GRAPH_HUMIDITY,
        DisplayMenu::GRAPH_ALTITUDE, DisplayMenu::SETTINGS,
        DisplayMenu::OTA, DisplayMenu::SYNC_TIME,
        DisplayMenu::SLEEP
    };

    for (int i = 0; i < 7; i++) {
        Serial.printf("  Rendering menu [%d]: %s\n", i, menuToString(items[i]));
        testDisplayService->showMenu(items[i]);
        delay(800); // Pause so user can see each item on the TFT
    }

    TEST_ASSERT_TRUE_MESSAGE(testDisplay.isReady(), "Display should be ready");
    Serial.println("  -> PASS");
}

void test_hiking_display_comfort_ui() {
    Serial.println("\n[HIKING] test_hiking_display_comfort_ui");

    ComfortLevel levels[] = {
        ComfortLevel::TOO_COLD, ComfortLevel::COLD, ComfortLevel::COMFORTABLE,
        ComfortLevel::WARM, ComfortLevel::TOO_WARM
    };

    for (int i = 0; i < 5; i++) {
        Serial.printf("  Rendering comfort [%d]: %s\n", i, comfortLevelToString(levels[i]));
        testDisplayService->showComfortUI(levels[i]);
        delay(800); // Pause so user can see each level on the TFT
    }

    TEST_ASSERT_TRUE_MESSAGE(testDisplay.isReady(), "Display should be ready");
    Serial.println("  -> PASS");
}

// ── Phase 5: End-to-end workflows ─────────────────────────────────────

void test_hiking_full_workflow() {
    Serial.println("\n[HIKING] test_hiking_full_workflow");

    // Step 1: Read sensors
    Serial.println("  [1/4] Reading sensors...");
    SensorReading reading = testSensor.getReading();
    TEST_ASSERT_TRUE_MESSAGE(reading.isValid, "Reading should be valid");
    Serial.printf("    T=%.1f H=%.1f P=%.1f Alt=%.1f\n",
                  reading.temperature, reading.humidity,
                  reading.pressure, reading.altitude);

    // Step 2: Store to SD
    Serial.println("  [2/4] Storing to SD...");
    bool stored = testStorage.storeReading(reading);
    TEST_ASSERT_TRUE_MESSAGE(stored, "Store should succeed");

    // Step 3: Read back
    Serial.println("  [3/4] Reading back from SD...");
    std::vector<SensorReading> verify;
    testStorage.getLastNReadings(verify, 1);
    TEST_ASSERT_EQUAL_MESSAGE(1, verify.size(), "Should retrieve 1 reading");
    assertFloatsEqual(reading.temperature, verify[0].temperature);
    assertFloatsEqual(reading.altitude, verify[0].altitude);
    Serial.printf("    Verified: T=%.1f Alt=%.1f\n",
                  verify[0].temperature, verify[0].altitude);

    // Step 4: Display graph
    Serial.println("  [4/4] Rendering graph...");
    std::vector<float> vals = {reading.temperature};
    std::vector<time_t> ts = {reading.timestamp};
    testDisplay.drawGraph("WORKFLOW TEST", "C", vals, ts,
                          reading.temperature - 5.0f, reading.temperature + 5.0f);
    TEST_ASSERT_TRUE_MESSAGE(testDisplay.isReady(), "Display should be ready");

    Serial.println("  Full workflow complete");
    Serial.println("  -> PASS");
}

void test_hiking_comfort_workflow() {
    Serial.println("\n[HIKING] test_hiking_comfort_workflow");

    // Step 1: Read sensors
    Serial.println("  [1/3] Reading sensors...");
    SensorReading reading = testSensor.getReading();
    TEST_ASSERT_TRUE_MESSAGE(reading.isValid, "Reading should be valid");
    Serial.printf("    T=%.1f C\n", reading.temperature);

    // Step 2: Log comfort
    Serial.println("  [2/3] Logging comfort...");
    ComfortLog log;
    log.timestamp = testRtc.getEpochTime();
    log.level = ComfortLevel::COMFORTABLE;
    bool stored = testStorage.storeComfortLog(log);
    TEST_ASSERT_TRUE_MESSAGE(stored, "Comfort store should succeed");
    Serial.printf("    Logged: %s @ %ld\n", comfortLevelToString(log.level), (long)log.timestamp);

    // Step 3: Query back
    Serial.println("  [3/3] Querying comfort logs...");
    std::vector<ComfortLog> logs;
    testStorage.getComfortLogsSince(log.timestamp - 1, logs);
    TEST_ASSERT_TRUE_MESSAGE(logs.size() >= 1, "Should have at least 1 log");
    Serial.printf("    Found %d comfort log(s)\n", (int)logs.size());

    Serial.println("  Comfort workflow complete");
    Serial.println("  -> PASS");
}

void test_hiking_timing() {
    Serial.println("\n[HIKING] test_hiking_timing");

    // Measure sensor read time
    unsigned long t0 = millis();
    SensorReading reading = testSensor.getReading();
    unsigned long t1 = millis();

    // Measure store time
    bool stored = testStorage.storeReading(reading);
    unsigned long t2 = millis();

    // Measure read-back time
    std::vector<SensorReading> verify;
    testStorage.getLastNReadings(verify, 1);
    unsigned long t3 = millis();

    Serial.printf("  Sensor read:   %lu ms\n", t1 - t0);
    Serial.printf("  SD store:      %lu ms\n", t2 - t1);
    Serial.printf("  SD read-back:  %lu ms\n", t3 - t2);
    Serial.printf("  Total:         %lu ms\n", t3 - t0);

    TEST_ASSERT_TRUE_MESSAGE(stored, "Store should succeed");
    TEST_ASSERT_TRUE_MESSAGE((t3 - t0) < 500, "Total cycle should be under 500ms");

    Serial.println("  -> PASS");
}

// ── Test suite ────────────────────────────────────────────────────────

namespace test_hiking_station {

    void setUp() {
        // Re-initialize hardware each test
        testSensor.begin();
        testStorage.begin();
        testDisplay.begin();
        testRtc.begin();
        pinMode(NAV_BUTTON_PIN, INPUT_PULLUP);
        pinMode(SEL_BUTTON_PIN, INPUT_PULLUP);
    }

    void tearDown() {
        testDisplay.disconnect();
    }

    void run_tests() {
        UnitySetTestFile(__FILE__);
        TEST_CONTEXT.setFixtures(setUp, tearDown);

        LOG_INFO("\n========================================");
        LOG_INFO("[RUN TEST] HIKING STATION INTEGRATION");
        LOG_INFO("========================================\n");

        // ── One-time NTP sync ────────────────────────────────────────
        Serial.println("\n[HIKING] NTP time sync (one-time)...");
        testRtc.begin();

        if (testWifi.connect(WIFI_DEFAULT_SSID, WIFI_DEFAULT_PASSWORD, 10)) {
            Serial.printf("  WiFi connected: %s\n", testWifi.getIPAddress().c_str());

            if (testWifi.syncTimeNTP(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC)) {
                time_t now;
                time(&now);
                testRtc.setTime(now);
                testRtc.setLastSyncTime(now);
                Serial.printf("  RTC synced: %s\n", testRtc.getFormattedTime().c_str());
            } else {
                Serial.println("  WARNING: NTP sync failed, tests will use boot time");
            }

            testWifi.disconnect();
            Serial.println("  WiFi disconnected");
        } else {
            Serial.println("  WARNING: WiFi connect failed, tests will use boot time");
        }

        // ── Create display service (nullptr for ConnectivityService) ──
        testDisplayService = new DisplayService(&testDisplay, &testRtc, nullptr);

        // ── Run all tests ────────────────────────────────────────────
        RUN_TEST(test_hiking_sensor_with_altitude);
        RUN_TEST(test_hiking_fresh_start);
        RUN_TEST(test_hiking_csv_roundtrip);
        RUN_TEST(test_hiking_measurement_cycle);
        RUN_TEST(test_hiking_comfort_single);
        RUN_TEST(test_hiking_comfort_multiple);
        RUN_TEST(test_hiking_display_graph);
        RUN_TEST(test_hiking_display_menu);
        RUN_TEST(test_hiking_display_comfort_ui);
        RUN_TEST(test_hiking_full_workflow);
        RUN_TEST(test_hiking_comfort_workflow);
        RUN_TEST(test_hiking_timing);

        // ── Cleanup ──────────────────────────────────────────────────
        delete testDisplayService;
        testDisplayService = nullptr;

        TEST_CONTEXT.clearFixtures();
    }
}
