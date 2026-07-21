# AGENTS.md

## Project

PlatformIO embedded C++ project for Adafruit Feather ESP32-C6. Portable hiking weather station — reads BME280 sensor, displays on ST7789 TFT, logs to SD card, syncs time via WiFi/NTP, then enters deep sleep. Two-mode operation: silent measurement (timer wake) and interactive display (button wake). Uses Unity test framework.

## Build & Run

```sh
pio run -e main              # build firmware
pio run -t upload -e main    # upload + auto-opens serial monitor (via scripts/auto_monitor.py)
pio run -t clean             # full clean before rebuilding after platform/lib changes
pio device monitor           # open serial monitor manually
```

## Test

```sh
pio test -e main             # embedded tests — all hardware + hiking station integration (34 tests)
pio test -e service          # service-layer tests on device
pio test -e mock             # mock tests on host (33 tests, no hardware needed)
pio test -e native           # host-native tests (placeholder, no real tests)
```

Tests use a custom `TestContext` fixture system (`lib/test_fixture/test_fixture.h`) and a custom Unity runner (`lib/test_runner/`). Hardware tests are in `test/test_hardware/`, mock tests in `test/test_mock/`.

## Architecture

```
src/main.cpp                 # firmware entrypoint (two-mode: measurement + display, deep sleep)
include/
  config.h                   # pin definitions, timing constants, filenames, WiFi creds
  data_structures.h          # SensorReading, ComfortLevel, DisplayMenu, enums
  logger.h                   # LOG_INFO / LOG_ERROR macros (auto-injects function name)

lib/
  hardware/                  # HAL layer — direct hardware access
    sensor_manager/          # BME280 I2C (temp, humidity, pressure, altitude)
    rtc_manager/             # ESP32Time RTC
    display_manager/         # ST7789 240x240 TFT (GFX-based drawGraph, menus)
    storage_manager/         # SD card (SPI) — sensor logs + comfort logs
    wifi_manager/            # WiFi connection
  services/                  # service layer — orchestrates hardware
    data/                    # DataService: sensor → storage → RTC orchestration
    display/                 # DisplayService: startup, readings, graphs, menus, comfort UI
    connectivity/            # ConnectivityService: WiFi + NTP time sync
  test_runner/               # custom Unity test runner with PlatformIO-compatible output
  test_fixture/              # setUp/tearDown dispatch system for multiple test namespaces
  utils/                     # DateTimeUtils, I2CScanner, test helpers
```

**Important**: The build uses `lib/hardware/` and `lib/services/` via `lib_extra_dirs` in `platformio.ini` with `lib_ldf_mode = deep`. There are older copies of services at `lib/data_service/`, `lib/connectivity_service/`, `lib/display_service/` — these are NOT used by the build.

## Platform Quirk

The project uses the **Tasmota fork** of platform-espressif32 (not the official Espressif platform) to get Arduino framework support for ESP32-C6. If you see `Error: This board doesn't support arduino framework!`, run `pio run -t clean` and reinstall packages with `pio pkg uninstall && pio pkg install`.

## Conditional Compilation (MOCK)

Headers use `#ifdef MOCK` to swap Arduino types for standard C++ types when running on the host. The `env:mock` environment passes `-DMOCK` as a build flag. When adding new headers that use `Arduino.h` types (e.g., `String`, `time_t`), wrap Arduino includes in `#ifndef MOCK` guards. See `include/data_structures.h` and `include/logger.h` for the pattern.

## Code Style Notes

- Pin definitions live in `include/config.h`
- Shared data structures (`SensorReading`, `TemperatureStats`, `SystemStatus`, enums) are in `include/data_structures.h`
- Logging uses macros `LOG_INFO(...)`, `LOG_ERROR(...)`, etc. from `include/logger.h` (auto-injects function name)
- WiFi credentials are currently hardcoded in `include/config.h`
- `time_t` on ESP32-C6 is 64-bit (`__int_least64_t`) — always cast to `unsigned long` for `%lu` format specifiers

## Known Issues

- The `env:service` environment references `test/test_service/includes` but may not have all dependencies resolved

## Wiki

Full documentation lives in `docs/` (Starlight/Astro). Run locally:

```sh
cd docs && bun run dev
```

Wiki should be updated when adding new features, changing configuration, or modifying the architecture.
