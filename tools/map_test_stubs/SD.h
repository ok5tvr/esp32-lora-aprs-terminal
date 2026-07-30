#pragma once
#include "FS.h"
constexpr int FILE_READ = 0;
class SDClass { public: File open(const char*, int = FILE_READ) { return File(); } };
inline SDClass SD;
