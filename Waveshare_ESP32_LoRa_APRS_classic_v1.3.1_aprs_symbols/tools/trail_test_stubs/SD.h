#pragma once
#include <string>
#include "FS.h"
constexpr int FILE_WRITE = 1;
class SDClass {
public:
    bool exists(const char* path);
    bool mkdir(const char* path);
    File open(const char* path, int mode = 0);
    const std::string& content(const char* path) const;
};
extern SDClass SD;
