#ifndef MOCK
#include "wifi_manager.h"
#include <time.h>
#include <logger.h>

WiFiManager::WiFiManager() : wifiConnected(false) {}

bool WiFiManager::connect(const char* ssid, const char* password, int timeoutSeconds, AbortCallback abort) {
    LOG_INFO("\n=== WiFi Connection ===");
    LOG_INFO("Connecting to: %s\n", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    int maxAttempts = timeoutSeconds * 2; // Check every 500ms
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        if (abort && abort()) {
            LOG_INFO("\nWiFi connection aborted by user");
            disconnect();
            return false;
        }
        LOG_INFO("%s", (attempts % 2 == 0) ? "." : "..");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        LOG_INFO("\nWiFi connected!");
        LOG_INFO("IP Address: %s\n", WiFi.localIP().toString().c_str());
        LOG_INFO("Signal: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        wifiConnected = false;
        LOG_ERROR("\nWiFi connection failed!");
        disconnect(); // Clean up
        return false;
    }
}

void WiFiManager::disconnect() {
    if (wifiConnected) {
        LOG_INFO("Disconnecting WiFi...");
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiConnected = false;
    
    // Small delay to ensure WiFi is fully off
    delay(100);
}

bool WiFiManager::isConnected() {
    return wifiConnected && (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

int WiFiManager::getRSSI() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

bool WiFiManager::syncTimeNTP(const char* ntpServer, long gmtOffset, int daylightOffset) {
    if (!isConnected()) {
        LOG_ERROR("Cannot sync time: WiFi not connected");
        return false;
    }
    
    LOG_INFO("\n=== NTP Time Sync ===");
    LOG_INFO("NTP Server: %s\n", ntpServer);
    LOG_INFO("GMT Offset: %ld seconds (%+.1f hours)\n", 
                  gmtOffset, gmtOffset / 3600.0);
    
    // Configure time with NTP
    configTime(gmtOffset, daylightOffset, ntpServer);
    
    // Wait for time to synchronize
    if (waitForTimeSync()) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            LOG_INFO("Time synchronized successfully!");
            LOG_INFO("Current time: ");
            
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &timeinfo);
            LOG_INFO("Current time: %s", timeStr);
            return true;
        }
    }
    
    LOG_INFO("Time synchronization failed");
    return false;
}

bool WiFiManager::waitForTimeSync(int maxRetries) {
    struct tm timeinfo;
    int retries = 0;
    
    while (retries < maxRetries) {
        if (getLocalTime(&timeinfo)) {
            // Check if time is realistic (after year 2020)
            if (timeinfo.tm_year > (2020 - 1900)) {
                return true;
            }
        }
        LOG_INFO(".");
        delay(1000);
        retries++;
    }
    
    return false;
}

#endif // !MOCK