// Display Manager Example
// 100% isolated — no RTC, no WiFi, no SD card. Exercises all DisplayManager methods.

#include <Arduino.h>
#include <display_manager.h>
#include <data_structures.h>
#include <config.h>

DisplayManager display;

// ── Dummy Data ──────────────────────────────────────────────────────────────

SensorReading makeReading(float temp, float hum, float press, float alt) {
    SensorReading r;
    r.temperature = temp;
    r.humidity = hum;
    r.pressure = press;
    r.altitude = alt;
    r.timestamp = 1700000000;
    r.isValid = true;
    return r;
}

TemperatureStats makeStats(float minT, float maxT, float avg, int count) {
    TemperatureStats s;
    s.min = minT;
    s.max = maxT;
    s.average = avg;
    s.sampleCount = count;
    s.isValid = true;
    return s;
}

SystemStatus makeStatus() {
    SystemStatus s;
    s.sensorOk = true;
    s.displayOk = true;
    s.storageOk = false;
    s.rtcOk = true;
    s.wifiOk = false;
    s.freeMemory = 180000;
    s.uptime = 3661;
    return s;
}

// ── Demo Sequence ───────────────────────────────────────────────────────────

void demoReading() {
    Serial.println("=== showReading ===");
    SensorReading r = makeReading(23.5, 58.2, 1013.25, 420);
    display.showReading(r, "2025-07-20 14:30");
    delay(3000);
}

void demoReadingNoTime() {
    Serial.println("=== showReading (no time) ===");
    SensorReading r = makeReading(18.1, 72.0, 998.5, 850);
    display.showReading(r);
    delay(3000);
}

void demoTemperatureStats() {
    Serial.println("=== showTemperatureStats ===");
    TemperatureStats s = makeStats(12.3, 28.7, 20.1, 48);
    display.showTemperatureStats(s);
    delay(3000);
}

void demoSystemStatus() {
    Serial.println("=== showSystemStatus ===");
    SystemStatus s = makeStatus();
    display.showSystemStatus(s);
    delay(3000);
}

void demoConnectivityStatus() {
    Serial.println("=== showConnectivityStatus ===");
    ConnectivityStatus states[] = {
        ConnectivityStatus::DISCONNECTED,
        ConnectivityStatus::CONNECTING,
        ConnectivityStatus::CONNECTED,
        ConnectivityStatus::TIME_SYNCING,
        ConnectivityStatus::SYNC_COMPLETE
    };
    for (auto state : states) {
        display.showConnectivityStatus(state);
        delay(1500);
    }
}

void demoMessage() {
    Serial.println("=== showMessage ===");
    display.showMessage("Hello, World!");
    delay(2000);
}

void demoError() {
    Serial.println("=== showError ===");
    display.showError("SD card failed!");
    delay(2000);
}

void demoGraph() {
    Serial.println("=== drawGraph ===");
    std::vector<float> temps = {18.2, 19.1, 20.5, 22.3, 23.1, 22.8, 21.0, 19.5};
    std::vector<time_t> times;
    time_t base = 1700000000;
    for (int i = 0; i < (int)temps.size(); i++)
        times.push_back(base + i * 3600);

    display.drawGraph("TEMPERATURE", "C", temps, times, 15.0, 25.0);
    delay(3000);
}

void demoBrightness() {
    Serial.println("=== setBrightness ===");
    display.showMessage("Bright");
    delay(1000);
    for (int b = 255; b >= 10; b -= 15) {
        display.setBrightness(b);
        delay(100);
    }
    display.setBrightness(255);
    delay(500);
}

void demoHeaderAndSeparator() {
    Serial.println("=== drawHeader + drawSeparator ===");
    display.clear();
    display.drawHeader("TEST PAGE");
    display.drawSeparator(30);
    display.getTFT()->setTextColor(ST77XX_WHITE);
    display.getTFT()->setTextSize(1);
    display.getTFT()->setCursor(5, 40);
    display.getTFT()->println("drawHeader + drawSeparator");
    display.getTFT()->println("work correctly.");
    delay(3000);
}

// ── Setup / Loop ────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("\n=== Display Manager Example ===\n");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    display.begin();
    display.setBrightness(255);
}

void loop() {
    demoReading();
    demoReadingNoTime();
    demoTemperatureStats();
    demoSystemStatus();
    demoConnectivityStatus();
    demoMessage();
    demoError();
    demoGraph();
    demoBrightness();
    demoHeaderAndSeparator();

    Serial.println("\n--- Demo complete, restarting ---\n");
    delay(1000);
}
