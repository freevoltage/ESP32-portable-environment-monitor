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

void printWakeupReason() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:  Serial.println("Wakeup: Timer"); break;
        case ESP_SLEEP_WAKEUP_EXT1:   Serial.println("Wakeup: Button"); break;
        default: Serial.printf("Wakeup: Reset (%d)\n", cause); break;
    }
}

void enterDeepSleep() {
    Serial.println("Entering deep sleep...");
    displayService.turnOff();

    // Configure EXT1 wake on both buttons (active LOW)
    esp_sleep_enable_ext1_wakeup(
        (1ULL << NAV_BUTTON_PIN) | (1ULL << SEL_BUTTON_PIN),
        ESP_EXT1_WAKEUP_ANY_LOW
    );

    // Also configure timer wake for periodic measurements
    esp_sleep_enable_timer_wakeup(MEASUREMENT_INTERVAL_SEC * uS_TO_S_FACTOR);

    // Hold backlight pin state during deep sleep (prevents GPIO2 floating -> backlight leakage)
    // Toggle via HOLD_GPIO_IN_SLEEP in config.h; hardware alternative: pull-down resistor on TFT_LIT
#ifdef HOLD_GPIO_IN_SLEEP
    gpio_hold_en(static_cast<gpio_num_t>(TFT_LIT));
#endif

    Serial.printf("Deep sleep. Timer=%ds, EXT1 on GPIO%d+GPIO%d\n",
                  MEASUREMENT_INTERVAL_SEC, NAV_BUTTON_PIN, SEL_BUTTON_PIN);
    Serial.flush();
    esp_deep_sleep_start();
}

// ── Measurement Mode (timer wake, display stays OFF) ──────────────────────

void runMeasurementMode() {
    Serial.println("[MEASUREMENT] Timer wake — reading sensors, display OFF");

    // Initialize SD (display CS high to avoid SPI conflict)
    digitalWrite(TFT_CS, HIGH);
    storage.begin();

    // Initialize sensor
    sensor.begin();

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

    rtc.begin();
    connectivity.ensureTimeSync();

    displayService.showStartupScreen();
    delay(1500);

    // Take a reading and display it
    if (dataService.collectCurrentReading()) {
        SensorReading reading = dataService.getCurrentReading();
        dataService.storeCurrentReading();
        displayService.showCurrentReading(reading, rtc.getFormattedTime());
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
            idx = (idx + 1) % 5; // 5 menu items
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
            }
        }
    }

    Serial.println("[DISPLAY] User selected sleep");
}

// ── Setup / Loop ──────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    while (!Serial);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);

    ++bootCount;
    Serial.printf("\n=== Boot #%d ===\n", bootCount);
    printWakeupReason();

    // Configure button pins for EXT1 wake
    pinMode(NAV_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SEL_BUTTON_PIN, INPUT_PULLUP);

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

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
