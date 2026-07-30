#pragma once

#include <cstdint>

struct PowerTestSerial {
    template <typename... Args>
    void printf(const char*, Args...) {}
};

inline PowerTestSerial Serial;
