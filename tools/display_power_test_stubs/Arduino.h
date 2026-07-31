#pragma once
#include <cstdint>
class SerialStub {
public:
    template <typename... Args>
    void printf(const char*, Args...) {}
};
[[maybe_unused]] inline SerialStub Serial;
