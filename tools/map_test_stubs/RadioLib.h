#pragma once
#include <cstdint>
constexpr int RADIOLIB_ERR_NONE = 0;
class SPIClass;
class SPISettings;
class Module { public: template <typename... Args> Module(Args...) {} };
class SX1278 { public: template <typename... Args> SX1278(Args...) {} };
