#ifndef MOCK
#include "settings_manager.h"
#include <logger.h>

static const char* SETTINGS_FILE = "/settings.txt";
static const uint16_t INTERVAL_OPTIONS[] = {60, 300, 900, 1800, 3600};
static const uint16_t HOURS_OPTIONS[] = {1, 6, 12, 24};
static const int INTERVAL_COUNT = 5;
static const int HOURS_COUNT = 4;

SettingsManager::SettingsManager() : _settings(), _initialized(false) {}

bool SettingsManager::begin() {
    if (!LittleFS.begin(true)) {
        LOG_ERROR("SettingsManager: LittleFS mount failed");
        return false;
    }
    _initialized = true;
    load();
    return true;
}

const DeviceSettings& SettingsManager::getSettings() const {
    return _settings;
}

void SettingsManager::cycleMeasurementInterval() {
    uint16_t current = _settings.measurementIntervalSec;
    for (int i = 0; i < INTERVAL_COUNT; i++) {
        if (INTERVAL_OPTIONS[i] == current) {
            _settings.measurementIntervalSec = INTERVAL_OPTIONS[(i + 1) % INTERVAL_COUNT];
            save();
            return;
        }
    }
    _settings.measurementIntervalSec = INTERVAL_OPTIONS[0];
    save();
}

void SettingsManager::cycleNTPSyncInterval() {
    uint16_t current = _settings.ntpSyncIntervalHours;
    for (int i = 0; i < HOURS_COUNT; i++) {
        if (HOURS_OPTIONS[i] == current) {
            _settings.ntpSyncIntervalHours = HOURS_OPTIONS[(i + 1) % HOURS_COUNT];
            save();
            return;
        }
    }
    _settings.ntpSyncIntervalHours = HOURS_OPTIONS[0];
    save();
}

bool SettingsManager::save() {
    if (!_initialized) return false;

    File f = LittleFS.open(SETTINGS_FILE, "w");
    if (!f) {
        LOG_ERROR("SettingsManager: Failed to open settings file for writing");
        return false;
    }

    f.printf("measurement_interval=%u\n", _settings.measurementIntervalSec);
    f.printf("ntp_sync_interval=%u\n", _settings.ntpSyncIntervalHours);
    f.close();

    LOG_INFO("Settings saved: interval=%us, ntp=%uh",
             _settings.measurementIntervalSec, _settings.ntpSyncIntervalHours);
    return true;
}

void SettingsManager::apply() {
    LOG_INFO("Settings applied: interval=%us, ntp=%uh",
             _settings.measurementIntervalSec, _settings.ntpSyncIntervalHours);
}

bool SettingsManager::load() {
    if (!_initialized) return false;

    File f = LittleFS.open(SETTINGS_FILE, "r");
    if (!f) {
        LOG_INFO("SettingsManager: No settings file, using defaults");
        return false;
    }

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        int eq = line.indexOf('=');
        if (eq < 0) continue;

        String key = line.substring(0, eq);
        String val = line.substring(eq + 1);

        if (key == "measurement_interval") {
            _settings.measurementIntervalSec = val.toInt();
        } else if (key == "ntp_sync_interval") {
            _settings.ntpSyncIntervalHours = val.toInt();
        }
    }
    f.close();

    // Validate ranges
    if (_settings.measurementIntervalSec < 60) _settings.measurementIntervalSec = 60;
    if (_settings.measurementIntervalSec > 3600) _settings.measurementIntervalSec = 3600;
    if (_settings.ntpSyncIntervalHours < 1) _settings.ntpSyncIntervalHours = 1;
    if (_settings.ntpSyncIntervalHours > 24) _settings.ntpSyncIntervalHours = 24;

    LOG_INFO("Settings loaded: interval=%us, ntp=%uh",
             _settings.measurementIntervalSec, _settings.ntpSyncIntervalHours);
    return true;
}

#endif
