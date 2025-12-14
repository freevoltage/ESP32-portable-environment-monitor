#pragma once
#include <string>
#include <map>

class MockFile {
private:
    std::string* content;
    size_t readPos;
    bool _isOpen;

public:
    MockFile() : content(nullptr), readPos(0), _isOpen(false) {}
    MockFile(std::string* c) : content(c), readPos(0), _isOpen(true) {}
    
    operator bool() const { return _isOpen; }
    
    size_t println(const std::string& str) {
        if (!_isOpen || !content) return 0;
        *content += str + "\n";
        return str.length() + 1;
    }
    
    bool available() {
        return _isOpen && content && readPos < content->length();
    }
    
    std::string readStringUntil(char delim) {
        if (!available()) return "";
        
        size_t start = readPos;
        size_t end = content->find(delim, start);
        
        if (end == std::string::npos) {
            end = content->length();
        }
        
        std::string result = content->substr(start, end - start);
        readPos = end + 1;
        return result;
    }
    
    void close() { _isOpen = false; }
};

class MockSD {
private:
    std::map<std::string, std::string> fileSystem;
    bool _initialized;

public:
    MockSD() : _initialized(false) {}
    
    bool begin(uint8_t) {
        _initialized = true;
        return true;
    }
    
    bool exists(const char* path) {
        return fileSystem.find(path) != fileSystem.end();
    }
    
    bool remove(const char* path) {
        return fileSystem.erase(path) > 0;
    }
    
    MockFile open(const char* path, const char* mode = "r") {
        std::string modeStr(mode);
        
        // Create file if it doesn't exist
        if (fileSystem.find(path) == fileSystem.end()) {
            fileSystem[path] = "";
        }
        
        // Clear on write mode
        if (modeStr == "w" || modeStr == FILE_WRITE) {
            fileSystem[path].clear();
        }
        
        return MockFile(&fileSystem[path]);
    }
    
    // Testing helper
    std::string getFileContent(const std::string& path) {
        return fileSystem[path];
    }
    
    void clearAll() {
        fileSystem.clear();
    }
};

extern MockSD SD;

#define FILE_WRITE "w"
#define FILE_APPEND "a"
