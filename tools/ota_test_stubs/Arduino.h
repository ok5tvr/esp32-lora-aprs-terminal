#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#define F(x) x
class String {
public:
    String() = default;
    String(const char* s): value_(s ? s : "") {}
    String(const std::string& s): value_(s) {}
    String(unsigned long v): value_(std::to_string(v)) {}
    String(unsigned int v): value_(std::to_string(v)) {}
    String(int v): value_(std::to_string(v)) {}
    void reserve(std::size_t n) { value_.reserve(n); }
    const char* c_str() const { return value_.c_str(); }
    std::size_t length() const { return value_.length(); }
    String& operator+=(const char* s) { value_ += s ? s : ""; return *this; }
    String& operator+=(char c) { value_ += c; return *this; }
    String& operator+=(const String& s) { value_ += s.value_; return *this; }
private:
    std::string value_;
};
class IPAddress {
public:
    IPAddress(int=0,int=0,int=0,int=0) {}
    String toString() const { return String("192.168.4.1"); }
};
class SerialClass { public: template<class... A> void printf(const char*, A...) {} };
inline SerialClass Serial;
inline std::uint32_t millis() { return 1000; }
inline void delay(std::uint32_t) {}
class ESPClass {
public:
    std::uint32_t getFreeSketchSpace() const { return 0x700000; }
    void restart() {}
};
inline ESPClass ESP;
