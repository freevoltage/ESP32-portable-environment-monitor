// test_fixture.h
#pragma once

#include <functional>

/**
 * Global test context manager
 * Allows each test namespace to register its own setUp/tearDown
 */
class TestContext {
public:
    using SetupFunc = std::function<void()>;
    using TeardownFunc = std::function<void()>;
    
    static TestContext& instance() {
        static TestContext ctx;
        return ctx;
    }
    
    // Register current test suite's fixtures
    void setFixtures(SetupFunc setup, TeardownFunc teardown) {
        _currentSetup = setup;
        _currentTeardown = teardown;
    }
    
    // Clear fixtures (between test suites)
    void clearFixtures() {
        _currentSetup = nullptr;
        _currentTeardown = nullptr;
    }
    
    // Called by Unity's global setUp/tearDown
    void runSetUp() {
        if (_currentSetup) {
            _currentSetup();
        }
    }
    
    void runTearDown() {
        if (_currentTeardown) {
            _currentTeardown();
        }
    }
    
private:
    TestContext() = default;
    SetupFunc _currentSetup = nullptr;
    TeardownFunc _currentTeardown = nullptr;
};

// Helper macros for cleaner syntax
#define TEST_CONTEXT TestContext::instance()
