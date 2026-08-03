#pragma once
#include "Arduino.h"
#include <cstdint>
enum wifi_mode_t { WIFI_OFF=0, WIFI_STA=1, WIFI_AP=2, WIFI_AP_STA=3 };
class WiFiClass {
public:
    void persistent(bool) {}
    bool mode(wifi_mode_t m) { mode_=m; return true; }
    wifi_mode_t getMode() const { return mode_; }
    bool softAPdisconnect(bool) { return true; }
    bool softAPConfig(const IPAddress&, const IPAddress&, const IPAddress&) { return true; }
    bool softAP(const char*, const char*, std::uint8_t, bool, std::uint8_t) { return true; }
    IPAddress softAPIP() const { return IPAddress(); }
    std::uint8_t softAPgetStationNum() const { return 0; }
private: wifi_mode_t mode_=WIFI_OFF;
};
inline WiFiClass WiFi;
