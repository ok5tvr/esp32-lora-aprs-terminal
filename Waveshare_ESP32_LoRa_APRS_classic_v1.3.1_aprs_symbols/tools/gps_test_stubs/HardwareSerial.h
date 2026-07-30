#pragma once

#include <cstdint>

class HardwareSerial {
public:
    explicit HardwareSerial(int) {}
    void begin(std::uint32_t, int, int, int) {}
    int available() const { return 0; }
    int read() { return -1; }
};
