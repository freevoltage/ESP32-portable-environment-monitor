# Project Status — ESP32 BME280 Weather Station

Last updated: 2026-07-19

## Fully Working (tested on hardware + native)

| Module | Status | Tests |
|---|---|---|
| `sensor_manager` | Complete | 6 embedded |
| `storage_manager` | Complete (~935 lines) | 5 active embedded |
| `rtc_manager` | Complete | 11 embedded |
| `display_manager` | Complete (one stub: `showConnectivityStatus`) | 1 embedded |
| `wifi_manager` | Complete (WiFi + NTP) | 0 tests |
| `data_service` | Complete (232 lines) | 21 native mock + 21 on-device |
| `test_runner` | Complete (custom Unity runner) | — |
| `test_fixture` | Complete (setUp/tearDown dispatch) | — |
| `utils` | Complete (DateTimeUtils + test helpers) | — |

## Firmware (`src/main.cpp`)

**Working:** Sensor read, SD storage, display, RTC init, deep sleep config, SPI bus conflict fix

**Commented out / disabled:**
- WiFi/NTP time sync (`syncTime()` call)
- Deep sleep never actually triggers (`esp_deep_sleep_start` commented out)
- LED power-off

## Stub / Empty (not implemented)

| Module | Status |
|---|---|
| `display_service` | Empty `.cpp` — planned service layer for display |
| `connectivity_service` | Empty `.cpp` — planned service layer for WiFi + NTP. Header has filename typo (`serivce`) |
| `lib/hardware/` | 5 empty subdirectories — planned HAL layer |
| `lib/services/` | Legacy copies of data/display/connectivity — superseded by top-level modules |

## Test Infrastructure

| Env | Platform | What it tests | Status |
|---|---|---|---|
| `main` | ESP32 | All hardware (sensor, storage, display, RTC) | 22 tests pass on device |
| `service` | ESP32 | DataService with real hardware | 21 tests pass on device |
| `mock` | Native (Mac) | DataService with mock managers | 21 tests pass, 0 hardware needed |
| `native` | Native (Mac) | Placeholder (2+2=4) | Works, no real tests |

## What's Missing / Needs Work

1. **WiFi/NTP time sync** — `wifi_manager` exists but isn't wired into firmware or service layer
2. **`display_service`** — empty stub, display logic lives directly in `main.cpp`
3. **`connectivity_service`** — empty stub, would orchestrate WiFi + NTP
4. **`lib/hardware/` HAL** — empty, intended abstraction layer
5. **Deep sleep** — configured but never triggered
6. **`storage_manager` cleanup()** — marked TODO, not implemented
7. **Test coverage gaps** — WiFi (0 tests), display (1 test), storage has many tests commented out (24 defined, 5 active)
8. **`lib/services/` legacy copies** — could be cleaned up / deleted
9. **`lib/some_lib/`** — trivial test lib, can be removed
10. **`test/test_native/` and `test/test_lib/`** — placeholder tests with no real content

## The Architectural Question

The stated architecture is **Application → Service → HAL**. Currently:

- **Application** = `main.cpp` (monolithic, handles everything)
- **Service layer** = `data_service` (complete), `display_service` (empty), `connectivity_service` (empty)
- **HAL** = `lib/hardware/` (empty)

The `data_service` is the only service fully implemented. The firmware still calls hardware managers directly from `main.cpp` instead of going through services.
