#pragma once

#include <cstddef>
#include <cstdint>

class WiFiClient {
public:
    bool connected() const { return connected_; }
    void stop() { connected_ = false; }
    bool connect(const char*, std::uint16_t, std::uint32_t) { connected_ = false; return false; }
    void setNoDelay(bool) {}
    int available() const { return 0; }
    int read() { return -1; }
    std::size_t write(const std::uint8_t*, std::size_t length) { return connected_ ? length : 0U; }
private:
    bool connected_ = false;
};
