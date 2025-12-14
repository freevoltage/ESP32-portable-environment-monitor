#pragma once
#include <Arduino.h>
#include <unity.h>

extern struct UNITY_STORAGE_T Unity; 

extern const char MyUnityStrOk[];
extern const char MyUnitStrPass[];
extern const char MyUnityStrFail[];
extern const char MyUnityStrIgnore[];
extern const char MyUnityStrBreaker[];
extern const char MyUnityStrResultsTests[];
extern const char MyUnityStrResultsFailures[];
extern const char MyUnityStrResultsIgnored[];

void MyCustomTestRunner(UnityTestFunction Func, const char* FuncName, const int FuncLineNum);
void MyCustomUnityEnd();

#pragma endregion

#undef RUN_TEST
#define RUN_TEST(func) MyCustomTestRunner(func, #func, __LINE__)

#undef UNITY_END
#define UNITY_END() MyCustomUnityEnd()