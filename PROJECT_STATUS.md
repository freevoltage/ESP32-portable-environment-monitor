# Project Status — ESP32 Hiking Weather Station

Last updated: 2026-07-22

## Overview

PlatformIO embedded C++ project for **Adafruit Feather ESP32-C6**. Portable hiking weather station that reads BME280 sensor, stores data to SD card, syncs time via WiFi/NTP or BLE phone sync, and displays 24h rolling graphs on a 1.54" ST7789 TFT. Two-mode operation: silent measurement mode (timer wake) and interactive display mode (button wake). Uses Unity test framework.

## Architecture

```
Application (main.cpp)
    ├── DataService        → orchestrates sensor/storage/rtc
    ├── DisplayService     → manages display output (menu, graphs, comfort UI)
    ├── ConnectivityService → WiFi + NTP time sync
    └── TimeSyncService    → BLE phone sync + WiFi fallback (NimBLE)
            │
        Service Layer (lib/services/)
            │
        HAL Layer (lib/hardware/)
```

`main.cpp` never calls hardware managers directly — all interactions go through service layers.

**Important**: There are duplicate libraries at two levels — `lib/data_service/` and `lib/services/data/`, `lib/connectivity_service/` and `lib/services/connectivity/`, `lib/display_service/` and `lib/services/display/`. The top-level versions are the ones used by the build. The `lib/services/` copies are older/alternate implementations.

## Hardware

| Component | Model | Notes |
|-----------|-------|-------|
| MCU | Adafruit Feather ESP32-C6 | RISC-V 32-bit |
| Sensor | BME280 (I2C) | Temperature, humidity, pressure, altitude |
| Display | 1.54" 240x240 IPS ST7789 | No SD slot (old display's SD wired separately) |
| Storage | SD card (SPI) | CS=GPIO0, separate from display |
| Buttons | GPIO8 (Navigate), GPIO3 (Select) | Active LOW, internal pullup |
| Backlight | Active-HIGH | `TFT_BACKLIGHT_INVERTED = 1`, pull-down resistor to GND |
| Display rotation | `TFT_ROTATION = 2` | |
| Battery | MAX17048 (I2C) | Fuel gauge, I2C power on GPIO20 |
| RTC | ESP32Time (internal RTC) | Synced via BLE or WiFi/NTP |

## Firmware Modes

### Measurement Mode (timer wake)
- Display OFF, WiFi OFF
- Read sensors → store to SD → sleep immediately
- ~2-3 seconds total, wakes every 30 minutes (`MEASUREMENT_INTERVAL_SEC = 1800`)

### Display Mode (button wake via EXT1)
- Turn on display → auto time sync (per configured mode) → show current reading
- Navigate menu: Graph Temp / Graph Humidity / Graph Altitude / Log Comfort / OTA / Sync Time / Sleep
- 24h rolling graph using `getReadingsSince()`
- Comfort logging: 5 levels (Too cold → Too warm) → `/comfort.csv`
- OTA mode: ElegantOTA web server on port 8080, TFT progress bar, auth required
- Sync Time: cycle modes (OFF/BLE/WiFi/BLE+WiFi/WiFi+BLE), trigger manual sync
- EXT1 wake on Select button only: `(1ULL << GPIO3)`, `ESP_EXT1_WAKEUP_ANY_LOW`
- GPIO8/GPIO9 are NOT RTC GPIOs — only GPIO0-7 support EXT1 wakeup on ESP32-C6

## Shared Data Structures (`include/data_structures.h`)

- `SensorReading` — temp, humidity, pressure, altitude, timestamp, isValid
- `TemperatureStats` — min, max, average, sampleCount, isValid
- `SystemStatus` — sensor/display/storage/rtc/wifi ok, freeMemory, uptime
- `BatteryStatus` — percentage, voltage, charging, isLow
- `ComfortLevel` — enum class : uint8_t (TOO_COLD=0..TOO_WARM=4)
- `ComfortLog` — timestamp + ComfortLevel
- `DisplayMenu` — enum class : uint8_t (GRAPH_TEMP..SLEEP) [7 items]
- `SyncMode` — enum class : uint8_t (OFF=0, BLE=1, WIFI=2, BLE_FIRST=3, WIFI_FIRST=4)
- `SyncSource` — enum class : uint8_t (NONE=0, BLE=1, WIFI=2)
- `SyncStatus` — source, timestamp, inProgress
- `SystemMode`, `ConnectivityStatus` — state machine enums

