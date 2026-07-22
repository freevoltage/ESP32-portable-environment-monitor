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

    // Connect WiFi (OTA always requires WiFi)
    display.showMessage("WiFi required\nfor OTA...");
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

    // Stay in OTA loop — exit on button B or 120s timeout
    unsigned long otaStart = millis();
    Serial.println("[OTA] Server started. Waiting for upload... (B=Exit, 120s timeout)");

    while (true) {
        ElegantOTA.loop();

        // Check for abort: Button B alone or both buttons together
        bool navLow = (digitalRead(NAV_BUTTON_PIN) == LOW);
        bool selLow = (digitalRead(SEL_BUTTON_PIN) == LOW);
        if (selLow || (navLow && selLow)) {
            Serial.println("[OTA] Aborted by user");
            display.showMessage("OTA Aborted.\nGoing to sleep...");
            delay(1500);
            return;
        }

        // 120 second timeout
        if (millis() - otaStart > 120000) {
            Serial.println("[OTA] Timed out");
            display.showMessage("OTA Timeout.\nGoing to sleep...");
            delay(1500);
            return;
        }

        delay(10);
    }
}

// ── Measurement Mode (timer wake, display stays OFF) ──────────────────────

void runMeasurementMode() {
    Serial.println("[MEASUREMENT] Timer wake — reading sensors, display OFF");

    // Initialize SD (display CS high to avoid SPI conflict)
    digitalWrite(TFT_CS, HIGH);
    storage.begin();

    storage.logDebug("BOOT", "Measurement mode (timer wake)");

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
        char buf[80];
        snprintf(buf, sizeof(buf), "Reading: %.1fC %.0f%% %.0fhPa %.0fm",
                 reading.temperature, reading.humidity,
                 reading.pressure, reading.altitude);
        storage.logDebug("SENSOR", buf);
    } else {
        Serial.println("[MEASUREMENT] Sensor read failed");
        storage.logDebug("SENSOR", "Read FAILED");
    }

    // Log battery status
    BatteryStatus battStatus = battery.getStatus();
    if (battStatus.isValid) {
        Serial.printf("[MEASUREMENT] Battery: %.0f%% (%.2fV)\n",
                      battStatus.percent, battStatus.voltage);
        char buf[40];
        snprintf(buf, sizeof(buf), "Battery: %.0f%% (%.2fV)", battStatus.percent, battStatus.voltage);
        storage.logDebug("BATT", buf);
    }

    // Sleep immediately — no display, no WiFi
}

// ── Display Mode (button wake, full UI) ───────────────────────────────────

bool enterMenu(bool& aborted);  // Forward declaration

// Wait for any button press and return which one: 1=NAV(A), 2=SEL(B), 3=BOTH (abort)
int waitForButton() {
    int pressed = 0;
    while (pressed == 0) {
        bool navLow = (digitalRead(NAV_BUTTON_PIN) == LOW);
        bool selLow = (digitalRead(SEL_BUTTON_PIN) == LOW);
        if (navLow && selLow) pressed = 3;      // Both = abort
        else if (navLow) pressed = 1;
        else if (selLow) pressed = 2;
        delay(10);
    }
    delay(100); // Debounce after capture
    return pressed;
}

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
    waitForButton();
}

