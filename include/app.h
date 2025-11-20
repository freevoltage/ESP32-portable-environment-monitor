#pragma once
#include "../services/data_service.h"
#include "display_service.h"
#include "connectivity_serivce.h"
#include "data_structures.h"

class App {
private:
    // Services
    DataService* dataService;
    DisplayService* displayService;
    ConnectivityService* connectivityService;
    
    // State
    SystemMode currentMode = SystemMode::NORMAL_OPERATION;
    bool isFirstBoot = true;
    unsigned long bootTime;
    
    // Configuration
    const int READINGS_TO_SHOW = 3;
    const int DISPLAY_TIME_MS = 2000;
    
public:
    App(DataService* dataServ, DisplayService* displayServ, ConnectivityService* connServ);
    
    // Main Application Flow
    bool initialize();
    void run();
    
    // Application Phases
    bool startup();
    bool executeMainSequence();
    bool shutdown();
    
private:
    // Internal Operations
    bool determineSystemMode();
    bool handleFirstBoot();
    bool handleNormalOperation();
    bool handleTimeSync();
    bool handleErrorState();
    
    // Display Sequence
    bool showCurrentData();
    bool showHistoricalAnalysis();
    bool showSystemInfo();
    
    // Utility
    SystemMode detectSystemMode();
    void logSystemState();
};
