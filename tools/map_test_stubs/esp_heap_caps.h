#pragma once
#include <cstdlib>
constexpr int MALLOC_CAP_SPIRAM = 1;
constexpr int MALLOC_CAP_8BIT = 2;
constexpr int MALLOC_CAP_INTERNAL = 4;
inline void* heap_caps_malloc(std::size_t size, int) { return std::malloc(size); }

inline std::size_t heap_caps_get_free_size(int) { return 1024; }
