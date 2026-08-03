#pragma once
#include <cstddef>
#include <cstdint>
constexpr std::uint32_t MALLOC_CAP_INTERNAL=1;
constexpr std::uint32_t MALLOC_CAP_SPIRAM=2;
inline std::size_t heap_caps_get_free_size(std::uint32_t) { return 100000; }
inline std::size_t heap_caps_get_largest_free_block(std::uint32_t) { return 50000; }
inline std::size_t heap_caps_get_minimum_free_size(std::uint32_t) { return 90000; }
