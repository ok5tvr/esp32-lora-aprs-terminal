#pragma once
#include <cstdint>
#include <string>
class String {
public:
    String() = default;
    String(const char* value) : value_(value ? value : "") {}
    const char* c_str() const { return value_.c_str(); }
private:
    std::string value_;
};
class SerialStub {
public:
    template <typename... Args>
    void printf(const char*, Args...) {}
};
[[maybe_unused]] inline SerialStub Serial;
