# ESP32 Hiking Weather Station

A portable hiking weather station built with **Adafruit Feather ESP32-C6**, **BME280 sensor**, and **1.54" ST7789 TFT display**. Reads temperature, humidity, pressure, and altitude — logs data to SD card silently every 30 minutes, then displays 24h rolling graphs on button wake.

## Features

- **BME280 sensor** — temperature, humidity, pressure, altitude (I2C)
- **1.54" color TFT** — 240x240 IPS ST7789 display
- **SD card logging** — CSV format, sensor data + comfort logs
- **Two-mode operation** — silent measurement (timer wake) + interactive display (button wake)
- **Deep sleep** — ~2-3 sec wake time, 30 min intervals
- **WiFi/NTP sync** — automatic time synchronization on display wake
- **24h rolling graphs** — visualize temperature, humidity, altitude history
- **Comfort logging** — 5 levels from "Too cold" to "Too warm"

## Hardware

| Component | Model | Connection |
|-----------|-------|------------|
| MCU | Adafruit Feather ESP32-C6 | RISC-V 32-bit, 4MB Flash |
| Sensor | BME280 | I2C |
| Display | 1.54" 240x240 IPS ST7789 | SPI |
| Storage | SD card module | SPI (separate from display) |
| Button A | Navigate | GPIO9 (active LOW, internal pullup) |
| Button B | Select | GPIO3 (active LOW, internal pullup) |

### Pin Mapping

| Signal | GPIO | Notes |
|--------|------|-------|
| `SD_CS` | 0 | SD card chip select |
| `TFT_LIT` | 2 | Backlight (active-HIGH) |
| `SEL_BUTTON_PIN` | 3 | Select button |
| `TFT_CS` | 5 | Display chip select |
| `TFT_RST` | 6 | Display reset |
| `TFT_DC` | 7 | Display data/command |
| `NAV_BUTTON_PIN` | 9 | Navigate button (BOOT) |

Pin definitions live in `include/config.h`.

## Architecture

```
Application (main.cpp)
    │
    ├── DataService        → sensor + storage + RTC orchestration
    ├── DisplayService     → menus, graphs, comfort UI
    └── ConnectivityService → WiFi + NTP time sync
            │
        HAL Layer (lib/hardware/)
            ├── SensorManager    (BME280 I2C)
            ├── DisplayManager   (ST7789 TFT)
            ├── StorageManager   (SD card SPI)
            ├── RTCManager       (ESP32Time)
            └── WiFiManager      (WiFi + NTP)
```

**`main.cpp` never calls hardware managers directly** — all interactions go through the service layer.

## Project Structure

```
src/main.cpp                    # Firmware entrypoint (two-mode state machine)
include/
  config.h                      # Pin definitions, timing, WiFi creds
  data_structures.h             # SensorReading, ComfortLevel, DisplayMenu, enums
  logger.h                      # LOG_INFO / LOG_ERROR macros

lib/
  hardware/                     # HAL layer — direct hardware access
    sensor_manager/             # BME280 I2C
    rtc_manager/                # ESP32Time RTC
    display_manager/            # ST7789 240x240 TFT
    storage_manager/            # SD card (SPI)
    wifi_manager/               # WiFi connection
  services/                     # Service layer — orchestrates hardware
    data/                       # DataService
    display/                    # DisplayService
    connectivity/               # ConnectivityService
  test_runner/                  # Custom Unity test runner
  test_fixture/                 # setUp/tearDown dispatch
  utils/                        # DateTimeUtils, I2CScanner

test/
  test_hardware/                # Hardware integration tests (34 tests)
  test_mock/                    # Mock tests on host (33 tests)
  test_service/                 # Service tests on device

docs/                           # Starlight wiki (documentation site)
```

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
|-------------|---------|
| `main` | Firmware build, flash, and hardware tests |
| `mock` | Mock tests on host (no hardware needed) |
| `service` | Service-layer tests on device |
| `display` | Display manager standalone example |
| `clock` | Clock example (DisplayService + WiFi + RTC) |

