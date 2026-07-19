# ESP32 Weather Station

A wearable, water-resistant gadget that measures temperature, air humidity, and pressure using the BME280 IC from Bosch Sensortec. Data is displayed on an OLED display and logged to an SD card. Battery-powered, designed to last at least a few weeks on a single charge.

## Hardware

| Component | Interface | Notes |
|---|---|---|
| Adafruit Feather ESP32-C6 | — | 4MB Flash, 320KB RAM |
| BME280 | I2C | Temperature, humidity, pressure |
| ST7789 240x135 TFT | SPI | Shared SPI bus with SD card |
| SD Card Module | SPI | CSV data logging |

### Pin Mapping

| Pin | GPIO | Function |
|---|---|---|
| `SD_CS` | 0 | SD card chip select |
| `TFT_CS` | 5 | Display chip select |
| `TFT_RST` | 6 | Display reset |
| `TFT_DC` | 7 | Display data/command |
| `TFT_LIT` | 2 | Display backlight |

Pin definitions live in `include/config.h`.

## Architecture

The project follows a three-layer architecture:

- **Application Layer** — `src/main.cpp` orchestrates the full cycle: init hardware, read sensor, display, store, sleep
- **Service Layer** — Coordinates multiple managers (e.g. `DataService` orchestrates sensor + storage + RTC). Not yet fully wired into main.
- **Hardware Abstraction Layer** — Manager classes that wrap individual peripherals (`SensorManager`, `DisplayManager`, `StorageManager`, `RTCManager`, `WiFiManager`)

Currently `main.cpp` uses the hardware managers directly. The service layer (`DataService`, `ConnectivityService`, `DisplayService`) is implemented but not yet integrated into the main application flow.

## Project Structure

```
src/
  main.cpp                 # Firmware entrypoint (setup/loop, deep sleep)

include/
  config.h                 # Pin definitions, WiFi creds, constants
  data_structures.h        # Shared types (SensorReading, TemperatureStats, enums)
  logger.h                 # LOG_INFO/LOG_ERROR macros with auto function name
  utils.h                  # DateTimeUtils (string ↔ timestamp)
  app.h                    # Planned App class (commented out)

lib/
  sensor_manager/          # BME280 I2C sensor driver
  rtc_manager/             # ESP32Time RTC wrapper
  display_manager/         # ST7789 TFT display driver
  storage_manager/         # SD card CSV logging (largest library ~930 lines)
  wifi_manager/            # WiFi connection + NTP time sync

  data_service/            # Orchestrates sensor/storage/rtc
  connectivity_service/    # WiFi + NTP orchestration (header only, .cpp empty)
  display_service/         # Display orchestration (header only, .cpp empty)

  test_runner/             # Custom Unity runner with PlatformIO-compatible output
  test_fixture/            # setUp/tearDown dispatch for multiple test namespaces
  utils/                   # DateTimeUtils implementation + test helpers

  services/                # Older copies of service libs (not used by build)
  hardware/                # Empty subdirs — planned HAL (not implemented)

test/
  test_embedded/           # On-device tests (Unity, runs on ESP32)
  test_service/            # Service-layer tests (on device)
  test_native/             # Host-native tests (no hardware)
  test_lib/                # Library smoke tests
```

**Note:** There are duplicate libraries at two levels — e.g. `lib/data_service/` and `lib/services/data/`. The top-level versions are used by the build. The `lib/services/` copies are older and reference outdated APIs.

## Getting Started

### Prerequisites

- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) or VS Code + PlatformIO extension
- Adafruit Feather ESP32-C6 connected via USB

### Build & Flash

```sh
pio run -e main              # build firmware
pio run -t upload -e main    # flash (auto-opens serial monitor)
pio device monitor           # open serial monitor manually
pio run -t clean             # clean build (run after platform/lib changes)
```

The upload automatically opens the serial monitor via `scripts/auto_monitor.py`.

### PlatformIO Environments

| Environment | Purpose |
|---|---|
| `main` | Firmware build and flash (default) |
| `service` | Service-layer tests on device |
| `native` | Host-native tests (no hardware needed) |
| `sd` | SD card continuous write stability test |
| `sd_read` | Dump all SD card content to serial |
| `sd_delete` | Delete datalog.csv from SD card |
| `display` | Display manager standalone example |
| `mock` | Mock tests (env configured, test dir not yet created) |

## Testing

Tests use the [Unity](https://www.throwtheswitch.org/unity) test framework with a custom runner and fixture system.

```sh
pio test -e main         # embedded tests (runs on device)
pio test -e service      # service tests (on device)
pio test -e native       # native host tests
```

**Custom test runner** (`lib/test_runner/`): Overrides Unity's `RUN_TEST` macro to print PlatformIO-compatible result lines (`file:line:name:PASS/FAIL`).

**Fixture system** (`lib/test_fixture/`): `TestContext` singleton lets multiple test namespaces register their own `setUp`/`tearDown` functions. Global `setUp()`/`tearDown()` dispatch to the active namespace.

Individual test suites are enabled/disabled by uncommenting `run_tests()` calls in the test runner `.cpp` files.

## Hardware Notes

### GPIO9 Internal Pull-Up

GPIO9 has an internal weak pull-up resistor enabled by default (documented in ESP32-C6 Technical Reference Manual, Appendix A, pages 77-78). This means you cannot use GPIO9 to power-gate the display — it will always source some current.

### Display Power

The ST7789 display draws ~30mA total. Options for power management:

- **GPIO high-drive mode** — A single GPIO in high-current drive mode (40mA) can power the display directly. Automatically cuts power during deep sleep.
- **PMOS transistor** — Use a PMOS on the 3V rail, driven by a GPIO (or via NPN for level shifting). Can leverage GPIO hold during deep sleep to keep the display off.
- **Desolder pull-up resistor** — Remove the default pull-up on the display's power pin and solder a pull-down instead. Cleanest solution, no extra components or software hacks.

The 3V output on the ESP32-C6 Feather stays at 3V even during deep sleep, so external switching is needed if you want to fully cut display power.

### Platform Fork

This project uses the [Tasmota fork](https://github.com/tasmota/platform-espressif32) of platform-espressif32 (not the official Espressif platform) for Arduino framework support on ESP32-C6.

If you see `Error: This board doesn't support arduino framework!`:
```sh
pio run -t clean
pio pkg uninstall && pio pkg install
```

## Known Issues

- Filename typo: `connectivity_serivce.h` (not `service`) in `lib/connectivity_service/` and `lib/services/connectivity/`
- `lib/hardware/` contains empty subdirectories — planned HAL, not yet implemented
- `test/test_mock/` referenced by `env:mock` in `platformio.ini` does not exist yet
- `src/utils.cpp` is a partial duplicate of `lib/utils/utils.cpp` — potential linker conflict
- `app.h` is entirely commented out (planned App class)
- WiFi credentials are hardcoded in `include/config.h`
