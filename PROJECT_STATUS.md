# Project Status — ESP32 Hiking Weather Station

Last updated: 2026-07-20

## Overview

PlatformIO embedded C++ project for **Adafruit Feather ESP32-C6**. Portable hiking weather station that reads BME280 sensor, stores data to SD card, syncs time via WiFi/NTP, and displays 24h rolling graphs on a 1.54" ST7789 TFT. Two-mode operation: silent measurement mode (timer wake) and interactive display mode (button wake). Uses Unity test framework.

## Architecture

```
Application (main.cpp)
    ├── DataService        → orchestrates sensor/storage/rtc
    ├── DisplayService     → manages display output (menu, graphs, comfort UI)
    └── ConnectivityService → WiFi + NTP time sync
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
| Buttons | GPIO9 (Navigate), GPIO3 (Select) | Active LOW, internal pullup |
| Backlight | Active-HIGH | `TFT_BACKLIGHT_INVERTED = 1` |
| Display rotation | `TFT_ROTATION = 2` | |

## Firmware Modes

### Measurement Mode (timer wake)
- Display OFF, WiFi OFF
- Read sensors → store to SD → sleep immediately
- ~2-3 seconds total, wakes every 30 minutes (`MEASUREMENT_INTERVAL_SEC = 1800`)

### Display Mode (button wake via EXT1)
- Turn on display → WiFi/NTP sync → show current reading
- Navigate menu: Graph Temp / Graph Humidity / Graph Altitude / Log Comfort / Sleep
- 24h rolling graph using `getReadingsSince()`
- Comfort logging: 5 levels (Too cold → Too warm) → `/comfort.csv`
- EXT1 wake on both buttons: `(1ULL << GPIO9) | (1ULL << GPIO3)`, `ESP_EXT1_WAKEUP_ANY_LOW`

## Shared Data Structures (`include/data_structures.h`)

- `SensorReading` — temp, humidity, pressure, altitude, timestamp, isValid
- `TemperatureStats` — min, max, average, sampleCount, isValid
- `SystemStatus` — sensor/display/storage/rtc/wifi ok, freeMemory, uptime
- `ComfortLevel` — enum class : uint8_t (TOO_COLD=0..TOO_WARM=4)
- `ComfortLog` — timestamp + ComfortLevel
- `DisplayMenu` — enum class : uint8_t (GRAPH_TEMP..SLEEP)
- `SystemMode`, `ConnectivityStatus` — state machine enums

## Fully Working (tested on hardware + native)

### Hardware Managers (`lib/hardware/`)

| Module            | Location                        | Status                      | Tests              |
| ----------------- | ------------------------------- | --------------------------- | ------------------ |
| `sensor_manager`  | `lib/hardware/sensor_manager/`  | Complete (BME280 + altitude)| 6 embedded         |
| `storage_manager` | `lib/hardware/storage_manager/` | Complete (~1030 lines)      | 5 active embedded  |
| `rtc_manager`     | `lib/hardware/rtc_manager/`     | Complete (ESP32Time)        | 11 embedded        |
| `display_manager` | `lib/hardware/display_manager/` | Complete (240x240 ST7789)   | 1 embedded         |
| `wifi_manager`    | `lib/hardware/wifi_manager/`    | Complete (WiFi + NTP)       | 0 tests            |

### Service Layer (`lib/services/`)

| Module                 | Location                            | Status                    | Tests                     |
| ---------------------- | ----------------------------------- | ------------------------- | ------------------------- |
| `data_service`         | `lib/services/data/`                | Complete (232 lines)      | 21 native mock + 21 on-device |
| `display_service`      | `lib/services/display/`             | Complete (~120 lines)     | Tested on hardware (menu, graph, comfort UI) |
| `connectivity_service` | `lib/services/connectivity/`        | Complete (116 lines)      | 12 native mock            |

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
| `mock`    | Native (Mac) | DataService + ConnectivityService + storage mocks     | **33 tests pass**, 0 hardware     |
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
8. `test_hiking_display_menu` — Menu renders all 5 items
9. `test_hiking_display_comfort_ui` — Comfort UI renders all 5 levels
10. `test_hiking_full_workflow` — End-to-end: sensor → SD → display graph
11. `test_hiking_comfort_workflow` — End-to-end: sensor → comfort log → query
12. `test_hiking_timing` — Sensor + store + read <100ms

## Recent Changes (2026-07-20)

- **Hiking station redesign complete** — `main.cpp` rewritten as two-mode state machine (measurement vs display)
- **Comfort log bug fixed** — `time_t` on ESP32-C6 is 64-bit (`__int_least64_t`), but `%lu` expects 32-bit `unsigned long`. Fixed by explicit casts in `storeComfortLog()` and `getComfortLogsSince()`
- **Git branch `dev`** — 20 commits ahead of `origin/dev` (19 diverged)
- Latest commits: `9eadf56` (comfort log fix), `0e28d8c` (hardware test compilation), `e99bef6` (integration test suite), `33ca951` (main app build prep)

## What's Missing / Needs Work

1. **Test coverage gaps** — WiFi (0 tests), display (1 standalone test), storage has many tests commented out
2. **Comfort file not cleaned between test runs** — `/comfort.csv` accumulates across test runs (stale entries from prior tests appear in `test_hiking_comfort_multiple` but don't break it)
3. **WiFi credentials** — hardcoded in `include/config.h` (should be configurable)
4. **GitHub Pages deployment** — wiki built, needs deploy workflow

## Known Issues

- Filename typo: `connectivity_serivce.h` (not `service`) in both `lib/connectivity_service/` and `lib/services/connectivity/`
- The `env:service` environment references `test/test_service/includes` but may not have all dependencies resolved

## Platform Quirk

Uses the **Tasmota fork** of platform-espressif32 for Arduino framework support on ESP32-C6. If you see `Error: This board doesn't support arduino framework!`, run `pio run -t clean` and reinstall packages with `pio pkg uninstall && pio pkg install`.

## Build Commands

```sh
pio run -e main              # build firmware (52% flash, 11% RAM)
pio run -t upload -e main    # upload + auto-opens serial monitor
pio run -t clean             # full clean before rebuilding
pio test -e main             # run all hardware tests on device
pio test -e mock             # run mock tests on host
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
