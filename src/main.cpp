#include <Arduino.h>
#include <config.h>
#include <rtc_manager.h>
#include <sensor_manager.h>
#include <display_manager.h>
#include <storage_manager.h>
#include <wifi_manager.h>
#include <connectivity_service.h>
#include <display_service.h>
#include <data_service.h>
#include <data_structures.h>
#include <battery_manager.h>
#include <time_sync_service.h>

// ESP-IDF sleep API
#include "driver/gpio.h"
#include "esp_sleep.h"

// OTA
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

// RTC memory persists across deep sleep
RTC_DATA_ATTR int bootCount = 0;

// Manager objects
RTCManager rtc;
SensorManager sensor;
DisplayManager display;
StorageManager storage;
WiFiManager wifiMgr;
ConnectivityService connectivity(&wifiMgr, &rtc);
DisplayService displayService(&display, &rtc, &connectivity);
DataService dataService(&sensor, &storage, &rtc);
BatteryManager battery;
TimeSyncService timeSync;

// OTA server
AsyncWebServer otaServer(OTA_SERVER_PORT);

void printWakeupReason() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:  Serial.println("Wakeup: Timer"); break;
        case ESP_SLEEP_WAKEUP_EXT1:   Serial.println("Wakeup: Button"); break;
        default: Serial.printf("Wakeup: Reset (%d)\n", cause); break;
    }
}

// Detect EXT1 button wake using multiple fallback methods.
// On ESP32-C6 (Tasmota), esp_sleep_get_wakeup_cause() may return UNDEFINED
// even for valid EXT1 or timer wakes.
esp_sleep_wakeup_cause_t detectWakeupCause() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_UNDEFINED) return cause;

    // Fallback 1: check EXT1 status register directly
    uint64_t ext1Status = esp_sleep_get_ext1_wakeup_status();
    if (ext1Status != 0) {
        Serial.printf("[WAKE] EXT1 status register: 0x%llX\n", ext1Status);
        return ESP_SLEEP_WAKEUP_EXT1;
    }

    // Fallback 2: read raw GPIO before pinMode reconfigures the pin.
    // After EXT1 wake, the pad state still reflects the trigger level briefly.
    if (bootCount > 1) {
        bool selBtnLow = (gpio_get_level(static_cast<gpio_num_t>(SEL_BUTTON_PIN)) == 0);
        Serial.printf("[WAKE] Raw GPIO%d = %s\n", SEL_BUTTON_PIN, selBtnLow ? "LOW" : "HIGH");
        if (selBtnLow) return ESP_SLEEP_WAKEUP_EXT1;
    }

    return ESP_SLEEP_WAKEUP_UNDEFINED;
}

void enterDeepSleep() {
    Serial.println("Entering deep sleep...");
    displayService.turnOff();

    // Cut I2C power rail to save ~55uA during sleep
    battery.disableI2CPower();

    // Configure EXT1 wake on Select button only (GPIO3 = valid RTC GPIO on ESP32-C6)
    // Note: GPIO8/GPIO9 are NOT RTC GPIOs — only GPIO0-7 support EXT1 wakeup
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(
        (1ULL << SEL_BUTTON_PIN),
        ESP_EXT1_WAKEUP_ANY_LOW
    ));

    // Also configure timer wake for periodic measurements
    esp_sleep_enable_timer_wakeup(MEASUREMENT_INTERVAL_SEC * uS_TO_S_FACTOR);

    // Hold backlight pin state during deep sleep (prevents GPIO2 floating -> backlight leakage)
    // Toggle via HOLD_GPIO_IN_SLEEP in config.h; hardware alternative: pull-down resistor on TFT_LIT
#ifdef HOLD_GPIO_IN_SLEEP
    gpio_hold_en(static_cast<gpio_num_t>(TFT_LIT));
#endif

    Serial.printf("Deep sleep. Timer=%ds, EXT1 on GPIO%d\n",
                  MEASUREMENT_INTERVAL_SEC, SEL_BUTTON_PIN);
    Serial.flush();
    esp_deep_sleep_start();
}

// ── OTA Mode (menu-triggered, stays awake for firmware upload) ────────────

