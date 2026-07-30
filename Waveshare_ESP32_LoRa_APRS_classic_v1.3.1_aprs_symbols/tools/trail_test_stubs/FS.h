#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace FakeFs {
struct Entry { std::string path; std::string data; bool directory = false; };
int find(const char* path);
}

class File {
public:
    File() = default;
    explicit File(int index) : index_(index), valid_(index >= 0) {}
    explicit operator bool() const { return valid_; }
    bool operator!() const { return !valid_; }
    bool isDirectory() const;
    const char* name() const;
    std::size_t size() const;
    File openNextFile();
    std::size_t print(const char* text);
    std::size_t write(const std::uint8_t* data, std::size_t length);
    void flush() {}
    void close() { valid_ = false; }
private:
    int index_ = -1;
    bool valid_ = false;
    std::size_t iterator_ = 0;
};
