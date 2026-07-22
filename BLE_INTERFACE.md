# BLE Interface Specification

Hiking Station BLE GATT interface for phone app integration.

## Device Information

| Property | Value |
|----------|-------|
| Device Name | `HikingStation` |
| Service UUID | `4c54494d-6568-6b69-6e67-53746174696f` |
| Protocol | BLE (Bluetooth Low Energy) |
| Roles | Device = Peripheral, Phone = Central |

## GATT Services and Characteristics

### Time Sync Service

**UUID:** `4c54494d-6568-6b69-6e67-53746174696f`

| Characteristic | UUID | Properties | Description |
|----------------|------|------------|-------------|
| Time Write | `4c54494d-7469-6d65-7772-697465000001` | Write | Phone writes current Unix epoch (4 bytes) |
| Time Read | `4c54494d-7469-6d65-7772-697465000002` | Read, Notify | Phone reads device's current Unix epoch (4 bytes) |

## Data Format

All time values are **4-byte unsigned 32-bit integers** in **little-endian byte order**, representing Unix epoch seconds (seconds since 1970-01-01 00:00:00 UTC).

### Example

Current epoch: `1721566800` (2024-07-21 16:00:00 UTC)

Byte representation (little-endian):
```
[0x30, 0x5A, 0x8F, 0x66]
  LSB                MSB
```

## Connection Flow

### Time Synchronization

1. Phone scans for devices named `HikingStation`
2. Phone connects to the device
3. Phone discovers the Time Sync Service (`4c54494d-6568-6b69-6e67-53746174696f`)
4. Phone writes current Unix epoch (4 bytes, little-endian) to the Time Write characteristic
5. Device sets its internal RTC to the received time
6. Phone may optionally read the Time Read characteristic to verify the device accepted the time
7. Phone disconnects

### Timeout Behavior

- Device advertises for **30 seconds** (`BLE_SYNC_TIMEOUT_MS`) after BLE sync is triggered
- If no phone connects within the timeout, BLE sync fails
- Device falls back to WiFi NTP sync if configured (see Sync Modes below)
- After timeout, device stops advertising and releases BLE resources

### Reconnection

- Device restarts advertising automatically after each disconnect
- Each time sync trigger creates a fresh BLE server instance
- No bonding or pairing required — all characteristics are open access

## Sync Modes

The device supports 5 sync modes, configurable via the device menu or persisted in LittleFS:

| Mode | Value | Behavior |
|------|-------|----------|
| OFF | 0 | No time synchronization |
| BLE_ONLY | 1 | Phone BLE only |
| WIFI_ONLY | 2 | WiFi NTP only |
| BLE_FIRST | 3 | Try BLE first, fallback to WiFi NTP |
| WIFI_FIRST | 4 | Try WiFi NTP first, fallback to BLE |

### Default Mode

`BLE_FIRST` (value 3) — tries phone sync first, falls back to WiFi NTP if BLE times out.

## Advertising

When BLE sync is active, the device advertises:
- **Device Name:** `HikingStation`
- **Service UUID:** `4c54494d-6568-6b69-6e67-53746174696f` (included in advertisement)
- **Connectable:** Yes
- **Scannable:** Yes

When BLE sync is not active, the device does not advertise.

## Phone App Requirements

### Minimum Requirements
- Bluetooth Low Energy (BLE 4.0+)
- Ability to scan for BLE devices by name
- Ability to connect and write to GATT characteristics
- Access to current Unix epoch time (most OS time APIs provide this)

### Recommended Flow

```
On button press or automatic trigger:
  1. Scan for "HikingStation"
  2. Connect (timeout: 5s)
  3. Discover services
  4. Write current epoch (4 bytes LE) to Time Write characteristic
  5. Read Time Read characteristic to verify
  6. Disconnect
  7. Report success/failure to user
```

### Error Handling

| Error | Cause | Recovery |
|-------|-------|----------|
| Device not found | BLE off, device not in sync mode, or out of range | Prompt user to wake device and select "Sync Time" |
| Write failed | Connection lost during write | Retry up to 3 times |
| Verification failed | Device rejected time value | Check epoch value is valid (post-2000, pre-2100) |

## Future Extensions (Planned)

These characteristics are not yet implemented but are planned for future firmware versions:

| Characteristic | UUID | Properties | Description |
|----------------|------|------------|-------------|
| Battery Status | `4c54494d-62617474-0000000000001` | Read, Notify | Battery percentage (1 byte, 0-100) + voltage (2 bytes, mV) |
| Sensor Data | `4c54494d-73656e73-000000000001` | Read, Notify | Current temp/humidity/pressure (12 bytes, 3x float LE) |
| Device Info | `4c54494d-696e666f-000000000001` | Read | Firmware version, uptime, free memory |

## Source Code Reference

| File | Description |
|------|-------------|
| `lib/services/time_sync_service/src/time_sync_service.h` | TimeSyncService class definition |
| `lib/services/time_sync_service/src/time_sync_service.cpp` | BLE GATT setup, callbacks, sync logic |
| `include/config.h` | BLE_DEVICE_NAME, BLE_SYNC_TIMEOUT_MS |
| `include/data_structures.h` | SyncMode, SyncSource, SyncStatus structs |
