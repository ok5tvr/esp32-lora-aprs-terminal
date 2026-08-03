#pragma once
#include <cstddef>
#include <cstdint>
constexpr int U_FLASH=0;
class UpdateClass {
public:
    bool begin(std::size_t, int) { running_=true; return true; }
    std::size_t write(const std::uint8_t*, std::size_t n) { return n; }
    bool end(bool) { running_=false; return true; }
    void abort() { running_=false; }
    bool isRunning() const { return running_; }
    const char* errorString() const { return "error"; }
private: bool running_=false;
};
inline UpdateClass Update;