## Fully Working (tested on hardware + native)

### Hardware Managers (`lib/hardware/`)

| Module            | Location                        | Status                      | Tests              |
| ----------------- | ------------------------------- | --------------------------- | ------------------ |
| `sensor_manager`  | `lib/hardware/sensor_manager/`  | Complete (BME280 + altitude)| 6 embedded         |
| `storage_manager` | `lib/hardware/storage_manager/` | Complete (~1030 lines)      | 5 active embedded  |
| `rtc_manager`     | `lib/hardware/rtc_manager/`     | Complete (ESP32Time)        | 11 embedded        |
| `display_manager` | `lib/hardware/display_manager/` | Complete (240x240 ST7789)   | 1 embedded         |
| `wifi_manager`    | `lib/hardware/wifi_manager/`    | Complete (WiFi + NTP)       | 11 native mock     |
| `battery_manager` | `lib/hardware/battery_manager/` | Complete (MAX17048)         | 9 native mock      |

### Service Layer (`lib/services/`)

| Module                 | Location                            | Status                    | Tests                     |
| ---------------------- | ----------------------------------- | ------------------------- | ------------------------- |
| `data_service`         | `lib/services/data/`                | Complete (232 lines)      | 21 native mock + 21 on-device |
| `display_service`      | `lib/services/display/`             | Complete (~140 lines)     | Tested on hardware (7-item menu, graph, comfort UI) |
| `connectivity_service` | `lib/services/connectivity/`        | Complete (116 lines)      | 12 native mock            |
| `time_sync_service`    | `lib/services/time_sync_service/`   | Complete (BLE+WiFi)       | 15 native mock        |

### Infrastructure

| Module         | Status                                | Tests    |
| -------------- | ------------------------------------- | -------- |
| `test_runner`  | Complete (custom Unity runner)        | —        |
| `test_fixture` | Complete (setUp/tearDown dispatch)    | —        |
| `utils`        | Complete (DateTimeUtils + test helpers)| —       |

## Test Results

| Env       | Platform     | What it tests                                         | Status                            |
| --------- | ------------ | ----------------------------------------------------- | --------------------------------- |
| `main`    | ESP32-C6     | All hardware + hiking station integration             | **34 tests pass on device**       |
| `mock`    | Native (Mac) | DataService + ConnectivityService + storage + WiFi + battery + time sync mocks | **68 tests pass**, 0 hardware     |
| `service` | ESP32-C6     | DataService with real hardware                        | 21 tests pass on device           |
| `native`  | Native (Mac) | Placeholder (2+2=4)                                  | Works, no real tests              |

### Hiking Station Integration Tests (`test/test_hardware/`)

12 verbose integration tests covering the full hiking station workflow:

1. `test_hiking_sensor_with_altitude` — BME280 reading includes altitude
2. `test_hiking_fresh_start` — SD card starts clean with correct header
3. `test_hiking_csv_roundtrip` — Write + read sensor data preserves values
4. `test_hiking_measurement_cycle` — Full sensor → store → read cycle <2s
5. `test_hiking_comfort_single` — Comfort log store + retrieve preserves level
6. `test_hiking_comfort_multiple` — Multiple comfort logs + timestamp filtering
7. `test_hiking_display_graph` — Graph renders 10 data points on TFT
8. `test_hiking_display_menu` — Menu renders all 7 items
9. `test_hiking_display_comfort_ui` — Comfort UI renders all 5 levels
10. `test_hiking_full_workflow` — End-to-end: sensor → SD → display graph
11. `test_hiking_comfort_workflow` — End-to-end: sensor → comfort log → query
12. `test_hiking_timing` — Sensor + store + read <100ms

