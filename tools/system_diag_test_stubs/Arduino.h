#pragma once
#include <cstdint>
class SerialClass { public: template<class... A> void printf(const char*, A...) {} };
inline SerialClass Serial;
inline bool psramFound() { return true; }