void runOTAMode() {
    Serial.println("[OTA] Entering OTA mode");

    // Initialize display
    digitalWrite(TFT_CS, HIGH);
    display.begin();
    display.setBrightness(255);

    // Connect WiFi
    display.showMessage("Connecting WiFi...");
    const WiFiConfig& wifiCfg = connectivity.getWiFiConfig();
    wifiMgr.connect(wifiCfg.ssid, wifiCfg.password, 30);

    if (!wifiMgr.isConnected()) {
        display.showMessage("WiFi FAILED!\nRebooting...");
        delay(2000);
        ESP.restart();
    }

    String ip = wifiMgr.getIPAddress();
    Serial.printf("[OTA] Connected! IP: %s\n", ip.c_str());

    // Show OTA screen
    display.showOTAMode(ip.c_str());

    // Configure ElegantOTA
    ElegantOTA.begin(&otaServer, OTA_USERNAME, OTA_PASSWORD);

    ElegantOTA.onStart([]() {
        display.showMessage("OTA UPDATE\nStarting...");
    });

    ElegantOTA.onProgress([](size_t current, size_t total) {
        static unsigned long lastDraw = 0;
        if (millis() - lastDraw > 500) {
            lastDraw = millis();
            int percent = (total > 0) ? (current * 100 / total) : 0;
            display.showOTAProgress(percent, current, total);
            Serial.printf("[OTA] Progress: %d%% (%u / %u)\n", percent, (unsigned int)current, (unsigned int)total);
        }
    });

    ElegantOTA.onEnd([](bool success) {
        if (success) {
            display.showMessage("OTA Complete!\nRebooting...");
        } else {
            display.showMessage("OTA FAILED!\nRebooting...");
        }
        delay(3000);
        ESP.restart();
    });

    otaServer.begin();
    Serial.println("[OTA] Server started. Waiting for upload...");

    // Stay in OTA loop indefinitely (no deep sleep)
    while (true) {
        ElegantOTA.loop();
        delay(10);
    }
}

// ── Measurement Mode (timer wake, display stays OFF) ──────────────────────

void runMeasurementMode() {
    Serial.println("[MEASUREMENT] Timer wake — reading sensors, display OFF");

    // Initialize SD (display CS high to avoid SPI conflict)
    digitalWrite(TFT_CS, HIGH);
    storage.begin();

    // Initialize sensor and battery
    sensor.begin();
    battery.begin();

    // Collect and store reading
    if (dataService.collectCurrentReading()) {
        SensorReading reading = dataService.getCurrentReading();
        dataService.storeCurrentReading();
        Serial.printf("[MEASUREMENT] Stored: %.1f°C %.0f%% %.0fhPa %.0fm\n",
                      reading.temperature, reading.humidity,
                      reading.pressure, reading.altitude);
    } else {
        Serial.println("[MEASUREMENT] Sensor read failed");
    }

    // Log battery status
    BatteryStatus battStatus = battery.getStatus();
    if (battStatus.isValid) {
        Serial.printf("[MEASUREMENT] Battery: %.0f%% (%.2fV)\n",
                      battStatus.percent, battStatus.voltage);
    }

    // Sleep immediately — no display, no WiFi
}

// ── Display Mode (button wake, full UI) ───────────────────────────────────

void showGraphForMenu(DisplayMenu menu) {
    std::vector<SensorReading> readings;
    time_t since = rtc.getEpochTime() - 86400; // Last 24h
    storage.getReadingsSince(since, readings, 240);

    std::vector<float> values;
    std::vector<time_t> timestamps;
    float minVal = 9999, maxVal = -9999;

    for (const auto& r : readings) {
        float val = 0;
        const char* title = "";
        const char* unit = "";

        switch (menu) {
            case DisplayMenu::GRAPH_TEMP:
                val = r.temperature; unit = "C"; break;
            case DisplayMenu::GRAPH_HUMIDITY:
                val = r.humidity; unit = "%"; break;
            case DisplayMenu::GRAPH_ALTITUDE:
                val = r.altitude; unit = "m"; break;
            default: break;
        }

        values.push_back(val);
        timestamps.push_back(r.timestamp);
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }

    if (values.empty()) {
        displayService.showErrorScreen("No data yet");
        delay(2000);
        return;
    }

    const char* titles[] = {"TEMPERATURE", "HUMIDITY", "ALTITUDE"};
    const char* units[] = {"C", "%", "m"};
    int idx = static_cast<int>(menu); // GRAPH_TEMP=0, GRAPH_HUMIDITY=1, GRAPH_ALTITUDE=2

    // Add padding to min/max
    float range = maxVal - minVal;
    if (range < 1.0f) range = 1.0f;
    minVal -= range * 0.1f;
    maxVal += range * 0.1f;

    displayService.showGraph(titles[idx], units[idx], values, timestamps, minVal, maxVal);

    // Wait for button press to go back to menu
    Serial.println("Graph shown. Press any button to return to menu.");
    while (digitalRead(NAV_BUTTON_PIN) == HIGH && digitalRead(SEL_BUTTON_PIN) == HIGH) {
        delay(50);
    }
    delay(200); // Debounce
}

