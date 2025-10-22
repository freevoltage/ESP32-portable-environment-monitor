#include "wifi_manager.h"
#include <time.h>

WiFiManager::WiFiManager() : wifiConnected(false) {
}

bool WiFiManager::connect(const char* ssid, const char* password, int timeoutSeconds) {
    Serial.println("\n=== WiFi Connection ===");
    Serial.printf("Connecting to: %s\n", ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    int maxAttempts = timeoutSeconds * 2; // Check every 500ms
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("\n✓ WiFi connected!");
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
        return true;
    } else {
        wifiConnected = false;
        Serial.println("\n✗ WiFi connection failed!");
        disconnect(); // Clean up
        return false;
    }
}

void WiFiManager::disconnect() {
    if (wifiConnected) {
        Serial.println("Disconnecting WiFi...");
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
        Serial.println("✗ Cannot sync time: WiFi not connected");
        return false;
    }
    
    Serial.println("\n=== NTP Time Sync ===");
    Serial.printf("NTP Server: %s\n", ntpServer);
    Serial.printf("GMT Offset: %ld seconds (%+.1f hours)\n", 
                  gmtOffset, gmtOffset / 3600.0);
    
    // Configure time with NTP
    configTime(gmtOffset, daylightOffset, ntpServer);
    
    // Wait for time to synchronize
    if (waitForTimeSync()) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            Serial.println("✓ Time synchronized successfully!");
            Serial.print("Current time: ");
            Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
            return true;
        }
    }
    
    Serial.println("✗ Time synchronization failed");
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
        Serial.print(".");
        delay(1000);
        retries++;
    }
    
    return false;
}
