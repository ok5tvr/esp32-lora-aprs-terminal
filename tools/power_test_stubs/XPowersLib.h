#pragma once

#include <cstdint>
#include <Wire.h>

constexpr std::uint8_t AXP2101_SLAVE_ADDRESS = 0x34;
constexpr std::uint8_t XPOWERS_AXP2101_CHG_TRI_STATE = 0;
constexpr std::uint8_t XPOWERS_AXP2101_CHG_PRE_STATE = 1;
constexpr std::uint8_t XPOWERS_AXP2101_CHG_CC_STATE = 2;
constexpr std::uint8_t XPOWERS_AXP2101_CHG_CV_STATE = 3;
constexpr std::uint8_t XPOWERS_AXP2101_CHG_DONE_STATE = 4;
constexpr std::uint8_t XPOWERS_AXP2101_CHG_STOP_STATE = 5;

inline bool powerTestBatteryConnected = true;
inline bool powerTestCharging = false;
inline bool powerTestDischarging = true;
inline bool powerTestStandby = false;
inline bool powerTestVbusConnected = false;
inline bool powerTestVbusGood = false;
inline int powerTestBatteryVoltageMv = 3900;
inline int powerTestVbusVoltageMv = 0;
inline int powerTestSystemVoltageMv = 4020;
inline float powerTestTemperatureC = 37.4F;
inline int powerTestBatteryPercent = 74;
inline std::uint8_t powerTestChargerStatus = XPOWERS_AXP2101_CHG_STOP_STATE;

class XPowersPMU {
public:
    bool begin(TwoWire&, std::uint8_t, int, int) { return true; }
    void disableTSPinMeasure() {}
    void enableBattDetection() {}
    void enableBattVoltageMeasure() {}
    void enableVbusVoltageMeasure() {}
    void enableSystemVoltageMeasure() {}
    void enableTemperatureMeasure() {}

    bool isBatteryConnect() { return powerTestBatteryConnected; }
    bool isCharging() { return powerTestCharging; }
    bool isDischarge() { return powerTestDischarging; }
    bool isStandby() { return powerTestStandby; }
    bool isVbusIn() { return powerTestVbusConnected; }
    bool isVbusGood() { return powerTestVbusGood; }
    int getBattVoltage() { return powerTestBatteryVoltageMv; }
    int getVbusVoltage() { return powerTestVbusVoltageMv; }
    int getSystemVoltage() { return powerTestSystemVoltageMv; }
    float getTemperature() { return powerTestTemperatureC; }
    int getBatteryPercent() { return powerTestBatteryPercent; }
    std::uint8_t getChargerStatus() { return powerTestChargerStatus; }
    std::uint8_t getChargerConstantCurr() { return 8; }
    std::uint8_t getChargeTargetVoltage() { return 2; }
};
