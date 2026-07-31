#pragma once
#include <cstdint>
class TCA9554 {
public:
    explicit TCA9554(std::uint8_t) {}
    void begin(){}
    void pinMode1(std::uint8_t,int){}
    void write1(std::uint8_t,int){}
};
