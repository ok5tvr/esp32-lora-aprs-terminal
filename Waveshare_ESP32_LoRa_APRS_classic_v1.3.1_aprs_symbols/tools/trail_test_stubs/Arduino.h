#pragma once
#include <cstddef>
#include <cstdint>
struct SerialStub { template <typename... T> int printf(const char*, T...) { return 0; } };
extern SerialStub Serial;
unsigned long millis();
