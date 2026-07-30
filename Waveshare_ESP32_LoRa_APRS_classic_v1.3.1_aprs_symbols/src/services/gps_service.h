#pragma once

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <cstdint>

namespace Services {

class GpsService {
public:
    struct ViewState {
        bool uartStarted = false;
        bool serialTrafficDetected = false;
        bool nmeaPacketDetected = false;
        bool receiverDetected = false;
        bool hasFix = false;
        bool speedValid = false;
        bool courseValid = false;
        bool locatorValid = false;
        bool utcTimeValid = false;
        bool utcDateValid = false;
        double latitude = 0.0;
        double longitude = 0.0;
        double altitudeMeters = 0.0;
        float speedKmh = 0.0F;
        float courseDegrees = 0.0F;
        std::uint32_t satellites = 0;
        float hdop = 0.0F;
        std::uint32_t fixAgeMs = 0xFFFFFFFFU;
        std::uint32_t lastByteAgeMs = 0xFFFFFFFFU;
        std::uint32_t lastSentenceAgeMs = 0xFFFFFFFFU;
        std::uint32_t lastValidPacketAgeMs = 0xFFFFFFFFU;
        std::uint32_t charsProcessed = 0;
        std::uint32_t charsPerSecond = 0;
        std::uint32_t sentencesProcessed = 0;
        std::uint32_t passedChecksums = 0;
        std::uint32_t failedChecksums = 0;
        std::uint8_t utcHour = 0;
        std::uint8_t utcMinute = 0;
        std::uint8_t utcSecond = 0;
        std::uint8_t utcDay = 0;
        std::uint8_t utcMonth = 0;
        std::uint16_t utcYear = 0;
        char lastSentenceType[7] = "--";
        char lastNmeaSentence[128] = "--";
        char locator[7] = "";
        std::uint32_t revision = 0;
    };

    bool begin();
    void update(std::uint32_t now);
    const ViewState& viewState() const;

private:
    void processDiagnosticCharacter(char value, std::uint32_t now);
    void finishDiagnosticSentence(std::uint32_t now);

    HardwareSerial serial_{2};
    TinyGPSPlus parser_;
    ViewState view_;
    std::uint32_t lastByteAt_ = 0;
    std::uint32_t lastSentenceAt_ = 0;
    std::uint32_t lastValidSentenceAt_ = 0;
    std::uint32_t lastPassedChecksums_ = 0;
    std::uint32_t sentenceCount_ = 0;
    std::uint32_t rateWindowStartedAt_ = 0;
    std::uint32_t rateWindowChars_ = 0;
    std::uint32_t lastDiagnosticsSecond_ = 0xFFFFFFFFU;
    bool byteSeen_ = false;
    bool sentenceSeen_ = false;
    bool validSentenceSeen_ = false;
    bool collectingSentenceType_ = false;
    bool sentenceInProgress_ = false;
    char sentenceTypeBuffer_[7] = {};
    std::uint8_t sentenceTypeLength_ = 0;
    char currentSentenceBuffer_[128] = {};
    std::uint8_t currentSentenceLength_ = 0;
};

}  // namespace Services
