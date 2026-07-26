#pragma once
#ifdef MOCK
    #include <string>
    typedef std::string String;
#else
    #include <Arduino.h>
    #include <WiFi.h>
#endif

class WiFiManager {
public:
    typedef bool (*AbortCallback)();

    WiFiManager();
    
    // Connect to WiFi (abort callback checked every 500ms)
    bool connect(const char* ssid, const char* password, int timeoutSeconds = 20, AbortCallback abort = nullptr);
    
    // Disconnect and turn off WiFi
    void disconnect();
    
    // Check connection status
    bool isConnected();
    
    // Get IP address
    String getIPAddress();
    
    // Get signal strength
    int getRSSI();
    
    // Sync time from NTP server
    bool syncTimeNTP(const char* ntpServer = "pool.ntp.org", 
                     long gmtOffset = 3600, 
                     int daylightOffset = 3600);

private:
    bool wifiConnected;
    
    // Wait for NTP time sync
    bool waitForTimeSync(int maxRetries = 10);
};
