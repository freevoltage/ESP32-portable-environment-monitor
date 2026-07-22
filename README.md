# ESP32 Hiking Weather Station

A portable hiking weather station built with **Adafruit Feather ESP32-C6**, **BME280 sensor**, and **1.54" ST7789 TFT display**. Reads temperature, humidity, pressure, and altitude — logs data to SD card silently every 30 minutes, then displays 24h rolling graphs on button wake.

## Features

- **BME280 sensor** — temperature, humidity, pressure, altitude (I2C)
- **1.54" color TFT** — 240x240 IPS ST7789 display
- **SD card logging** — CSV format, sensor data + comfort logs
- **Two-mode operation** — silent measurement (timer wake) + interactive display (button wake)
- **Deep sleep** — ~2-3 sec wake time, 30 min intervals
- **WiFi/NTP sync** — automatic time synchronization on display wake
- **BLE phone sync** — sync time from phone app via NimBLE (configurable modes)
- **Battery monitoring** — MAX17048 fuel gauge with color-coded display
- **24h rolling graphs** — visualize temperature, humidity, altitude history
- **Comfort logging** — 5 levels from "Too cold" to "Too warm"

## Hardware

| Component | Model | Connection |
|-----------|-------|------------|
| MCU | Adafruit Feather ESP32-C6 | RISC-V 32-bit, 4MB Flash |
| Sensor | BME280 | I2C |
| Display | 1.54" 240x240 IPS ST7789 | SPI |
| Storage | SD card module | SPI (separate from display) |
| Button A | Navigate | GPIO8 (active LOW, internal pullup) |
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
| `NAV_BUTTON_PIN` | 8 | Navigate button |

Pin definitions live in `include/config.h`.

## Quick Start

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

### Run Tests

```sh
pio test -e main             # 34 hardware tests (on device)
pio test -e mock             # 68 mock tests (on host, no hardware)
```

## Architecture

```
Application (main.cpp)
    ├── DataService         → sensor + storage + RTC
    ├── DisplayService      → menus, graphs, comfort UI
    ├── ConnectivityService → WiFi + NTP
    └── TimeSyncService     → BLE phone sync + WiFi fallback (NimBLE)
            │
        HAL Layer (lib/hardware/)
            ├── SensorManager    (BME280)
            ├── DisplayManager   (ST7789)
            ├── StorageManager   (SD card)
            ├── RTCManager       (ESP32Time)
            ├── WiFiManager      (WiFi)
            └── BatteryManager   (MAX17048)
```

`main.cpp` never calls hardware managers directly — all interactions go through the service layer.

## Firmware Modes

**Measurement mode** (timer wake, every 30 min):
Read sensors → store to SD → deep sleep. Display OFF, WiFi OFF. ~2-3 sec.

**Display mode** (button wake):
Show dashboard → navigate → menu → 24h graphs → comfort logging → sleep.

## User Interface

### Dashboard (first screen on button wake)

```
┌──────────────────────────────┐
│      HIKING STATION          │  ← Header
├──────────────────────────────┤
│ 23.5°C       42%            │  ← Temperature + Humidity
│  1,245 m     1013hPa        │  ← Altitude + Pressure
│                              │
│  14:32                       │  ← Current time
│                              │
├──────────────────────────────┤
│ > Log Comfort                │  ← Selectable item
│   Menu                       │  ← Selectable item
├──────────────────────────────┤
│ ████████░░ 78% 4.02V        │  ← Battery bar
│ A=Navigate  B=Select        │  ← Footer
└──────────────────────────────┘
```

**Button A** cycles between "Log Comfort" and "Menu".
**Button B** selects the highlighted item:
- **Log Comfort** → comfort logging screen (5 levels, log to SD)
- **Menu** → full menu with all options

### Full Menu

```
┌──────────────────────────────┐
│          MENU                │
├──────────────────────────────┤
│ > Graph Temperature          │
│   Graph Humidity             │
│   Graph Altitude             │
│   Log Comfort                │
│   OTA                        │
│   Sync Time                  │
│   Sleep                      │
├──────────────────────────────┤
│ A=Navigate  B=Select        │
└──────────────────────────────┘
```

**Button A** cycles through items. **Button B** selects.

| Item | Action |
|------|--------|
| Graph Temp/Humidity/Altitude | Show 24h rolling graph |
| Log Comfort | Log comfort level to SD |
| OTA | WiFi firmware update (B=Exit, 120s timeout) |
| Sync Time | Sub-menu: Mode / Sync Now / Back |
| Sleep | Enter deep sleep |

### Sync Time Sub-Menu

```
┌──────────────────────────────┐
│         TIME SYNC            │
├──────────────────────────────┤
│ > Mode     BLE+WiFi          │  ← Tap B to cycle modes
│   Sync Now                   │  ← Tap B to start sync
│   Back                       │  ← Tap B to exit
├──────────────────────────────┤
│ Last sync:                   │
│ BLE @ 1721654532             │
│                              │
│ A=Navigate  B=Select        │
└──────────────────────────────┘
```

### OTA Mode

```
┌──────────────────────────────┐
│          OTA MODE            │
├──────────────────────────────┤
│ Open in browser:             │
│ http://192.168.1.42/update   │
│                              │
│      Waiting for             │
│       upload...              │
│                              │
│ Auth: admin / hikingstation  │
│ B=Exit  Timeout=120s         │
└──────────────────────────────┘
```

**Button B** exits OTA. Times out after 120 seconds of inactivity.

## Project Structure

```
src/main.cpp                    # Firmware entrypoint
include/config.h                # Pins, timing, WiFi creds
include/data_structures.h       # SensorReading, enums, shared types

  lib/
  hardware/                     # HAL — direct hardware access
    sensor_manager/             # BME280 I2C
    display_manager/            # ST7789 TFT
    storage_manager/            # SD card (SPI)
    rtc_manager/                # ESP32Time
    wifi_manager/               # WiFi
    battery_manager/            # MAX17048 fuel gauge
  services/                     # Service layer — orchestrates hardware
    data/                       # DataService
    display/                    # DisplayService
    connectivity/               # ConnectivityService
    time_sync_service/          # TimeSyncService (BLE + WiFi)

test/
  test_hardware/                # 34 integration tests (on device)
  test_mock/                    # 68 mock tests (on host)
  test_service/                 # service tests (on device)

docs/                           # Starlight wiki
```

## Documentation

Full documentation in `docs/` (Starlight/Astro):

```sh
cd docs && bun run dev
```

## Platform Note

Uses the [Tasmota fork](https://github.com/tasmota/platform-espressif32) of platform-espressif32 for Arduino framework support on ESP32-C6.

If you see `Error: This board doesn't support arduino framework!`:
```sh
pio run -t clean
pio pkg uninstall && pio pkg install
```

## License

MIT