## Recent Changes (2026-07-22)

- **BLE double-free crash fixed** — NimBLE `deinit(true)` causes heap corruption on Tasmota ESP32-C6; switched to `deinit(false)` with callback cleanup before deinit
- **Black display after deep sleep fixed** — `gpio_hold_dis(TFT_LIT)` at boot; ESP32-C6 doesn't auto-release GPIO hold after wake
- **SD data preservation** — removed unconditional datalog deletion from `StorageManager::begin()`; data now persists across boot cycles
- **BLE sync progress countdown** — TFT shows "BLE 9s...", "BLE 8s..." during sync; `ProgressCallback` added to `sync()`/`syncBLE()` API
- **BLE timeout reduced** — 30s → 10s (`BLE_SYNC_TIMEOUT_MS`)
- **WiFi config via LittleFS** — `/wifi_config.txt` with defaults fallback; `WiFiConfig` struct
- **Mock build fixes** — excluded `hardware_test.cpp` and `display_manager` from native build; fixed `utils.cpp` exclude path

## Recent Changes (2026-07-21)

- **BLE time sync service added** — `TimeSyncService` library with NimBLE-Arduino, 5 configurable sync modes (OFF/BLE/WiFi/BLE+WiFi/WiFi+BLE), LittleFS persistence
- **OTA updates** — ElegantOTA + AsyncWebServer on port 8080, partition table updated (ota_4mb.csv), TFT progress bar, auth (admin/hikingstation)
- **Battery management** — MAX17048 fuel gauge library, I2C power control (GPIO20), TFT battery display with color-coded alerts
- **Menu expanded** to 7 items: Graph Temp / Graph Humidity / Graph Altitude / Log Comfort / OTA / Sync Time / Sleep
- **Display mode auto-sync** — time sync runs on wake per configured mode, no manual step needed
- **`setBrightness()` bug fixed** — was inverting PWM value; fixed to raw passthrough
- **Test runner format bug fixed** — missing colon between file path and line number
- **Starlight wiki** — 16 pages, GitHub Pages auto-deploy workflow
- **External RTC removed** — user confirmed removing RV-3028-C7 from PCB; ESP32 internal RTC + BLE/WiFi sync sufficient

## What's Missing / Needs Work

1. **BLE sync blocks display mode** — time sync waits up to 10s for phone before showing menu; consider making non-blocking or skipping on button wake
2. **Test coverage gaps** — display (1 standalone test), storage has many tests commented out
3. **Comfort file not cleaned between test runs** — `/comfort.csv` accumulates across test runs
4. **Flash usage at 64%** — healthy headroom after partition table optimization

## Known Issues

- The `env:service` environment references `test/test_service/includes` but may not have all dependencies resolved

## Platform Quirk

Uses the **Tasmota fork** of platform-espressif32 for Arduino framework support on ESP32-C6. If you see `Error: This board doesn't support arduino framework!`, run `pio run -t clean` and reinstall packages with `pio pkg uninstall && pio pkg install`.

## Build Commands

```sh
pio run -e main              # build firmware (83% flash, 13% RAM)
pio run -t upload -e main    # upload + auto-opens serial monitor
pio run -t clean             # full clean before rebuilding
pio test -e main             # run all hardware tests on device (34 tests)
pio test -e mock             # run mock tests on host (68 tests)
pio device monitor           # open serial monitor manually
```

## Code Style Notes

- Pin definitions live in `include/config.h`
- Shared data structures in `include/data_structures.h`
- Logging uses macros `LOG_INFO(...)`, `LOG_ERROR(...)`, etc. from `include/logger.h`
- Conditional compilation: `#ifdef MOCK` swaps Arduino types for standard C++ in mock builds
- `time_t` on ESP32-C6 is 64-bit — always cast to `unsigned long` when using `%lu` format specifier

## Wiki

Full documentation lives in `docs/` (Starlight/Astro). Run locally:

```sh
cd docs && bun run dev
```
