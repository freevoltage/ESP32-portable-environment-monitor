#pragma once

// Debug mode: uncomment to enable debug output (SD card file listing, etc.)
// #define DEBUG

// SD Card
#define SD_CS     0

// Display pins
#define TFT_CS  5
#define TFT_RST 6
#define TFT_DC  7
#define TFT_LIT 2  // Backlight pin

// Backlight polarity: 1 = active-HIGH (HIGH = ON, LOW = OFF), 0 = inverted (LOW = ON)
#define TFT_BACKLIGHT_INVERTED 1

// Display rotation: 0=portrait, 1=landscape, 2=portrait flipped, 3=landscape flipped
#define TFT_ROTATION 2


// BME280
//#define BME280_ADDRESS 0x77
#define SEALEVELPRESSURE_HPA (1013.25)

// Deep Sleep Configuration
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  5  // Sleep duration in seconds

// Buttons
#define NAV_BUTTON_PIN 8   // GPIO8 = dedicated SMD button (Navigate) — NOT an RTC GPIO, so no EXT1 wake
#define SEL_BUTTON_PIN 3   // GPIO3 = Select button

// Measurement Interval (for timer wake)
#define MEASUREMENT_INTERVAL_SEC 1800  // 30 minutes

// Graph Layout Constants
#define GRAPH_PADDING    10
#define GRAPH_WIDTH      (240 - 2 * GRAPH_PADDING)
#define GRAPH_HEIGHT     (240 - 2 * GRAPH_PADDING)

// Data Log
#define DATALOG_FILENAME "/datalog.csv"
#define COMFORT_FILENAME "/comfort.csv"
#define DEFAULT_MAX_SIZE 1000

// Hold GPIO output states during deep sleep (prevents backlight leakage on GPIO2)
// Hardware alternative: pull-down resistor from TFT_LIT to GND on the display board
#define HOLD_GPIO_IN_SLEEP 1

// WiFi Configuration (compile-time defaults — overridden by LittleFS config if present)
#define WIFI_DEFAULT_SSID "TP-Link_0B73"
#define WIFI_DEFAULT_PASSWORD "63392418"
#define WIFI_CONFIG_FILE "/wifi_config.txt"

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600      // GMT+1 (3600 seconds = 1 hour)
#define DAYLIGHT_OFFSET_SEC 3600 // Daylight saving time offset

// Time sync interval (sync once per day)
#define TIME_SYNC_INTERVAL_HOURS 24

// OTA Configuration
#define OTA_SERVER_PORT 80
#define OTA_USERNAME "admin"
#define OTA_PASSWORD "hikingstation"

// Battery Management (MAX17048 fuel gauge on I2C)
#define NEOPIXEL_I2C_POWER 20   // GPIO20 controls I2C power rail — cut before deep sleep to save ~55uA

// BLE Time Sync Configuration
#define BLE_DEVICE_NAME          "HikingStation"
#define BLE_SYNC_TIMEOUT_MS      30000   // 30s to wait for phone BLE time write
#define TIME_SYNC_CONFIG_FILE    "/sync_config.txt"  // Persisted sync mode on LittleFS