#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <Arduino.h>

class TestHelper {
public:
    static void printTestHeader(const char* testName) {
        Serial.println("\n========================================");
        Serial.printf("TEST: %s\n", testName);
        Serial.println("========================================");
    }
    
    static void printPass(const char* message) {
        Serial.print("✓ PASSED");
        if (strlen(message) > 0) {
            Serial.printf(": %s", message);
        }
        Serial.println();
    }
    
    static void printFail(const char* message) {
        Serial.print("✗ FAILED");
        if (strlen(message) > 0) {
            Serial.printf(": %s", message);
        }
        Serial.println();
    }
    
    static void printInfo(const char* message) {
        Serial.printf("ℹ %s\n", message);
    }
    
    static void printSeparator() {
        Serial.println("----------------------------------------");
    }
    
    static bool assertInRange(float value, float min, float max, const char* name) {
        if (value >= min && value <= max) {
            Serial.printf("  %s: %.2f (valid range: %.2f-%.2f)\n", name, value, min, max);
            return true;
        } else {
            Serial.printf("  %s: %.2f (OUT OF RANGE: %.2f-%.2f)\n", name, value, min, max);
            return false;
        }
    }
    
    static void waitForSerial(unsigned long timeout = 2000) {
        unsigned long start = millis();
        while (!Serial && (millis() - start < timeout)) {
            delay(10);
        }
    }
};

#endif
