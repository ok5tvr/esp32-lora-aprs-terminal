#pragma once

#include <cstddef>
#include <cstdint>

namespace Aprs {

constexpr std::uint8_t OE_LORA_HEADER[] = {0x3C, 0xFF, 0x01};
constexpr std::size_t OE_LORA_HEADER_SIZE = sizeof(OE_LORA_HEADER);
constexpr std::size_t MAX_SOURCE_CALL_LENGTH = 15;
constexpr std::size_t MAX_ENTITY_NAME_LENGTH = 15;
constexpr std::size_t MAX_MESSAGE_ADDRESS_LENGTH = 9;
constexpr std::size_t MAX_MESSAGE_TEXT_LENGTH = 67;
constexpr std::size_t MAX_MESSAGE_ID_LENGTH = 5;
constexpr std::size_t MAX_PATH_LENGTH = 192;

enum class EntityType : std::uint8_t {
    Station,
    Object,
    Item
};

enum class PositionFormat : std::uint8_t {
    None,
    Uncompressed,
    Compressed,
    MicE
};

enum class MessageKind : std::uint8_t {
    Text,
    Acknowledgement,
    Rejection
};

struct ParsedMessage {
    bool valid = false;
    MessageKind kind = MessageKind::Text;
    char source[MAX_SOURCE_CALL_LENGTH + 1] = {};
    char addressee[MAX_MESSAGE_ADDRESS_LENGTH + 1] = {};
    char text[MAX_MESSAGE_TEXT_LENGTH + 1] = {};
    char messageId[MAX_MESSAGE_ID_LENGTH + 1] = {};
    bool hasMessageId = false;
};

struct WeatherData {
    bool valid = false;
    bool hasTemperature = false;
    bool hasHumidity = false;
    bool hasPressure = false;
    bool hasWindDirection = false;
    bool hasWindSpeed = false;
    bool hasWindGust = false;
    bool hasRainLastHour = false;
    bool hasRainLast24Hours = false;
    bool hasRainToday = false;
    bool hasSolarRadiation = false;
    float temperatureC = 0.0F;
    float humidityPercent = 0.0F;
    float pressureHpa = 0.0F;
    float windDirectionDegrees = 0.0F;
    float windSpeedKmh = 0.0F;
    float windGustKmh = 0.0F;
    float rainLastHourMm = 0.0F;
    float rainLast24HoursMm = 0.0F;
    float rainTodayMm = 0.0F;
    float solarRadiationWm2 = 0.0F;
};

struct TelemetryData {
    bool valid = false;
    bool hasSequence = false;
    std::uint16_t sequence = 0;
    bool analogValid[5] = {};
    std::uint16_t analog[5] = {};
    bool digitalValid = false;
    bool digital[8] = {};
};

struct PhgData {
    bool valid = false;
    std::uint16_t powerWatts = 0;
    std::uint32_t heightFeet = 0;
    std::uint16_t gainDb = 0;
    std::uint16_t directivityDegrees = 0;
};

struct FrequencyData {
    bool valid = false;
    float frequencyMhz = 0.0F;
    bool hasTone = false;
    float toneHz = 0.0F;
    bool hasOffset = false;
    float offsetMhz = 0.0F;
};

struct PathData {
    bool valid = false;
    bool direct = true;
    std::uint8_t digipeaterHops = 0;
    char path[MAX_PATH_LENGTH + 1] = {};
    char lastDigipeater[MAX_SOURCE_CALL_LENGTH + 1] = {};
};

struct ParsedFrame {
    bool valid = false;
    bool hasPosition = false;
    bool alive = true;
    PositionFormat positionFormat = PositionFormat::None;
    bool emergency = false;
    EntityType type = EntityType::Station;
    char source[MAX_SOURCE_CALL_LENGTH + 1] = {};
    char entityName[MAX_ENTITY_NAME_LENGTH + 1] = {};
    char symbolTable = ' ';
    char symbolCode = ' ';
    double latitude = 0.0;
    double longitude = 0.0;
    WeatherData weather;
    TelemetryData telemetry;
    PhgData phg;
    FrequencyData frequency;
    PathData path;
};

bool encodeTnc2(
    const char* tnc2,
    bool addOeHeader,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength);

// Creates a printable representation for the UI. Non-printable Mic-E bytes
// are replaced by '.'. Use decodeRaw() for protocol parsing.
bool decodeText(
    const std::uint8_t* packet,
    std::size_t packetLength,
    char* output,
    std::size_t outputCapacity,
    bool& hadOeHeader);

// Removes the optional OE/DL header and preserves all payload bytes, including
// non-printable Mic-E data. outputLength does not include the trailing NUL.
bool decodeRaw(
    const std::uint8_t* packet,
    std::size_t packetLength,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength,
    bool& hadOeHeader);

// Binary-safe parser for TNC2 frames. Supports normal and compressed station
// positions, Mic-E, APRS objects, APRS items, common APRS weather fields
// and one third-party wrapper.
bool parseTnc2(
    const std::uint8_t* tnc2,
    std::size_t length,
    ParsedFrame& frame);

bool parseTnc2(const char* tnc2, ParsedFrame& frame);

// Parses APRS directed messages in TNC2 form. The addressee field is exactly
// nine characters on air and is returned without trailing padding spaces.
bool parseMessageTnc2(
    const std::uint8_t* tnc2,
    std::size_t length,
    ParsedMessage& message);

bool parseMessageTnc2(const char* tnc2, ParsedMessage& message);

// Builds a directed APRS message. Message text is limited to 67 printable
// ASCII characters and may not contain '|', '~' or '{'. A non-empty message
// identifier requests an acknowledgement from the recipient.
bool buildMessageTnc2(
    const char* callsign,
    const char* destination,
    const char* addressee,
    const char* text,
    const char* messageId,
    char* output,
    std::size_t outputCapacity);

// Builds :ADDRESSEE:ackNNN or :ADDRESSEE:rejNNN.
bool buildMessageResponseTnc2(
    const char* callsign,
    const char* destination,
    const char* addressee,
    const char* messageId,
    bool accepted,
    char* output,
    std::size_t outputCapacity);

// Builds a complete TNC2 APRS position frame. The uncompressed form uses
// ddmm.mm/dddmm.mm coordinates. The compressed form uses the 13-character
// Base-91 position field from APRS 1.0.1. Speed is supplied in knots.
bool buildPositionTnc2(
    const char* callsign,
    const char* destination,
    double latitude,
    double longitude,
    char symbolTable,
    char symbolCode,
    bool compressed,
    bool includeCourseSpeed,
    double courseDegrees,
    double speedKnots,
    const char* comment,
    char* output,
    std::size_t outputCapacity);

struct DigipeaterOptions {
    bool fillInWide1 = true;
    bool traceWide2 = false;
    std::uint8_t maxWideHops = 2;
};

// Builds a traceable APRS digipeater copy. Only the first unused path
// component may be consumed. WIDE1-1 is handled as a fill-in alias;
// WIDE2-N is decremented and the local callsign is inserted with '*'.
// Obsolete RELAY/WIDE/TRACE aliases and abusive values above maxWideHops
// are deliberately not repeated. The function is binary-safe for Mic-E.
bool buildDigipeatedTnc2(
    const std::uint8_t* tnc2,
    std::size_t length,
    const char* ownCallsign,
    const DigipeaterOptions& options,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength);

// Builds the RF-to-APRS-IS line for a receive-only IGate. It rejects
// generic queries and paths containing TCPIP, TCPXX, NOGATE, RFONLY or
// q constructs. Third-party packets without an Internet marker are
// unwrapped as required by APRS-IS. qAO is appended for one-way gating.
bool buildReceiveOnlyIgateTnc2(
    const std::uint8_t* tnc2,
    std::size_t length,
    const char* igateCallsign,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength);

// APRS duplicate identity: origin, destination and data, deliberately
// ignoring the digipeater path. Suitable for a 30-second duplicate cache.
std::uint32_t tnc2PacketHash(const std::uint8_t* tnc2, std::size_t length);

}  // namespace Aprs