void runDisplayMode() {
    Serial.println("[DISPLAY] Button wake — full UI");

    // Initialize everything
    digitalWrite(TFT_CS, HIGH);
    storage.begin();

    display.begin();
    sensor.begin();
    battery.begin();

    rtc.begin();

    // Initialize time sync service
    timeSync.begin(&rtc, &connectivity);

    // Sync time (BLE first, then WiFi fallback per configured mode)
    display.showSyncProgress("Syncing time...");
    timeSync.sync([](const char* msg) {
        display.showSyncProgress(msg);
    });

    displayService.showStartupScreen();
    delay(1500);

    // Take a reading and display it
    if (dataService.collectCurrentReading()) {
        SensorReading reading = dataService.getCurrentReading();
        dataService.storeCurrentReading();
        displayService.showCurrentReading(reading, rtc.getFormattedTime());

        // Show battery info at bottom of screen
        BatteryStatus battStatus = battery.getStatus();
        display.showBatteryInfo(battStatus);

        delay(3000);
    }

    // Configure buttons as inputs
    pinMode(NAV_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SEL_BUTTON_PIN, INPUT_PULLUP);

    // ── Menu loop ──────────────────────────────────────────────────────
    DisplayMenu currentMenu = DisplayMenu::GRAPH_TEMP;
    bool inDisplayMode = true;

    while (inDisplayMode) {
        displayService.showMenu(currentMenu);

        // Wait for button press
        bool navPressed = false;
        bool selPressed = false;

        while (!navPressed && !selPressed) {
            if (digitalRead(NAV_BUTTON_PIN) == LOW) navPressed = true;
            if (digitalRead(SEL_BUTTON_PIN) == LOW) selPressed = true;
            delay(50);
        }

        delay(200); // Debounce

        if (navPressed) {
            // Cycle through menu items
            int idx = static_cast<int>(currentMenu);
            idx = (idx + 1) % 7; // 7 menu items
            currentMenu = static_cast<DisplayMenu>(idx);
        }

        if (selPressed) {
            switch (currentMenu) {
                case DisplayMenu::GRAPH_TEMP:
                case DisplayMenu::GRAPH_HUMIDITY:
                case DisplayMenu::GRAPH_ALTITUDE:
                    showGraphForMenu(currentMenu);
                    break;

                case DisplayMenu::LOG_COMFORT: {
                    ComfortLevel comfortLevel = ComfortLevel::COMFORTABLE;
                    bool selecting = true;

                    while (selecting) {
                        displayService.showComfortUI(comfortLevel);

                        // Wait for button
                        while (digitalRead(NAV_BUTTON_PIN) == HIGH &&
                               digitalRead(SEL_BUTTON_PIN) == HIGH) {
                            delay(50);
                        }
                        delay(200); // Debounce

                        if (digitalRead(NAV_BUTTON_PIN) == LOW) {
                            int cl = static_cast<int>(comfortLevel);
                            cl = (cl + 1) % 5;
                            comfortLevel = static_cast<ComfortLevel>(cl);
                        }

                        if (digitalRead(SEL_BUTTON_PIN) == LOW) {
                            // Log the comfort level
                            ComfortLog log;
                            log.timestamp = rtc.getEpochTime();
                            log.level = comfortLevel;
                            storage.storeComfortLog(log);

                            // Show confirmation
                            display.clear();
                            display.showMessage("LOGGED!");
                            delay(1500);
                            selecting = false;
                        }
                    }
                    break;
                }

                case DisplayMenu::SLEEP:
                    inDisplayMode = false;
                    break;

                case DisplayMenu::OTA:
                    runOTAMode(); // Never returns
                    break;

                case DisplayMenu::SYNC_TIME: {
                    // Sync time sub-menu
                    bool inSyncMenu = true;

                    while (inSyncMenu) {
                        SyncStatus syncStatus = timeSync.getStatus();
                        displayService.showSyncUI(syncStatus.mode, syncStatus.lastSource, syncStatus.lastSyncTime);

                        // Wait for button
                        while (digitalRead(NAV_BUTTON_PIN) == HIGH &&
                               digitalRead(SEL_BUTTON_PIN) == HIGH) {
                            delay(50);
                        }
                        delay(200); // Debounce

                        if (digitalRead(NAV_BUTTON_PIN) == LOW) {
                            // Cycle through sync modes
                            SyncMode current = timeSync.getMode();
                            int modeIdx = static_cast<int>(current);
                            modeIdx = (modeIdx + 1) % 5; // 5 modes
                            timeSync.setMode(static_cast<SyncMode>(modeIdx));
                        }

                        if (digitalRead(SEL_BUTTON_PIN) == LOW) {
                            // Trigger sync
                            display.showSyncProgress("Syncing...");
                            timeSync.sync();
                            delay(1000);
                        }
                    }
                    break;
                }
            }
        }
    }

    Serial.println("[DISPLAY] User selected sleep");
}

// ── Setup / Loop ──────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Release GPIO hold from deep sleep (ESP32-C6 doesn't auto-release)
    gpio_hold_dis(static_cast<gpio_num_t>(TFT_LIT));

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);

    ++bootCount;
    Serial.printf("\n=== Boot #%d ===\n", bootCount);
    printWakeupReason();

    esp_sleep_wakeup_cause_t cause = detectWakeupCause();

    if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        // Button wake → full display mode
        runDisplayMode();
    } else {
        // Timer wake or power-on → silent measurement
        rtc.begin();
        runMeasurementMode();
    }

    enterDeepSleep();
}

void loop() {
    // Never reached — deep sleep restarts the chip
}
