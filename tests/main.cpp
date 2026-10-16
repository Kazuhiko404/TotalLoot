// Standalone unit tests for TotalLoot's pure logic (no Kenshi / MyGUI headers).
// Build and run:
//   MSBuild tests/totalloot_tests.vcxproj -p:Configuration=Release -p:Platform=x64
//   tests/x64/Release/totalloot_tests.exe

#include "test_harness.h"

namespace testharness
{
int& Checks() { static int n = 0; return n; }
int& Failures() { static int n = 0; return n; }

void Check(bool ok, const char* suite, const char* what)
{
    ++Checks();
    if (!ok)
    {
        ++Failures();
        std::printf("  FAIL %s: %s\n", suite, what);
    }
}
} // namespace testharness

void RunMenuSlotTests();
void RunPickpocketRulesTests();
void RunClickGestureTests();
void RunLootConfigTests();
void RunLootWalkRulesTests();
void RunOrderWatchSetTests();
void RunLootDispatchRulesTests();
void RunAutoLootRulesTests();

int main()
{
    typedef void (*Suite)();
    const Suite suites[] =
    {
        RunMenuSlotTests,
        RunPickpocketRulesTests,
        RunClickGestureTests,
        RunLootConfigTests,
        RunLootWalkRulesTests,
        RunOrderWatchSetTests,
        RunLootDispatchRulesTests,
        RunAutoLootRulesTests,
    };
    const int count = sizeof(suites) / sizeof(suites[0]);
    int failedSuites = 0;
    for (int i = 0; i < count; ++i)
    {
        const int before = testharness::Failures();
        suites[i]();
        if (testharness::Failures() != before)
            ++failedSuites;
    }
    std::printf("\n%d/%d suites passed, %d/%d checks failed\n",
                count - failedSuites, count,
                testharness::Failures(), testharness::Checks());
    return failedSuites == 0 ? 0 : 1;
}
