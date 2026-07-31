#pragma once
#include <cstddef>
#include <cstdint>
class TwoWire {
public:
    std::uint8_t registers[256] = {};
    std::uint32_t requestCount = 0;
    void beginTransmission(std::uint8_t address) { address_ = address; txLength_ = 0; }
    std::size_t write(std::uint8_t value) { tx_[txLength_++] = value; return 1; }
    int endTransmission(bool stop = true) {
        (void)stop;
        if (address_ != 0x51) return 1;
        if (txLength_ == 0) return 0;
        pointer_ = tx_[0];
        for (std::uint8_t index = 1; index < txLength_; ++index) registers[pointer_++] = tx_[index];
        if (txLength_ == 1) pointer_ = tx_[0];
        return 0;
    }
    std::uint8_t requestFrom(std::uint8_t address, std::uint8_t count) {
        if (address != 0x51) return 0;
        remaining_ = count;
        ++requestCount;
        return count;
    }
    int read() {
        if (remaining_ == 0) return -1;
        --remaining_;
        return registers[pointer_++];
    }
private:
    std::uint8_t address_ = 0;
    std::uint8_t tx_[16] = {};
    std::uint8_t txLength_ = 0;
    std::uint8_t pointer_ = 0;
    std::uint8_t remaining_ = 0;
};
extern TwoWire Wire;
