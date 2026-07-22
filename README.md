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
            ├── WiFiManager      (WiFi + abort callback)
            ├── BatteryManager   (MAX17048)
            └── SettingsManager  (LittleFS persistence)
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
│      HIKING STATION      WiFi│  ← Header + connectivity indicator
├──────────────────────────────┤
│ 23.5°C       42%            │  ← Temperature + Humidity
│  1,245 m     1013hPa        │  ← Altitude + Pressure
│                              │
│  14:32                       │  ← Current time
│                              │
├──────────────────────────────┤
│ > Log Comfort                │  ← Selectable item
│   Menu                       │  ← Selectable item
│   Sleep                      │  ← Selectable item
├──────────────────────────────┤
│ ████████░░ 78% 4.02V Last:WiFi│  ← Battery bar + last sync source
│ A=Nav B=Sel A+B=Back        │  ← Footer
└──────────────────────────────┘
```

**Button A** cycles between items. **Button B** selects. **Both buttons** = abort back to Dashboard.

| Action | Button B |
|--------|----------|
| **Log Comfort** | Comfort logging (5 levels, one log per day max) |
| **Menu** | Full menu with all options |
| **Sleep** | Enter deep sleep |

### Navigation Rules

| Input | Action |
|-------|--------|
| **Button A** | Navigate (cycle items) |
| **Button B** | Select (activate highlighted item) |
| **Both A+B** | **Abort** — always returns to Dashboard |

Both-buttons abort works from **every screen**: comfort logging, sync sub-menu, graphs, settings, and the full menu. In OTA mode, both buttons also abort (go to sleep).

### Full Menu

```
┌──────────────────────────────┐
│          MENU                │
├──────────────────────────────┤
│ > Graph Temperature          │
│   Graph Humidity             │
│   Graph Altitude             │
│   Settings                   │
│   OTA                        │
│   Sync Time                  │
│   Sleep                      │
├──────────────────────────────┤
│ A=Navigate  B=Select        │
│ A+B=Back to Dashboard       │
└──────────────────────────────┘
```

**Button A** cycles through items. **Button B** selects. **Both buttons** returns to Dashboard.

| Item | Action |
|------|--------|
| Graph Temp/Humidity/Altitude | Show 24h rolling graph |
| Settings | Device settings sub-menu (Sleep Interval, NTP Sync) |
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

Full documentation in `docs/` (Starlight/Astro). Auto-deploys to GitHub Pages on push.

### Local Development

```sh
cd docs
bun install        # first time only
bun run dev        # start dev server (http://localhost:4321/ESP32-portable-environment-monitor/)
```

### Build & Preview

```sh
bun run build      # build static site to dist/
bun run preview    # preview the built site locally
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