void runDisplayMode() {
    Serial.println("[DISPLAY] Button wake — full UI");

    // Initialize everything
    digitalWrite(TFT_CS, HIGH);
    storage.begin();

    storage.logDebug("BOOT", "Display mode (button wake)");

    display.begin();
    sensor.begin();
    battery.begin();

    rtc.begin();

    // Initialize time sync service (for manual sync from menu)
    timeSync.begin(&rtc, &connectivity);

    // Time is already set from the last measurement cycle (within 30 min).
    // No auto-sync here — the device should respond instantly on button wake.
    // Users can sync manually from the SYNC_TIME menu if needed.

    displayService.showStartupScreen();
    delay(1500);

    // Take a reading
    SensorReading reading;
    BatteryStatus battStatus;
    if (dataService.collectCurrentReading()) {
        reading = dataService.getCurrentReading();
        dataService.storeCurrentReading();

        char buf[80];
        snprintf(buf, sizeof(buf), "Reading: %.1fC %.0f%% %.0fhPa",
                 reading.temperature, reading.humidity, reading.pressure);
        storage.logDebug("SENSOR", buf);

        battStatus = battery.getStatus();
        if (battStatus.isValid) {
            snprintf(buf, sizeof(buf), "Battery: %.0f%% (%.2fV)", battStatus.percent, battStatus.voltage);
            storage.logDebug("BATT", buf);
        }
    }

    // Configure buttons as inputs
    pinMode(NAV_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SEL_BUTTON_PIN, INPUT_PULLUP);

    // ── Dashboard loop ──────────────────────────────────────────────
    // Dashboard shows current readings + three quick actions:
    //   0 = Log Comfort, 1 = Menu (full menu), 2 = Sleep
    int dashItem = 0;
    bool inDisplayMode = true;

    while (inDisplayMode) {
        displayService.showDashboard(reading, rtc.getFormattedTime(), dashItem, battStatus);

        int btn = waitForButton();

        if (btn == 1) {
            dashItem = (dashItem + 1) % 3;
        }

        if (btn == 2) {
            if (dashItem == 0) {
                // ── Log Comfort ──────────────────────────────────────
                // Check if already logged today
                time_t now = rtc.getEpochTime();
                struct tm* ti = localtime(&now);
                time_t startOfDay = now - (ti->tm_hour * 3600 + ti->tm_min * 60 + ti->tm_sec);

                std::vector<ComfortLog> todayLogs;
                storage.getComfortLogsSince(startOfDay, todayLogs);

                if (!todayLogs.empty()) {
                    display.clear();
                    display.showMessage("Already logged\ntoday!");
                    delay(1500);
                } else {
                    ComfortLevel comfortLevel = ComfortLevel::COMFORTABLE;
                    bool selecting = true;

                    while (selecting) {
                        displayService.showComfortUI(comfortLevel);

                        int cbtn = waitForButton();

                        if (cbtn == 3) { selecting = false; }           // Abort → dashboard
                        if (cbtn == 1) {                                 // Cycle level
                            int cl = static_cast<int>(comfortLevel);
                            cl = (cl + 1) % 5;
                            comfortLevel = static_cast<ComfortLevel>(cl);
                        }
                        if (cbtn == 2) {                                 // Log it
                            ComfortLog log;
                            log.timestamp = rtc.getEpochTime();
                            log.level = comfortLevel;
                            storage.storeComfortLog(log);

                            display.clear();
                            display.showMessage("LOGGED!");
                            delay(1500);
                            selecting = false;
                        }
                    }
                }
            } else if (dashItem == 1) {
                // ── Full Menu ────────────────────────────────────────
                bool aborted = false;
                inDisplayMode = enterMenu(aborted);
                // aborted → back to dashboard (inDisplayMode stays true)
            } else {
                // ── Sleep ────────────────────────────────────────────
                inDisplayMode = false;
            }
        }
        // btn == 3 (both) at dashboard level → do nothing, stay here
    }
}

// ── Full Menu (reached from dashboard) ───────────────────────────────────

bool enterMenu(bool& aborted) {
    DisplayMenu currentMenu = DisplayMenu::GRAPH_TEMP;
    bool inMenu = true;
    aborted = false;

    while (inMenu) {
        displayService.showMenu(currentMenu);

        int btn = waitForButton();

        if (btn == 3) { aborted = true; break; }  // Both → back to dashboard

        if (btn == 1) {
            int idx = static_cast<int>(currentMenu);
            idx = (idx + 1) % 6;
            currentMenu = static_cast<DisplayMenu>(idx);
        }

        if (btn == 2) {
            switch (currentMenu) {
                case DisplayMenu::GRAPH_TEMP:
                case DisplayMenu::GRAPH_HUMIDITY:
                case DisplayMenu::GRAPH_ALTITUDE:
                    showGraphForMenu(currentMenu);
                    break;

                case DisplayMenu::SLEEP:
                    inMenu = false;
                    break;

                case DisplayMenu::OTA:
                    runOTAMode(); // Never returns
                    break;

                case DisplayMenu::SYNC_TIME: {
                    // Sync time sub-menu: 3 items, A=Navigate, B=Select
                    enum SyncMenuItem { SYNC_MODE, SYNC_NOW, SYNC_BACK };
                    int syncItem = SYNC_MODE;
                    bool inSyncMenu = true;

                    while (inSyncMenu) {
                        SyncStatus syncStatus = timeSync.getStatus();
                        displayService.showSyncSubMenu(syncItem, syncStatus.mode, syncStatus.lastSource, syncStatus.lastSyncTime);

                        int sbtn = waitForButton();

                        if (sbtn == 3) { inSyncMenu = false; break; }   // Abort → menu

                        if (sbtn == 1) {
                            syncItem = (syncItem + 1) % 3;
                        }

                        if (sbtn == 2) {
                            switch (syncItem) {
                                case SYNC_MODE: {
                                    SyncMode current = timeSync.getMode();
                                    int modeIdx = (static_cast<int>(current) + 1) % 5;
                                    timeSync.setMode(static_cast<SyncMode>(modeIdx));
                                    break;
                                }
                                case SYNC_NOW: {
                                    display.showSyncProgress("Syncing...");
                                    timeSync.sync([](const char* msg) {
                                        display.showSyncProgress(msg);
                                    });
                                    storage.logDebug("SYNC", "Manual sync triggered from menu");
                                    delay(1000);
                                    break;
                                }
                                case SYNC_BACK:
                                    inSyncMenu = false;
                                    break;
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    if (aborted) return true;   // Back to dashboard
    Serial.println("[DISPLAY] User selected sleep");
    return false;               // Deep sleep
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
