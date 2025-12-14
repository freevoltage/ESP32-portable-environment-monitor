#pragma once

/* 
#ifdef MOCK
    #include <cstdio>
    #define LOG_INFO(fmt, ...) printf("INFO: " fmt "\n", ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...) printf("WARN: " fmt "\n", ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
    #include <Arduino.h>
    #define LOG_INFO(fmt, ...) Serial.printf("INFO: " fmt "\n", ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) Serial.printf("ERROR: " fmt "\n", ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...) Serial.printf("WARN: " fmt "\n", ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) Serial.printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#endif
 */

#ifdef MOCK
    #include <cstdio>
    #include <cstdarg>
    
    inline void log_info(const char* func, const char* fmt, ...) {
        printf("INFO [%s]: ", func);
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }
    
    inline void log_error(const char* func, const char* fmt, ...) {
        fprintf(stderr, "ERROR [%s]: ", func);
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
    
    inline void log_warn(const char* func, const char* fmt, ...) {
        printf("WARN [%s]: ", func);
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }
    
    inline void log_debug(const char* func, const char* fmt, ...) {
        printf("DEBUG [%s]: ", func);
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }
    
#else
    #include <Arduino.h>
    
    inline void log_info(const char* func, const char* fmt, ...) {
        Serial.printf("INFO [%s]: ", func);
        va_list args;
        va_start(args, fmt);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.println(buffer);
    }
    
    inline void log_error(const char* func, const char* fmt, ...) {
        Serial.printf("ERROR [%s]: ", func);
        va_list args;
        va_start(args, fmt);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.println(buffer);
    }
    
    inline void log_warn(const char* func, const char* fmt, ...) {
        Serial.printf("WARN [%s]: ", func);
        va_list args;
        va_start(args, fmt);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.println(buffer);
    }
    
    inline void log_debug(const char* func, const char* fmt, ...) {
        Serial.printf("DEBUG [%s]: ", func);
        va_list args;
        va_start(args, fmt);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        Serial.println(buffer);
    }
    
#endif

// Macros to auto-inject function name
#define LOG_INFO(...) log_info(__func__, __VA_ARGS__)
#define LOG_ERROR(...) log_error(__func__, __VA_ARGS__)
#define LOG_WARN(...) log_warn(__func__, __VA_ARGS__)
#define LOG_DEBUG(...) log_debug(__func__, __VA_ARGS__)
