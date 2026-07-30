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

class XPowersPMU {
public:
    bool begin(TwoWire&, std::uint8_t, int, int) { return true; }
    void disableTSPinMeasure() {}
    void enableBattDetection() {}
    void enableBattVoltageMeasure() {}
    void enableVbusVoltageMeasure() {}
    void enableSystemVoltageMeasure() {}
    void enableTemperatureMeasure() {}

    bool isBatteryConnect() { return true; }
    bool isCharging() { return false; }
    bool isDischarge() { return true; }
    bool isStandby() { return false; }
    bool isVbusIn() { return false; }
    bool isVbusGood() { return false; }
    int getBattVoltage() { return 3900; }
    int getVbusVoltage() { return 0; }
    int getSystemVoltage() { return 4020; }
    float getTemperature() { return 37.4F; }
    int getBatteryPercent() { return 74; }
    std::uint8_t getChargerStatus() { return XPOWERS_AXP2101_CHG_STOP_STATE; }
    std::uint8_t getChargerConstantCurr() { return 8; }
    std::uint8_t getChargeTargetVoltage() { return 2; }
};
