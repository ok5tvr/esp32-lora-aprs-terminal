#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
constexpr int HIGH = 1;
constexpr int LOW = 0;
constexpr int OUTPUT = 1;
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline std::uint32_t millis() { return 0; }
struct StubSerial { void begin(std::uint32_t) {} void println() {} template <typename... Args> void printf(const char*, Args...) {} };
inline StubSerial Serial;

struct ESPClass { std::uint32_t getFlashChipSize() const { return 16U*1024U*1024U; } std::uint32_t getPsramSize() const { return 4U*1024U*1024U; } };
inline ESPClass ESP;
inline bool psramFound(){ return true; }
inline void delay(std::uint32_t) {}
