#include "storage_manager.h"

static std::vector<SensorReading> mockStorage;
static bool mockInitialized = false;
static bool mockShouldFail = false;
static size_t mockTotalSpace = 1024 * 1024;  // 1MB mock storage
static size_t mockUsedSpace = 0;

StorageManager::StorageManager() : _initialized(false), _filename("/Datalog.csv"){}

bool StorageManager::begin(){
    if(mockShouldFail){
        _initialized = false;
        return false;
    }
    _initialized = true;
    mockInitialized = true;
    return true;
}

bool StorageManager::isReady(){
    return _initialized && mockInitialized;
}

bool StorageManager::storeReading(const SensorReading& reading){
    if(!isReady()) return false;

    mockStorage.push_back(reading);
    mockUsedSpace += sizeof(reading);
    return true;
}

