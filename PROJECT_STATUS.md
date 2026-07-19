# Project Status — ESP32 BME280 Weather Station

Last updated: 2026-07-19

## Architecture

```
Application (main.cpp)
    ├── DataService       → orchestrates sensor/storage/rtc
    ├── DisplayService    → manages display output
    └── ConnectivityService → WiFi + NTP time sync
            │
        Service Layer (lib/services/)
            │
        HAL Layer (lib/hardware/)
```

`main.cpp` never calls hardware managers directly — all interactions go through service layers.

## Fully Working (tested on hardware + native)

### Hardware Managers (`lib/hardware/`)

| Module            | Location                        | Status                      | Tests              |
| ----------------- | ------------------------------- | --------------------------- | ------------------ |
| `sensor_manager`  | `lib/hardware/sensor_manager/`  | Complete                    | 6 embedded         |
| `storage_manager` | `lib/hardware/storage_manager/` | Complete (~935 lines)       | 5 active embedded  |
| `rtc_manager`     | `lib/hardware/rtc_manager/`     | Complete                    | 11 embedded        |
| `display_manager` | `lib/hardware/display_manager/` | Complete (240x240, inverted backlight) | 1 embedded |
| `wifi_manager`    | `lib/hardware/wifi_manager/`    | Complete (WiFi + NTP)       | 0 tests            |

### Service Layer (`lib/services/`)

| Module                 | Location                           | Status                    | Tests                     |
| ---------------------- | ---------------------------------- | ------------------------- | ------------------------- |
| `data_service`         | `lib/services/data_service/`       | Complete (232 lines)      | 21 native mock + 21 on-device |
| `display_service`      | `lib/services/display_service/`    | Complete (~120 lines)     | 0 (tested on hardware)    |
| `connectivity_service` | `lib/services/connectivity_service/`| Complete (116 lines)     | 12 native mock            |

### Infrastructure

| Module         | Status                                | Tests    |
| -------------- | ------------------------------------- | -------- |
| `test_runner`  | Complete (custom Unity runner)        | —        |
| `test_fixture` | Complete (setUp/tearDown dispatch)    | —        |
| `utils`        | Complete (DateTimeUtils + test helpers)| —       |

## Firmware (`src/main.cpp`)

**Fully active features:**

- Sensor read via DataService
- SD card storage via DataService
- Display output via DisplayService
- RTC init + time tracking
- WiFi/NTP time sync via ConnectivityService
- Deep sleep: BOOT button (GPIO9) triggers sleep, timer wakes after `TIME_TO_SLEEP`
- SPI bus conflict fix (storage init before display)
- DataService integration (no direct hardware calls in main)

## Test Infrastructure

| Env       | Platform     | What it tests                                | Status                           |
| --------- | ------------ | -------------------------------------------- | -------------------------------- |
| `main`    | ESP32        | All hardware (sensor, storage, display, RTC) | 22 tests pass on device          |
| `service` | ESP32        | DataService with real hardware               | 21 tests pass on device          |
| `mock`    | Native (Mac) | DataService + ConnectivityService mocks      | 33 tests pass, 0 hardware needed |
| `native`  | Native (Mac) | Placeholder (2+2=4)                          | Works, no real tests             |

## What's Missing / Needs Work

1. **`storage_manager` cleanup()** — marked TODO, not implemented
2. **Test coverage gaps** — WiFi (0 tests), display (1 test), storage has many tests commented out (24 defined, 5 active)
3. **`test/test_native/` and `test/test_lib/`** — placeholder tests with no real content
4. **`display_manager` stub** — `showConnectivityStatus()` declared but not implemented

## Platform Quirk

Uses the **Tasmota fork** of platform-espressif32 for Arduino framework support on ESP32-C6. If you see `Error: This board doesn't support arduino framework!`, run `pio run -t clean` and reinstall packages.
