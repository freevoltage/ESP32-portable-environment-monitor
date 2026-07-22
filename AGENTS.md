# AGENTS.md

## Project

PlatformIO embedded C++ project for Adafruit Feather ESP32-C6. Reads BME280 sensor, displays on ST7789 TFT, logs to SD card, syncs time via WiFi/NTP, then enters deep sleep. Uses Unity test framework.

## Build & Run

```sh
pio run -e main          # build firmware
pio run -t upload -e main  # upload + auto-opens serial monitor (via scripts/auto_monitor.py)
pio run -t clean         # full clean before rebuilding after platform/lib changes
pio device monitor       # open serial monitor manually
```

## Test

```sh
pio test -e main         # embedded tests (runs on device via Unity) — 34 tests
pio test -e mock         # mock tests (on host, no hardware) — 68 tests
```

Tests use a custom `TestContext` fixture system (`lib/test_fixture/test_fixture.h`) and a custom Unity runner (`lib/test_runner/`). Individual test suites are enabled/disabled by uncommenting `run_tests()` calls in the test runner .cpp files.

## Architecture

```
src/main.cpp            # firmware entrypoint (setup/loop, deep sleep, dashboard, menus)
lib/
  hardware/             # HAL — direct hardware access
    sensor_manager/     # BME280 I2C
    display_manager/    # ST7789 TFT (240x240)
    storage_manager/    # SD card (SPI)
    rtc_manager/        # ESP32Time
    wifi_manager/       # WiFi
    battery_manager/    # MAX17048 fuel gauge
  services/             # Service layer — orchestrates hardware
    data/               # DataService (sensor + storage + RTC)
    display/            # DisplayService (menus, graphs, comfort UI)
    connectivity/       # ConnectivityService (WiFi + NTP)
    time_sync_service/  # TimeSyncService (BLE phone sync + WiFi fallback)
  test_runner/          # custom Unity test runner
  test_fixture/         # setUp/tearDown dispatch system
```

**Important**: There are duplicate libraries at two levels — top-level `lib/data_service/`, `lib/connectivity_service/`, `lib/display_service/` are older copies. The `lib/services/` versions are the ones used by the build.

## UI Navigation Rules

**Two buttons**: A (GPIO8) = Navigate, B (GPIO3) = Select.

| Input | Action |
|-------|--------|
| **Button A** | Navigate (cycle items) |
| **Button B** | Select (activate highlighted item) |
| **Both A+B** | **Abort** — always returns to Dashboard |

Both-buttons abort works from **every screen**: comfort logging, sync sub-menu, graphs, and the full menu.

**Dashboard** (first screen on button wake): Shows sensor data + time + battery. Three items: Log Comfort, Menu, Sleep.

**Comfort logging**: One log per day max. If already logged today, shows "Already logged today!" and returns.

**`waitForButton()`** in `main.cpp`: returns 1 (NAV), 2 (SEL), or 3 (BOTH/abort). All button loops use this helper.

## Platform Quirk

The project uses the **Tasmota fork** of platform-espressif32 (not the official Espressif platform) to get Arduino framework support for ESP32-C6. If you see `Error: This board doesn't support arduino framework!`, run `pio run -t clean` and reinstall packages with `pio pkg uninstall && pio pkg install`.

## Conditional Compilation (MOCK)

Headers use `#ifdef MOCK` to swap Arduino types for standard C++ types when running on the host. The `env:mock` environment passes `-DMOCK` as a build flag. When adding new headers that use `Arduino.h` types (e.g., `String`, `time_t`), wrap Arduino includes in `#ifndef MOCK` guards. See `include/data_structures.h` and `include/logger.h` for the pattern.

## Code Style Notes

- Pin definitions live in `include/config.h`
- Shared data structures (`SensorReading`, `TemperatureStats`, `SystemStatus`, enums) are in `include/data_structures.h`
- Logging uses macros `LOG_INFO(...)`, `LOG_ERROR(...)`, etc. from `include/logger.h` (auto-injects function name)
- WiFi credentials are configurable via LittleFS (`/wifi_config.txt`), fallback to `config.h` defaults
- Debug logging to SD card: `storage.logDebug(tag, message)` appends to `/debug.log`

## Known Issues

- `test/test_mock/` directory referenced by `env:mock` in platformio.ini does not exist yet
- `env:service` environment references `test/test_service/includes` but may not have all dependencies resolved
