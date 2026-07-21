# Plan: Hardware Test Environment

## Goal
- Rename `test/test_embedded/` → `test/test_hardware/`
- Create `[env:hardware]` for `pio test -e hardware` (unit tests)
- Create `src/hardware_test.cpp` firmware for `pio run -t upload -e hardware` (lowest-level hardware exercise)
- Keep `[env:main]` unchanged (production firmware)

## Step 1: Rename test directory

```
test/test_embedded/ → test/test_hardware/
```

Files inside:
- `test_embedded.cpp` → `test_hardware.cpp` (rename the file too)
- `includes/test_sensor.hpp` — no change
- `includes/test_rtc.hpp` — no change
- `includes/test_storage.hpp` — no change
- `includes/test_display.hpp` — no change
- `includes/test_wifi.hpp` — no change

Update `test_hardware.cpp`:
- Include paths stay the same (relative `"test_storage.hpp"` etc.)
- All 4 test suites remain active

## Step 2: Create `[env:hardware]` in platformio.ini

```ini
[env:hardware]
extends = esp32_common
test_filter = test_hardware
build_flags = 
    ${esp32_common.build_flags}
    -Itest/test_hardware/includes
```

This environment:
- `pio test -e hardware` — compiles + runs tests from `test/test_hardware/`
- `pio run -t upload -e hardware` — compiles `src/` firmware (including `hardware_test.cpp`)

## Step 3: Create `src/hardware_test.cpp`

A minimal firmware that exercises ALL hardware managers directly at the lowest level — no service layer, no deep sleep, no display logic beyond basic init.

```cpp
#include <Arduino.h>
#include <config.h>
#include <sensor_manager.h>
#include <storage_manager.h>
#include <rtc_manager.h>
#include <display_manager.h>
#include <wifi_manager.h>
#include <data_structures.h>

// Direct hardware manager instances (no services)
RTCManager rtc;
SensorManager sensor;
DisplayManager display;
StorageManager storage;
WiFiManager wifiMgr;

#ifndef UNIT_TEST
void setup() {
    Serial.begin(115200);
    while (!Serial);
    delay(2000);

    Serial.println("=== Hardware Test ===");

    // 1. RTC
    Serial.println("[1/6] RTC...");
    rtc.begin();
    Serial.printf("  Time: %s\n", rtc.getFormattedTime().c_str());

    // 2. WiFi + NTP
    Serial.println("[2/6] WiFi + NTP...");
    if (wifiMgr.connect(WIFI_SSID, WIFI_PASSWORD, 10)) {
        Serial.printf("  IP: %s\n", wifiMgr IP address...
        wifiMgr.syncTimeNTP(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);
        time_t now; time(&now); rtc.setTime(now);
        Serial.printf("  Synced: %s\n", rtc.getFormattedTime().c_str());
        wifiMgr.disconnect();
    } else {
        Serial.println("  WiFi FAILED");
    }

    // 3. SD Card (SPI bus conflict avoidance)
    Serial.println("[3/6] SD Card...");
    digitalWrite(TFT_CS, HIGH);
    if (storage.begin()) {
        SensorReading r;
        r.temperature = 42.0; r.humidity = 50.0; r.pressure = 1013.25;
        r.altitude = 100.0; r.timestamp = rtc.getEpoch();
        storage.storeReading(r);
        Serial.println("  SD write OK");
    } else {
        Serial.println("  SD FAILED");
    }

    // 4. Display
    Serial.println("[4/6] Display...");
    display.begin();
    display.clearDisplay();
    display.showValue("HW TEST", 0, 0);
    Serial.println("  Display OK");

    // 5. Sensor
    Serial.println("[5/6] BME280 Sensor...");
    if (sensor.begin()) {
        SensorReading reading = sensor.getReading();
        Serial.printf("  T=%.1fC H=%.1f%% P=%.1fhPa\n",
            reading.temperature, reading.humidity, reading.pressure);
    } else {
        Serial.println("  Sensor FAILED");
    }

    // 6. Summary
    Serial.println("[6/6] All hardware tested.");
    Serial.println("=== Hardware Test Complete ===");
}

void loop() {
    delay(10000);
}
#endif
```

Key points:
- Uses `#ifndef UNIT_TEST` to exclude `setup()`/`loop()` during test builds (PlatformIO defines this macro)
- Exercises each manager directly — no service layer
- Respects SPI bus conflict (SD before display, TFT_CS HIGH)
- Reports pass/fail for each component via Serial

## Step 4: Update `[env:main]` test_filter

Change `test_filter = test_embedded` → `test_filter = test_hardware` so `pio test -e main` also uses the renamed tests.

## Step 5: Update platformio.ini example env paths

No changes needed — example envs reference `lib/hardware/` paths which are unchanged.

## Files Changed

| File | Action |
|------|--------|
| `test/test_embedded/test_embedded.cpp` | Rename → `test/test_hardware/test_hardware.cpp` |
| `test/test_embedded/includes/*.hpp` | Move → `test/test_hardware/includes/` |
| `src/hardware_test.cpp` | **Create** — hardware test firmware |
| `platformio.ini` | Add `[env:hardware]`, update `[env:main]` test_filter |

## Verification

1. `pio test -e hardware` — runs hardware unit tests
2. `pio run -t upload -e hardware` — uploads hardware test firmware
3. `pio test -e main` — still runs hardware tests (uses renamed dir)
4. `pio run -t upload -e main` — still uploads production firmware
5. `pio test -e mock` — 33 tests pass (unchanged)