## Testing

Tests use the [Unity](https://www.throwtheswitch.org/unity) test framework with a custom runner and fixture system.

```sh
pio test -e main             # hardware tests (34 tests, on device)
pio test -e mock             # mock tests (33 tests, on host)
pio test -e service          # service tests (on device)
```

### Test Environments

| Environment | Platform | What it tests | Tests |
|------------|----------|---------------|-------|
| `main` | ESP32-C6 | All hardware + hiking station integration | 34 |
| `mock` | Host (Mac) | DataService + ConnectivityService mocks | 33 |
| `service` | ESP32-C6 | DataService with real hardware | 21 |

### Test Architecture

- **Custom test runner** (`lib/test_runner/`): Overrides Unity's `RUN_TEST` macro to print PlatformIO-compatible result lines (`file:line:name:PASS/FAIL`).
- **Fixture system** (`lib/test_fixture/`): `TestContext` singleton lets multiple test namespaces register their own `setUp`/`tearDown` functions.

## Firmware Modes

### Measurement Mode (timer wake)

Wakes every 30 minutes via timer:
1. Read BME280 sensor (temperature, humidity, pressure, altitude)
2. Store reading to SD card (`/datalog.csv`)
3. Enter deep sleep immediately

Display OFF, WiFi OFF. Total wake time: ~2-3 seconds.

### Display Mode (button wake via EXT1)

Wake by pressing Navigate (GPIO9) or Select (GPIO3):
1. Turn on display, connect WiFi, sync time via NTP
2. Show current sensor reading
3. Navigate menu: Graph Temp / Graph Humidity / Graph Altitude / Log Comfort / Sleep
4. 24h rolling graphs using `getReadingsSince()`

## SD Card Files

| File | Format | Content |
|------|--------|---------|
| `/datalog.csv` | `epoch,temp,humidity,pressure,altitude` | Sensor readings |
| `/comfort.csv` | `epoch,level` | Comfort level logs |

## Documentation

Full documentation lives in `docs/` (Starlight/Astro):

```sh
cd docs && bun run dev       # start dev server
```

Sections: Getting Started, Architecture, Configuration, Development, API Reference.

## Hardware Notes

### GPIO9 Internal Pull-Up

GPIO9 has an internal weak pull-up resistor enabled by default (ESP32-C6 Technical Reference Manual, Appendix A, pages 77-78). You cannot use GPIO9 to power-gate the display.

### Deep Sleep GPIO Hold

During deep sleep, GPIO2 (backlight) is held LOW via `gpio_hold_en()` to prevent backlight leakage. Guarded by `HOLD_GPIO_IN_SLEEP` in `config.h`. Alternative: pull-down resistor from `TFT_LIT` to GND on the display board.

### Display Backlight

Backlight is active-HIGH (`HIGH` = ON, `LOW` = OFF). `TFT_BACKLIGHT_INVERTED = 1` in `config.h`. The `setBrightness()` method is a raw PWM passthrough — polarity is handled by `BL_ON`/`BL_OFF` macros.

### Platform Fork

This project uses the [Tasmota fork](https://github.com/tasmota/platform-espressif32) of platform-espressif32 for Arduino framework support on ESP32-C6.

If you see `Error: This board doesn't support arduino framework!`:
```sh
pio run -t clean
pio pkg uninstall && pio pkg install
```

## Code Style

- Pin definitions in `include/config.h`
- Shared types in `include/data_structures.h`
- Logging via `LOG_INFO(...)`, `LOG_ERROR(...)` macros from `include/logger.h`
- WiFi credentials hardcoded in `include/config.h`
- `time_t` on ESP32-C6 is 64-bit — always cast to `unsigned long` for `%lu`
- Conditional compilation: `#ifdef MOCK` swaps Arduino types for standard C++ in mock builds

## License

MIT
