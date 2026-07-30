#pragma once

#include <cstdint>

struct TinyGpsLocation {
    bool isValid() const { return false; }
    std::uint32_t age() const { return 0xFFFFFFFFU; }
    double lat() const { return 0.0; }
    double lng() const { return 0.0; }
};

struct TinyGpsAltitude {
    bool isValid() const { return false; }
    std::uint32_t age() const { return 0xFFFFFFFFU; }
    double meters() const { return 0.0; }
};

struct TinyGpsSpeed {
    bool isValid() const { return false; }
    std::uint32_t age() const { return 0xFFFFFFFFU; }
    double kmph() const { return 0.0; }
};

struct TinyGpsCourse {
    bool isValid() const { return false; }
    std::uint32_t age() const { return 0xFFFFFFFFU; }
    double deg() const { return 0.0; }
};

struct TinyGpsSatellites {
    bool isValid() const { return false; }
    std::uint32_t value() const { return 0U; }
};

struct TinyGpsHdop {
    bool isValid() const { return false; }
    double hdop() const { return 0.0; }
};

struct TinyGpsTime {
    bool isValid() const { return false; }
    std::uint32_t age() const { return 0xFFFFFFFFU; }
    std::uint8_t hour() const { return 0U; }
    std::uint8_t minute() const { return 0U; }
    std::uint8_t second() const { return 0U; }
};

struct TinyGpsDate {
    bool isValid() const { return false; }
    std::uint32_t age() const { return 0xFFFFFFFFU; }
    std::uint8_t day() const { return 0U; }
    std::uint8_t month() const { return 0U; }
    std::uint16_t year() const { return 0U; }
};

class TinyGPSPlus {
public:
    bool encode(char) { ++chars_; return false; }
    std::uint32_t passedChecksum() const { return 0U; }
    std::uint32_t failedChecksum() const { return 0U; }
    std::uint32_t charsProcessed() const { return chars_; }

    TinyGpsLocation location;
    TinyGpsAltitude altitude;
    TinyGpsSpeed speed;
    TinyGpsCourse course;
    TinyGpsSatellites satellites;
    TinyGpsHdop hdop;
    TinyGpsTime time;
    TinyGpsDate date;

private:
    std::uint32_t chars_ = 0U;
};
