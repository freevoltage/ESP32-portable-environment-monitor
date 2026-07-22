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
pio test -e main         # embedded tests (runs on device via Unity)
pio test -e service      # service-layer tests on device
pio test -e native       # host-native tests (no hardware)
pio test -e mock         # mock tests (env:mock in platformio.ini; test/test_mock/ not yet created)
```

Tests use a custom `TestContext` fixture system (`lib/test_fixture/test_fixture.h`) and a custom Unity runner (`lib/test_runner/`). Individual test suites are enabled/disabled by uncommenting `run_tests()` calls in the test runner .cpp files.

## Architecture

```
src/main.cpp            # firmware entrypoint (setup/loop, deep sleep)
lib/
  sensor_manager/       # BME280 I2C sensor
  rtc_manager/          # ESP32Time RTC
  display_manager/      # ST7789 TFT display
  storage_manager/      # SD card (SPI)
  wifi_manager/         # WiFi connection
  connectivity_service/ # WiFi + NTP time sync (service layer)
  data_service/         # orchestrates sensor/storage/rtc
  display_service/      # display service (older)
  test_runner/          # custom Unity test runner with PlatformIO-compatible output
  test_fixture/         # setUp/tearDown dispatch system for multiple test namespaces
  hardware/             # empty dirs (planned hardware abstraction layer)
  services/             # alternate service implementations (older copies)
```

**Important**: There are duplicate libraries at two levels -- `lib/data_service/` and `lib/services/data/`, `lib/connectivity_service/` and `lib/services/connectivity/`, `lib/display_service/` and `lib/services/display/`. The top-level versions are the ones used by the build. The `lib/services/` copies are older/alternate implementations.

## Platform Quirk

The project uses the **Tasmota fork** of platform-espressif32 (not the official Espressif platform) to get Arduino framework support for ESP32-C6. If you see `Error: This board doesn't support arduino framework!`, run `pio run -t clean` and reinstall packages with `pio pkg uninstall && pio pkg install`.

## Conditional Compilation (MOCK)

Headers use `#ifdef MOCK` to swap Arduino types for standard C++ types when running on the host. The `env:mock` environment passes `-DMOCK` as a build flag. When adding new headers that use `Arduino.h` types (e.g., `String`, `time_t`), wrap Arduino includes in `#ifndef MOCK` guards. See `include/data_structures.h` and `include/logger.h` for the pattern.

## Code Style Notes

- Pin definitions live in `include/config.h`
- Shared data structures (`SensorReading`, `TemperatureStats`, `SystemStatus`, enums) are in `include/data_structures.h`
- Logging uses macros `LOG_INFO(...)`, `LOG_ERROR(...)`, etc. from `include/logger.h` (auto-injects function name)
- WiFi credentials are currently hardcoded in `include/config.h`

## Known Issues

- `lib/hardware/` contains empty subdirectories (display, rtc, sensor, storage, wifi) -- not yet implemented
- `test/test_mock/` directory referenced by `env:mock` in platformio.ini does not exist yet
- The `env:service` environment references `test/test_service/includes` but may not have all dependencies resolved
- `app.h` is entirely commented out (planned App class not yet active)
