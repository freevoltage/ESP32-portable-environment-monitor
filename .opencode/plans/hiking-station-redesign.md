# Plan: Hiking Weather Station Firmware Redesign

## Overview

Major firmware rewrite to turn the device into a proper portable hiking weather station:
- Deep sleep 99% of the time
- Timer wakes every 30 min for silent measurement (display OFF)
- Button wakes display to show 24h graphs + comfort logging
- Two-button navigation (A=Navigate, B=Select)
- Power-optimized: display only on when user wants it

## Architecture

```
                    ┌─────────────────┐
                    │   DEEP SLEEP    │
                    └────────┬────────┘
                             │
              ┌──────────────┴──────────────┐
              │                             │
         Timer Wake                    EXT1 Wake
         (30 min)                  (Button A or B)
              │                             │
              ▼                             ▼
    ┌─────────────────┐          ┌─────────────────┐
    │ Measurement Mode│          │  Display Mode   │
    │                 │          │                 │
    │ 1. Read sensor  │          │ 1. Turn on LCD  │
    │ 2. Store to SD  │          │ 2. Load 24h data│
    │ 3. Back to sleep│          │ 3. Show graph   │
    │                 │          │ 4. Menu navigate│
    │ Display: OFF    │          │ 5. Comfort log  │
    │ WiFi: OFF       │          │ 6. Back to sleep│
    └─────────────────┘          └─────────────────┘
```

## Menu Structure (Display Mode)

Button A cycles, Button B selects:

```
[Graph: Temperature]  ← default on wake
[Graph: Humidity]
[Graph: Altitude]
[Log Comfort]        ← Button B enters, A cycles 5 levels, B confirms
[Sleep]              ← Button B goes back to deep sleep
```

## Comfort Levels

```cpp
enum class ComfortLevel : uint8_t {
    TOO_COLD = 0,   // "Too cold"
    COLD = 1,       // "Cold"
    COMFORTABLE = 2,// "Comfortable"
    WARM = 3,       // "Warm"
    TOO_WARM = 4    // "Too warm"
};
```

---

## Implementation Steps

### Step 1: Update Data Structures

**`include/data_structures.h`:**

Add altitude to `SensorReading`:
```cpp
struct SensorReading {
    float temperature;
    float humidity;
    float pressure;
    float altitude;      // <-- NEW
    time_t timestamp;
    bool isValid;
    // Update constructors to init altitude = 0
};
```

Add comfort types:
```cpp
enum class ComfortLevel : uint8_t {
    TOO_COLD = 0, COLD = 1, COMFORTABLE = 2, WARM = 3, TOO_WARM = 4
};

struct ComfortLog {
    time_t timestamp;
    ComfortLevel level;
};
```

Add display menu state:
```cpp
enum class DisplayMenu : uint8_t {
    GRAPH_TEMP, GRAPH_HUMIDITY, GRAPH_ALTITUDE,
    LOG_COMFORT, SLEEP
};
```

### Step 2: Update Config

**`include/config.h`:**

```cpp
// Buttons
#define NAV_BUTTON_PIN  BOOT_BUTTON_PIN  // GPIO9 = Navigate (reuse BOOT)
#define SEL_BUTTON_PIN  3                // GPIO3 = Select

// Measurement timing
#define MEASUREMENT_INTERVAL_SEC  (30 * 60)  // 30 minutes in seconds

// Display
#define GRAPH_PADDING    30   // px for axis labels
#define GRAPH_WIDTH      (240 - 2 * GRAPH_PADDING)  // 180px
#define GRAPH_HEIGHT     (240 - 2 * GRAPH_PADDING)  // 180px

// SD card - fresh start
#define DATALOG_FILENAME  "/datalog.csv"
#define COMFORT_FILENAME  "/comfort.csv"
```

### Step 3: Update Storage Manager

**`lib/hardware/storage_manager/src/storage_manager.h/.cpp`:**

1. Change CSV format to include altitude:
   - Header: `Timestamp,Temperature,Humidity,Pressure,Altitude`
   - Data: `%lu,%.2f,%.2f,%.2f,%.2f\n`

2. Add comfort log methods:
   ```cpp
   bool storeComfortLog(const ComfortLog& log);
   bool getComfortLogsSince(time_t since, std::vector<ComfortLog>& logs, uint16_t maxCount = 100);
   ```

3. Create fresh `/datalog.csv` on `begin()` (delete old file, write new header)

4. Create `/comfort.csv` on `begin()` if not exists

5. Update `SensorReading` parsing to include altitude field

### Step 4: Update Sensor Manager

**`lib/hardware/sensor_manager/src/sensor_manager.cpp`:**

- `getReading()` already calls `bme.readAltitude()` internally — just need to store it in the new `altitude` field
- Update `getReading()` to populate `reading.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA)`

### Step 5: Deep Sleep Redesign

**`src/main.cpp`:**

Replace the current single-path flow with a state machine:

```cpp
void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Check wake reason
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause == ESP_SLEEP_WAKEUP_EXT1) {
        // Button wake → display mode
        runDisplayMode();
    } else {
        // Timer or power-on → measurement mode
        runMeasurementMode();
    }

    // Both paths end in deep sleep
    enterDeepSleep();
}

void runMeasurementMode() {
    rtc.begin();
    sensor.begin();
    digitalWrite(TFT_CS, HIGH);
    storage.begin();

    // Read and store
    SensorReading reading = sensor.getReading(rtc.getEpochTime());
    storage.storeReading(reading);

    // No display, no WiFi — just sleep
}

void runDisplayMode() {
    rtc.begin();
    sensor.begin();
    digitalWrite(TFT_CS, HIGH);
    storage.begin();
    display.begin();

    // Optional: WiFi/NTP sync
    // connectivity.ensureTimeSync();

    // Menu loop — stays awake while user interacts
    displayMenuLoop();
}
```

