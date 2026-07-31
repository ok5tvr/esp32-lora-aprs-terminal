#pragma once

#include <cstdint>
#include "Arduino.h"

constexpr int WL_DISCONNECTED = 0;
constexpr int WL_CONNECTED = 3;
constexpr int WIFI_OFF = 0;
constexpr int WIFI_STA = 1;

class IPAddress {
public:
    String toString() const { return String("0.0.0.0"); }
};

class WiFiClass {
public:
    void persistent(bool) {}
    int status() const { return WL_DISCONNECTED; }
    void mode(int) {}
    void setAutoReconnect(bool) {}
    void disconnect(bool = false, bool = false) {}
    void begin(const char*, const char*) {}
    std::int32_t RSSI() const { return -100; }
    IPAddress localIP() const { return IPAddress{}; }
};

inline WiFiClass WiFi;
