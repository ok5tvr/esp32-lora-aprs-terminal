#pragma once
#include <cstdint>
constexpr int INPUT_PULLUP=1, LOW=0;
inline void pinMode(int,int) {}
inline int digitalRead(int) { return 1; }
inline std::uint32_t millis() { return 0; }
class SerialStub { public: template<typename... A> void printf(const char*,A...){} };
[[maybe_unused]] inline SerialStub Serial;