Configure dual-button wake:
```cpp
void enterDeepSleep() {
    displayService.turnOff();

    // Wake on timer (30 min) OR either button press
    esp_sleep_enable_timer_wakeup(MEASUREMENT_INTERVAL_SEC * uS_TO_S_FACTOR);
    esp_sleep_enable_ext1_wakeup(
        (1ULL << NAV_BUTTON_PIN) | (1ULL << SEL_BUTTON_PIN),
        ESP_EXT1_WAKEUP_ALL_LOW
    );

    Serial.flush();
    esp_deep_sleep_start();
}
```

### Step 6: Graph Drawing

**`lib/hardware/display_manager/src/display_manager.h/.cpp`:**

Add graph drawing method using Adafruit GFX primitives:

```cpp
void drawGraph(const char* title, const std::vector<SensorReading>& readings,
               float minVal, float maxVal, bool showAltitude = false);
```

Implementation approach:
1. Clear screen, draw title at top
2. Draw axes: X = time (0-24h), Y = value range
3. Plot points with `drawLine()` between consecutive readings
4. Color code: green=comfortable, blue=cold, red=hot
5. Use `setTextSize(1)` for axis labels

The Adafruit ST7789 library (via Adafruit GFX) provides:
- `drawLine(x0, y0, x1, y1, color)`
- `drawPixel(x, y, color)`
- `drawFastHLine()` / `drawFastVLine()`
- `setTextSize()`, `setTextColor()`, `setCursor()`, `print()`

### Step 7: Menu Navigation

**`src/main.cpp` or new `lib/services/display_service/src/display_service.cpp`:**

Menu state machine in `displayMenuLoop()`:

```cpp
void displayMenuLoop() {
    DisplayMenu currentMenu = DisplayMenu::GRAPH_TEMP;
    bool menuActive = true;

    showGraph(currentMenu);  // Initial display

    while (menuActive) {
        // Button A (Navigate) — cycle menu
        if (digitalRead(NAV_BUTTON_PIN) == LOW) {
            delay(50); // debounce
            if (digitalRead(NAV_BUTTON_PIN) == LOW) {
                currentMenu = nextMenu(currentMenu);
                showMenuOption(currentMenu);
                while (digitalRead(NAV_BUTTON_PIN) == LOW); // wait release
            }
        }

        // Button B (Select) — choose option
        if (digitalRead(SEL_BUTTON_PIN) == LOW) {
            delay(50); // debounce
            if (digitalRead(SEL_BUTTON_PIN) == LOW) {
                handleSelection(currentMenu);
                if (currentMenu == DisplayMenu::SLEEP) {
                    menuActive = false;
                }
                while (digitalRead(SEL_BUTTON_PIN) == LOW); // wait release
            }
        }

        delay(50); // power save in menu loop
    }
}
```

### Step 8: Comfort Logging Interface

When user selects "Log Comfort":
1. Show 5 options with current selection highlighted
2. Button A cycles through: Too cold / Cold / Comfortable / Warm / Too warm
3. Button B confirms
4. Write to `/comfort.csv`
5. Return to menu

```cpp
void showComfortMenu(int selected) {
    display.clear();
    display.drawHeader("Comfort Log");

    const char* labels[] = {"Too cold", "Cold", "Comfortable", "Warm", "Too warm"};
    for (int i = 0; i < 5; i++) {
        display.setCursor(20, 60 + i * 30);
        if (i == selected) {
            display.setTextColor(ST77XX_BLACK, ST77XX_YELLOW); // highlighted
            display.print("> ");
        } else {
            display.setTextColor(ST77XX_WHITE);
            display.print("  ");
        }
        display.println(labels[i]);
    }
}
```

---

## File Changes Summary

| File | Changes |
|------|---------|
| `include/config.h` | Add SEL_BUTTON_PIN, MEASUREMENT_INTERVAL_SEC, graph constants, comfort filename |
| `include/data_structures.h` | Add altitude to SensorReading, add ComfortLevel/ComfortLog/DisplayMenu |
| `lib/hardware/sensor_manager/src/sensor_manager.cpp` | Populate altitude field in getReading() |
| `lib/hardware/storage_manager/src/storage_manager.h/.cpp` | New CSV format with altitude, add comfort log storage, fresh start on begin() |
| `lib/hardware/display_manager/src/display_manager.h/.cpp` | Add drawGraph() method, axis drawing, color-coded plots |
| `lib/services/display_service/src/display_service.h/.cpp` | Add menu navigation, comfort logging UI |
| `src/main.cpp` | Complete rewrite: measurement mode vs display mode, EXT1 wake, two-button handling |
| `test/test_hardware/includes/test_sensor.hpp` | Update tests for new altitude field |
| `test/test_hardware/includes/test_storage.hpp` | Update tests for new CSV format |

## What Does NOT Change

- WiFi manager, connectivity service, data service — untouched
- Test runner, test fixture — untouched
- PlatformIO environments — untouched
- Native/mock tests — no changes needed (mocks don't test sensor fields)
