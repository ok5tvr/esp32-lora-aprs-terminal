#pragma once
#include <cstddef>
#include <cstdint>
class File {
public:
    explicit operator bool() const { return valid_; }
    bool isDirectory() const { return false; }
    std::uint64_t size() const { return 131072; }
    bool seek(std::uint32_t) { return true; }
    int read(std::uint8_t*, std::size_t size) { return static_cast<int>(size); }
    void close() { valid_ = false; }
    bool valid_ = true;
};
