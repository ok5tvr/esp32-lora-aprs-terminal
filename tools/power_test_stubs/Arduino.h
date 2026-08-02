#pragma once

#include <cstdint>

struct PowerTestSerial {
    template <typename... Args>
    void printf(const char*, Args...) {}
};

inline PowerTestSerial Serial;
inline std::uint32_t powerTestMillis = 0U;
inline std::uint32_t millis() { return powerTestMillis; }
