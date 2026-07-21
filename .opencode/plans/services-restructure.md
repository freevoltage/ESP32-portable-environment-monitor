# Services Restructure Plan

Move service libraries from `lib/` root into `lib/services/` to match the `lib/hardware/` pattern.

## Moves

```
lib/connectivity_service/  → lib/services/connectivity_service/
lib/data_service/          → lib/services/data_service/
lib/display_service/       → lib/services/display_service/
```

## Config Changes

### `platformio.ini`

**`[esp32_common]`** — update `lib_extra_dirs`:
```ini
lib_extra_dirs = 
    lib/hardware/
    lib/services/
```

**`[env:mock]`** — same change:
```ini
lib_extra_dirs = 
    lib/hardware/
    lib/services/
```

No other config changes needed. No include path changes needed.

## Verification

- `pio run -e main` — firmware builds
- `pio test -e mock` — 33 tests pass
