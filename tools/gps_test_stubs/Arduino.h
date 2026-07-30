#pragma once

#include <cstdint>
#include <cstdio>

constexpr int INPUT_PULLUP = 0;
constexpr int SERIAL_8N1 = 0;

inline void pinMode(int, int) {}
inline std::uint32_t millis() { return 0U; }

struct SerialStub {
    template <typename... Args>
    int printf(const char*, Args...) { return 0; }
};

inline SerialStub Serial;
