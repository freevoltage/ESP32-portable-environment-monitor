#include "test_runner.h"

const char MyUnityStrOk[]           = "OK";
const char MyUnitStrPass[]          = "PASS";
const char MyUnityStrFail[]         = "FAIL";
const char MyUnityStrIgnore[]       = "IGNORE";
const char MyUnityStrBreaker[]      = "---------------------";
const char MyUnityStrResultsTests[] = " Tests, ";
const char MyUnityStrResultsFailures[] = " Failures, ";
const char MyUnityStrResultsIgnored[] = " Ignored";


void MyCustomUnityEnd(){
    Serial.println();
    Serial.print(MyUnityStrBreaker); Serial.println();
    Serial.print((UNITY_INT)(Unity.NumberOfTests));
    Serial.print(MyUnityStrResultsTests);
    Serial.print((UNITY_INT)(Unity.TestFailures));
    Serial.print(MyUnityStrResultsFailures);
    Serial.print((UNITY_INT)(Unity.TestIgnores));
    Serial.print(MyUnityStrResultsIgnored);
    Serial.println();
    if (Unity.TestFailures == 0U) {
        Serial.print(MyUnityStrOk); // "OK"
    } else {
        Serial.print(MyUnityStrFail); // "FAIL"
    }
    Serial.println();
}

void MyCustomTestRunner(UnityTestFunction Func, const char* FuncName, const int FuncLineNum)
{
    // Store current test details for reporting
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (UNITY_LINE_TYPE)FuncLineNum;
    Unity.NumberOfTests++; // Increment total test count
    UNITY_CLR_DETAILS();   // Clear assertion details from previous test (important for TEST_PROTECT)
    UNITY_EXEC_TIME_START(); // Start execution timer

    // Reset flags for *this* test before running it
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;

    // Run setUp, the actual test function, and tearDown,
    // protected by Unity's TEST_PROTECT() macro for assertion handling.
    // If an assertion fails in Func(), Unity.CurrentTestFailed will be set internally.
    if (TEST_PROTECT())
    {
        setUp();
        Func(); // The test function executes here
    }
    if (TEST_PROTECT()) // This is in a separate block in Unity, likely for robustness if setUp/Func crashed
    {
        tearDown();
    }
    UNITY_EXEC_TIME_STOP(); // Stop execution timer

    // ************* CRITICAL CUSTOM LOGIC FOR REPORTING *************
    // Capture the state *NOW*, before Unity's internal flags are reset by any subsequent call.
    bool test_failed_status = Unity.CurrentTestFailed;
    bool test_ignored_status = Unity.CurrentTestIgnored;
    const char* test_output_name = Unity.CurrentTestName;
    UNITY_LINE_TYPE test_output_line_num = Unity.CurrentTestLineNumber;

    // Manually update the global Unity counters based on the captured status.
    // This ensures Unity.TestFailures and Unity.TestIgnores are correctly tallied
    // for any potential final summary if we were to print it, or if PlatformIO
    // tries to deduce it from these global values.
    if (test_ignored_status) {
        Unity.TestIgnores++;
    } else if (test_failed_status) {
        Unity.TestFailures++;
    }
    // (If passed, Unity.NumberOfTests already accounts for it)

    // Print the PlatformIO-friendly result line.
    // File path is set per test suite via UnitySetTestFile(__FILE__).
    Serial.print(Unity.TestFile ? Unity.TestFile : "unknown");
    Serial.print(test_output_line_num); // Line number where assertion failed or test defined
    Serial.print(":");
    Serial.print(test_output_name);     // Name of the test function
    Serial.print(":");

    if (test_ignored_status) {
        Serial.println("IGNORE");
    } else if (test_failed_status) {
        Serial.println("FAIL");
    } else {
        Serial.println("PASS");
    }
    Serial.flush();
    delay(10); // Small delay to ensure the serial buffer flushes completely

    // Reset Unity's internal current test flags for the *next* test.
    // This mimics the cleanup that UnityConcludeTest() would normally perform.
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
    // ************* END CRITICAL CUSTOM LOGIC *************
}