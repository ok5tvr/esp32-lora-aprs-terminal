#include "services/gps_service.h"

#include <Arduino.h>
#include <cmath>
#include <cstdio>

#include "app_config.h"
#include "app_log.h"
#include "board_pins.h"
#include "services/geo_utils.h"

namespace Services {
namespace {

std::uint32_t ageOrUnknown(bool seen, std::uint32_t now, std::uint32_t timestamp) {
    return seen ? now - timestamp : 0xFFFFFFFFU;
}

}  // namespace

bool GpsService::begin() {
    view_ = ViewState{};
    if (!AppConfig::ENABLE_GPS) {
        return false;
    }

    pinMode(BoardPins::GPS_RX, INPUT_PULLUP);
    serial_.begin(
        AppConfig::GPS_BAUD_RATE,
        SERIAL_8N1,
        BoardPins::GPS_RX,
        BoardPins::GPS_TX);
    view_.uartStarted = true;
    lastPassedChecksums_ = parser_.passedChecksum();
    rateWindowStartedAt_ = millis();
    rateWindowChars_ = parser_.charsProcessed();
    LOG_I(
        "GPS",
        "UART2 listening at %u baud on GPIO%d",
        static_cast<unsigned>(AppConfig::GPS_BAUD_RATE),
        BoardPins::GPS_RX);
    return true;
}

void GpsService::finishDiagnosticSentence(std::uint32_t now) {
    if (!sentenceInProgress_) {
        return;
    }

    // A valid diagnostic line must contain more than the start delimiter.
    // This also prevents a lone '$' caused by UART noise from replacing the
    // previously completed sentence.
    if (currentSentenceLength_ > 1U) {
        sentenceSeen_ = true;
        lastSentenceAt_ = now;
        ++sentenceCount_;

        if (sentenceTypeLength_ > 0U) {
            std::snprintf(
                view_.lastSentenceType,
                sizeof(view_.lastSentenceType),
                "%s",
                sentenceTypeBuffer_);
        }

        std::snprintf(
            view_.lastNmeaSentence,
            sizeof(view_.lastNmeaSentence),
            "%s",
            currentSentenceBuffer_);
    }

    collectingSentenceType_ = false;
    sentenceInProgress_ = false;
    sentenceTypeLength_ = 0;
    sentenceTypeBuffer_[0] = '\0';
    currentSentenceLength_ = 0;
    currentSentenceBuffer_[0] = '\0';
}

void GpsService::processDiagnosticCharacter(char value, std::uint32_t now) {
    byteSeen_ = true;
    lastByteAt_ = now;

    if (value == '$') {
        // A new '$' always starts a fresh NMEA sentence. If the previous line
        // was not terminated, discard it rather than combining two frames.
        sentenceInProgress_ = true;
        collectingSentenceType_ = true;
        sentenceTypeLength_ = 0;
        sentenceTypeBuffer_[0] = '\0';
        currentSentenceLength_ = 1;
        currentSentenceBuffer_[0] = '$';
        currentSentenceBuffer_[1] = '\0';
        return;
    }

    if (!sentenceInProgress_) {
        return;
    }

    // Different receivers use CRLF, LF-only or occasionally CR-only line
    // endings. Complete the line on either terminator. With CRLF the LF is
    // ignored because finishDiagnosticSentence() clears sentenceInProgress_.
    if (value == '\r' || value == '\n') {
        finishDiagnosticSentence(now);
        return;
    }

    // Never store NUL or other control bytes in a C string. A single embedded
    // NUL directly after '$' would make LVGL display only the dollar sign.
    const unsigned char byte = static_cast<unsigned char>(value);
    if (byte < 0x20U || byte > 0x7EU) {
        return;
    }

    if (currentSentenceLength_ + 1U < sizeof(currentSentenceBuffer_)) {
        currentSentenceBuffer_[currentSentenceLength_++] = value;
        currentSentenceBuffer_[currentSentenceLength_] = '\0';
    }

    if (collectingSentenceType_) {
        if (value == ',' || value == '*') {
            collectingSentenceType_ = false;
        } else if (sentenceTypeLength_ < 5U) {
            sentenceTypeBuffer_[sentenceTypeLength_++] = value;
            sentenceTypeBuffer_[sentenceTypeLength_] = '\0';
        } else {
            collectingSentenceType_ = false;
        }
    }
}

void GpsService::update(std::uint32_t now) {
    if (!view_.uartStarted) {
        return;
    }

    bool parsedData = false;
    while (serial_.available() > 0) {
        const int value = serial_.read();
        if (value >= 0) {
            const char character = static_cast<char>(value);
            processDiagnosticCharacter(character, now);
            parser_.encode(character);
            parsedData = true;
        }
    }

    const std::uint32_t passed = parser_.passedChecksum();
    if (passed != lastPassedChecksums_) {
        lastPassedChecksums_ = passed;
        lastValidSentenceAt_ = now;
        validSentenceSeen_ = true;
        parsedData = true;
    }

    const std::uint32_t lastByteAge = ageOrUnknown(byteSeen_, now, lastByteAt_);
    const std::uint32_t lastSentenceAge = ageOrUnknown(sentenceSeen_, now, lastSentenceAt_);
    const std::uint32_t validPacketAge = ageOrUnknown(
        validSentenceSeen_, now, lastValidSentenceAt_);

    const bool trafficDetected = byteSeen_ &&
        lastByteAge <= AppConfig::GPS_DETECT_TIMEOUT_MS;
    const bool packetDetected = sentenceSeen_ &&
        lastSentenceAge <= AppConfig::GPS_DETECT_TIMEOUT_MS;
    const bool detected = validSentenceSeen_ &&
        validPacketAge <= AppConfig::GPS_DETECT_TIMEOUT_MS;
    const std::uint32_t fixAge = parser_.location.isValid()
        ? parser_.location.age()
        : 0xFFFFFFFFU;
    const bool hasFix = parser_.location.isValid() &&
        fixAge <= AppConfig::GPS_FIX_MAX_AGE_MS;

    bool changed = parsedData ||
        view_.serialTrafficDetected != trafficDetected ||
        view_.nmeaPacketDetected != packetDetected ||
        view_.receiverDetected != detected ||
        view_.hasFix != hasFix;

    const std::uint32_t diagnosticsSecond = now / 1000U;
    if (diagnosticsSecond != lastDiagnosticsSecond_) {
        lastDiagnosticsSecond_ = diagnosticsSecond;
        changed = true;
    }

    const std::uint32_t rateElapsed = now - rateWindowStartedAt_;
    if (rateElapsed >= 1000U) {
        const std::uint32_t currentChars = parser_.charsProcessed();
        const std::uint32_t deltaChars = currentChars - rateWindowChars_;
        view_.charsPerSecond = rateElapsed > 0U
            ? static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(deltaChars) * 1000ULL) / rateElapsed)
            : 0U;
        rateWindowStartedAt_ = now;
        rateWindowChars_ = currentChars;
        changed = true;
    }

    view_.serialTrafficDetected = trafficDetected;
    view_.nmeaPacketDetected = packetDetected;
    view_.receiverDetected = detected;
    view_.hasFix = hasFix;
    view_.fixAgeMs = fixAge;
    view_.lastByteAgeMs = lastByteAge;
    view_.lastSentenceAgeMs = lastSentenceAge;
    view_.lastValidPacketAgeMs = validPacketAge;
    view_.charsProcessed = parser_.charsProcessed();
    view_.sentencesProcessed = sentenceCount_;
    view_.passedChecksums = passed;
    view_.failedChecksums = parser_.failedChecksum();

    if (hasFix) {
        const double latitude = parser_.location.lat();
        const double longitude = parser_.location.lng();
        if (std::fabs(latitude - view_.latitude) > 0.000001 ||
            std::fabs(longitude - view_.longitude) > 0.000001) {
            changed = true;
        }
        view_.latitude = latitude;
        view_.longitude = longitude;
        view_.locatorValid = maidenheadLocator(
            latitude,
            longitude,
            view_.locator,
            sizeof(view_.locator));
    } else {
        view_.locatorValid = false;
        view_.locator[0] = '\0';
    }

    if (parser_.altitude.isValid() &&
        parser_.altitude.age() <= AppConfig::GPS_FIX_MAX_AGE_MS) {
        view_.altitudeMeters = parser_.altitude.meters();
    }

    view_.speedValid = parser_.speed.isValid() &&
        parser_.speed.age() <= AppConfig::GPS_FIX_MAX_AGE_MS;
    if (view_.speedValid) {
        view_.speedKmh = static_cast<float>(parser_.speed.kmph());
    } else {
        view_.speedKmh = 0.0F;
    }

    view_.courseValid = parser_.course.isValid() &&
        parser_.course.age() <= AppConfig::GPS_FIX_MAX_AGE_MS;
    if (view_.courseValid) {
        view_.courseDegrees = static_cast<float>(parser_.course.deg());
    } else {
        view_.courseDegrees = 0.0F;
    }

    if (parser_.satellites.isValid()) {
        view_.satellites = parser_.satellites.value();
    }
    if (parser_.hdop.isValid()) {
        view_.hdop = static_cast<float>(parser_.hdop.hdop());
    }

    view_.utcTimeValid = parser_.time.isValid() &&
        parser_.time.age() <= AppConfig::GPS_DETECT_TIMEOUT_MS;
    if (view_.utcTimeValid) {
        view_.utcHour = parser_.time.hour();
        view_.utcMinute = parser_.time.minute();
        view_.utcSecond = parser_.time.second();
    }

    view_.utcDateValid = parser_.date.isValid() &&
        parser_.date.age() <= AppConfig::GPS_DETECT_TIMEOUT_MS;
    if (view_.utcDateValid) {
        view_.utcDay = parser_.date.day();
        view_.utcMonth = parser_.date.month();
        view_.utcYear = parser_.date.year();
    }

    if (changed) {
        ++view_.revision;
    }
}

const GpsService::ViewState& GpsService::viewState() const {
    return view_;
}

}  // namespace Services
