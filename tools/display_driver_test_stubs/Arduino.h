#pragma once
#include <cstdint>
constexpr int OUTPUT=1, HIGH=1, LOW=0;
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
inline void delay(unsigned long) {}
inline bool ledcAttach(std::uint8_t,std::uint32_t,std::uint8_t){return true;}
inline bool ledcWrite(std::uint8_t,std::uint32_t){return true;}
class SerialStub { public: template<typename... A> void printf(const char*,A...){} };
[[maybe_unused]] inline SerialStub Serial;
