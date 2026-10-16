#include "test_harness.h"
#include "click_gesture.h"

using namespace totalloot::clickgesture;

namespace
{
const char* const kSuiteName = "click_gesture";

void OrdinaryClickActsOnceThenEchoes()
{
    State s;
    Arm(&s);
    CHECK(Classify(&s) == ACT);     // press
    CHECK(Classify(&s) == ECHO);    // release of the same click
    CHECK(Classify(&s) == STRAY);   // anything after that
}

void ReleaseOnlyGestureActs()
{
    // Right button held to open the menu, released over the button: the press
    // happened outside, so only the release arrives.
    State s;
    Arm(&s);
    CHECK(Classify(&s) == ACT);
}

void NothingArmedNeverActs()
{
    // The click that opened the menu, or a stale event after the menu closed.
    State s;
    CHECK(Classify(&s) == STRAY);
    CHECK(Classify(&s) == STRAY);
}

void RearmingStartsAFreshGesture()
{
    State s;
    Arm(&s);
    CHECK(Classify(&s) == ACT);     // release-only gesture, no echo follows
    Arm(&s);                        // next menu: stale echo must not survive
    CHECK(Classify(&s) == ACT);
    CHECK(Classify(&s) == ECHO);
}

void ArmingTwiceDoesNotActTwice()
{
    // The menu hook can re-arm every frame it is open and unspent.
    State s;
    Arm(&s);
    Arm(&s);
    CHECK(Classify(&s) == ACT);
    CHECK(Classify(&s) == ECHO);
    CHECK(Classify(&s) == STRAY);
}
} // namespace

void RunClickGestureTests()
{
    OrdinaryClickActsOnceThenEchoes();
    ReleaseOnlyGestureActs();
    NothingArmedNeverActs();
    RearmingStartsAFreshGesture();
    ArmingTwiceDoesNotActTwice();
}
