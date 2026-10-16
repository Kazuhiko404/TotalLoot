#pragma once

// Minimal assert harness shared by the pure-logic suites. Each suite is a
// function `void RunXxxTests()`; failures are counted, never thrown.

#include <cstdio>

namespace testharness
{
int& Checks();
int& Failures();
void Check(bool ok, const char* suite, const char* what);
} // namespace testharness

#define CHECK(cond) ::testharness::Check((cond), kSuiteName, #cond)
