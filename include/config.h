#pragma once

// SD Card
#define SD_CS     0

// Display pins
#define TFT_CS  5
#define TFT_RST 6
#define TFT_DC  7
#define TFT_LIT 2


// BME280
//#define BME280_ADDRESS 0x77
#define SEALEVELPRESSURE_HPA (1013.25)

// Deep Sleep Configuration
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  5  // Sleep duration in seconds

// WiFi Configuration
#define WIFI_SSID "TP-Link_0B73"
#define WIFI_PASSWORD "63392418"

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600      // GMT+1 (3600 seconds = 1 hour)
#define DAYLIGHT_OFFSET_SEC 3600 // Daylight saving time offset

// Time sync interval (sync once per day)
#define TIME_SYNC_INTERVAL_HOURS 24
