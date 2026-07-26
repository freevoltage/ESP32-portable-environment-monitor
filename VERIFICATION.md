# Verification Checklist

All changes pushed to `dev` branch. Check each item off as you verify.

---

## 1. Wiki Build

```sh
cd docs && bun run dev
```

Open browser to `http://localhost:4321/ESP32-portable-environment-monitor/`

Verify these pages load and show **no stale content**:

| Page | What to check |
|------|---------------|
| **Hardware Setup** (`getting-started/hardware-setup`) | GPIO8 = Navigate, GPIO9 = NOT listed as NAV |
| **First Run** (`getting-started/first-run`) | Menu has 7 items: Graph Temp/Humidity/Altitude, Settings, OTA, Sync Time, Sleep. **No** "Log Comfort" in menu table. Settings sub-menu documented. `/settings.txt` and `/debug.log` in config/SD tables. |
| **System Overview** (`architecture/overview`) | SettingsManager in architecture diagram. Directory structure includes `settings_manager/`. Firmware modes mention configurable interval. |
| **Hardware Layer** (`architecture/hardware-layer`) | Battery Manager section. Settings Manager section. WiFi `connect()` shows abort callback param. |
| **Services Layer** (`architecture/services-layer`) | Menu system shows 7 items + dashboard + sub-menus. ConnectivityService `connect()` shows abort. Interaction diagram includes TimeSyncService. |
| **config.h Reference** (`configuration/config-h`) | Timing shows `60` dev default with "overridden by SettingsManager" note. OTA section (timeout, auth). Debug section (`DEBUG_LOG_FILENAME`). |
| **Deep Sleep** (`configuration/deep-sleep`) | Timer wake says "configurable via Settings", not hardcoded 1800. |
| **Testing** (`development/testing`) | 68 mock tests, not 33. |
| **DisplayManager Reference** (`reference/display-manager`) | `showDashboard()`, `showSyncSubMenu()`, `showSettingsSubMenu()`, `showOTAMode()`, `showSyncProgress()`, `showBatteryInfo()` — all listed. |
| **StorageManager Reference** (`reference/storage-manager`) | `logDebug()` listed. `/debug.log` in CSV formats section. |
| **BLE Companion App** (`guides/ble-companion-app`) | New page loads. GATT UUIDs correct. Swift code examples present. |

---

## 2. Git Status

```sh
git log --oneline -5
```

Should see:

```
2f10840 docs: update wiki for current firmware + add BLE companion app guide
4237a2d docs: fix wiki build
3f48944 docs: update wiki user-interface with connectivity icon and settings sub-menu
3919c6a feat: WiFi abort + connectivity icon + settings sub-menu
d5bd5b6 feat: quick wins — menu simplification, battery bar, WiFi dots, OTA msg
```

```sh
git status
```

Should say `nothing to commit, working tree clean`.

---

## 3. Firmware Build

```sh
pio run -e main
```

Should build without errors. Check flash/RAM usage at the end.

---

## 4. Mock Tests

```sh
pio test -e mock
```

**68 tests should pass.** Check for `OK` and `FAIL` counts.

---

## 5. Hardware Tests (on device)

```sh
pio test -e main
```

**34 tests should pass** on connected ESP32-C6.

---

## 6. Device Features (manual, on hardware)

### 6a. Menu Structure

- Press button B to wake → should show **Dashboard** (not menu directly)
- Dashboard shows: header (sensor data + time + "WiFi" when connected), battery bar with "Last:WiFi" or "Last:BLE", footer
- Press B on "Menu" → should show **7 items**: Graph Temp, Graph Humidity, Graph Altitude, Settings, OTA, Sync Time, Sleep
- Button A cycles through all 7. No "Log Comfort" in this list.

### 6b. Settings Sub-Menu

- Navigate to **Settings** in menu → press B
- Should show 3 items: "Sleep Interval", "NTP Sync", "Back"
- Press A on "Sleep Interval" → value cycles: 1m → 5m → 15m → 30m → 1hr
- Press A on "NTP Sync" → value cycles: 1hr → 6hr → 12hr → 24hr
- Press B on "Back" → returns to main menu
- Both buttons (A+B) from Settings → returns to Dashboard

### 6c. WiFi Abort (OTA)

- Set sync mode to BLE-only (Menu → Sync Time → cycle to BLE)
- Go to Menu → OTA
- Should see "WiFi required for OTA..." message
- Press B to exit → returns to menu
- WiFi never attempted (no hanging on connect)

### 6d. Connectivity Icon

- After WiFi sync: Dashboard header should show "WiFi"
- Battery bar should show "Last:WiFi" or "Last:BLE" depending on last sync source

### 6e. Both-Buttons Abort

- From any sub-menu (graphs, settings, sync, OTA), press A+B simultaneously
- Should always return to Dashboard
- Test from each screen

### 6f. Comfort Log Protection

- Log comfort from Dashboard (navigate to "Log Comfort", press B)
- Try to log again → should show "Already logged today!"
- Only one log per day allowed

### 6g. Settings Persistence

- Change Sleep Interval to 5m in Settings
- Power cycle device (unplug/replug)
- Navigate back to Settings → Sleep Interval should still show 5m

---

## 7. SD Card Files

Remove SD card, read on computer (or via serial debug):

- `/debug.log` should exist with BOOT, SENSOR, SYNC events
- `/datalog.csv` should have sensor readings
- `/settings.txt` should have your changed values

---

## 8. Serial Monitor

```sh
pio device monitor
```

Check for clean boot messages. No crashes, no `gpio_hold_dis` missing warnings.

---

## 9. GitHub Pages

After push, check: `https://freevoltage.github.io/ESP32-portable-environment-monitor/`

New "Guides" section should appear in sidebar with "BLE Companion App".

---

## Sign Off

| Item | Verified | Date |
|------|----------|------|
| Wiki build clean | | |
| Git status clean | | |
| Firmware builds | | |
| 68 mock tests pass | | |
| 34 hardware tests pass | | |
| Menu (7 items) works | | |
| Settings sub-menu works | | |
| WiFi abort works | | |
| Connectivity icon works | | |
| Both-buttons abort works | | |
| Comfort log protection works | | |
| Settings persistence works | | |
| SD card files correct | | |
| Serial monitor clean | | |
| GitHub Pages deployed | | |
