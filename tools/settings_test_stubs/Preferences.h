#pragma once
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include "Arduino.h"
class Preferences {
public:
    bool begin(const char*, bool = false) { return true; }
    void end() {}
    String getString(const char* key, const char* def = "") { return String(get(key, def)); }
    double getDouble(const char* key, double def = 0) { auto it=data().find(key); return it==data().end()?def:std::strtod(it->second.c_str(),nullptr); }
    float getFloat(const char* key, float def = 0) { auto it=data().find(key); return it==data().end()?def:std::strtof(it->second.c_str(),nullptr); }
    bool getBool(const char* key, bool def=false) { auto it=data().find(key); return it==data().end()?def:it->second=="1"; }
    std::uint8_t getUChar(const char* key, std::uint8_t def=0) { auto it=data().find(key); return it==data().end()?def:static_cast<std::uint8_t>(std::strtoul(it->second.c_str(),nullptr,10)); }
    std::uint32_t getUInt(const char* key, std::uint32_t def=0) { auto it=data().find(key); return it==data().end()?def:static_cast<std::uint32_t>(std::strtoul(it->second.c_str(),nullptr,10)); }
    std::int32_t getInt(const char* key, std::int32_t def=0) { auto it=data().find(key); return it==data().end()?def:static_cast<std::int32_t>(std::strtol(it->second.c_str(),nullptr,10)); }
    std::size_t putString(const char* key, const char* value) { data()[key]=value?value:""; return data()[key].size()+1; }
    std::size_t putDouble(const char* key, double value) { data()[key]=std::to_string(value); return sizeof(value); }
    std::size_t putFloat(const char* key, float value) { data()[key]=std::to_string(value); return sizeof(value); }
    std::size_t putBool(const char* key, bool value) { data()[key]=value?"1":"0"; return 1; }
    std::size_t putUChar(const char* key, std::uint8_t value) { data()[key]=std::to_string(value); return 1; }
    std::size_t putUInt(const char* key, std::uint32_t value) { data()[key]=std::to_string(value); return 4; }
    std::size_t putInt(const char* key, std::int32_t value) { data()[key]=std::to_string(value); return 4; }
    bool isKey(const char* key) { return data().count(key)>0; }
    bool remove(const char* key) { return data().erase(key)>0; }
private:
    static std::map<std::string,std::string>& data() { static std::map<std::string,std::string> values; return values; }
    static const char* get(const char* key, const char* def) { auto it=data().find(key); return it==data().end()?def:it->second.c_str(); }
};
