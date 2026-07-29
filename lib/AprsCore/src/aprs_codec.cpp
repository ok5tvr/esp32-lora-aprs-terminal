#include "aprs_codec.h"

#include <cstring>
#include <cmath>
#include <cstdio>

namespace Aprs {
namespace {

bool isDigitOrSpace(std::uint8_t value) {
    return (value >= '0' && value <= '9') || value == ' ';
}

int digitValue(std::uint8_t value) {
    return value == ' ' ? 0 : static_cast<int>(value - '0');
}

void copyText(
    char* destination,
    std::size_t destinationCapacity,
    const std::uint8_t* source,
    std::size_t sourceLength,
    bool trimTrailingSpaces) {

    if (destination == nullptr || destinationCapacity == 0) {
        return;
    }

    std::size_t length = sourceLength;
    if (trimTrailingSpaces) {
        while (length > 0 && source[length - 1] == ' ') {
            --length;
        }
    }
    if (length >= destinationCapacity) {
        length = destinationCapacity - 1;
    }
    if (length > 0 && source != nullptr) {
        std::memcpy(destination, source, length);
    }
    destination[length] = '\0';
}

const std::uint8_t* findByte(
    const std::uint8_t* data,
    std::size_t length,
    std::uint8_t value) {

    if (data == nullptr) {
        return nullptr;
    }
    return static_cast<const std::uint8_t*>(std::memchr(data, value, length));
}

bool parseUncompressedPosition(
    const std::uint8_t* position,
    std::size_t length,
    ParsedFrame& frame) {

    if (position == nullptr || length < 19) {
        return false;
    }

    if (!isDigitOrSpace(position[0]) || !isDigitOrSpace(position[1]) ||
        !isDigitOrSpace(position[2]) || !isDigitOrSpace(position[3]) ||
        position[4] != '.' || !isDigitOrSpace(position[5]) ||
        !isDigitOrSpace(position[6]) ||
        (position[7] != 'N' && position[7] != 'S')) {
        return false;
    }

    if (!isDigitOrSpace(position[9]) || !isDigitOrSpace(position[10]) ||
        !isDigitOrSpace(position[11]) || !isDigitOrSpace(position[12]) ||
        !isDigitOrSpace(position[13]) || position[14] != '.' ||
        !isDigitOrSpace(position[15]) || !isDigitOrSpace(position[16]) ||
        (position[17] != 'E' && position[17] != 'W')) {
        return false;
    }

    const int latitudeDegrees = digitValue(position[0]) * 10 + digitValue(position[1]);
    const double latitudeMinutes =
        static_cast<double>(digitValue(position[2]) * 10 + digitValue(position[3])) +
        static_cast<double>(digitValue(position[5]) * 10 + digitValue(position[6])) / 100.0;

    const int longitudeDegrees =
        digitValue(position[9]) * 100 + digitValue(position[10]) * 10 + digitValue(position[11]);
    const double longitudeMinutes =
        static_cast<double>(digitValue(position[12]) * 10 + digitValue(position[13])) +
        static_cast<double>(digitValue(position[15]) * 10 + digitValue(position[16])) / 100.0;

    if (latitudeDegrees > 90 || longitudeDegrees > 180 ||
        latitudeMinutes >= 60.0 || longitudeMinutes >= 60.0) {
        return false;
    }

    double latitude = static_cast<double>(latitudeDegrees) + latitudeMinutes / 60.0;
    double longitude = static_cast<double>(longitudeDegrees) + longitudeMinutes / 60.0;
    if (position[7] == 'S') {
        latitude = -latitude;
    }
    if (position[17] == 'W') {
        longitude = -longitude;
    }

    frame.latitude = latitude;
    frame.longitude = longitude;
    frame.symbolTable = static_cast<char>(position[8]);
    frame.symbolCode = static_cast<char>(position[18]);
    frame.hasPosition = true;
    return true;
}

bool isBase91(std::uint8_t value) {
    return value >= 33 && value <= 123;
}

std::uint32_t decodeBase91(const std::uint8_t* input) {
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < 4; ++index) {
        value = value * 91U + static_cast<std::uint32_t>(input[index] - 33);
    }
    return value;
}

bool parseCompressedPosition(
    const std::uint8_t* position,
    std::size_t length,
    ParsedFrame& frame) {

    if (position == nullptr || length < 10) {
        return false;
    }

    for (std::size_t index = 1; index <= 8; ++index) {
        if (!isBase91(position[index])) {
            return false;
        }
    }

    const std::uint32_t latitudeValue = decodeBase91(position + 1);
    const std::uint32_t longitudeValue = decodeBase91(position + 5);
    const double latitude = 90.0 - static_cast<double>(latitudeValue) / 380926.0;
    const double longitude = -180.0 + static_cast<double>(longitudeValue) / 190463.0;

    if (latitude < -90.0 || latitude > 90.0 ||
        longitude < -180.0 || longitude > 180.0) {
        return false;
    }

    frame.latitude = latitude;
    frame.longitude = longitude;
    frame.symbolTable = static_cast<char>(position[0]);
    frame.symbolCode = static_cast<char>(position[9]);
    frame.hasPosition = true;
    return true;
}

bool parsePositionAt(
    const std::uint8_t* position,
    std::size_t length,
    ParsedFrame& frame) {

    if (parseUncompressedPosition(position, length, frame)) {
        return true;
    }
    return parseCompressedPosition(position, length, frame);
}

bool parsePositionInformation(
    const std::uint8_t* information,
    std::size_t length,
    ParsedFrame& frame) {

    if (information == nullptr || length == 0) {
        return false;
    }

    const std::uint8_t* position = nullptr;
    std::size_t positionLength = 0;
    if (information[0] == '!' || information[0] == '=') {
        position = information + 1;
        positionLength = length - 1;
    } else if (information[0] == '/' || information[0] == '@') {
        if (length < 9) {
            return false;
        }
        position = information + 8;
        positionLength = length - 8;
    } else {
        return false;
    }

    return parsePositionAt(position, positionLength, frame);
}

bool decodeMicEDigit(std::uint8_t value, int& digit, bool& ambiguous) {
    ambiguous = false;
    if (value >= '0' && value <= '9') {
        digit = static_cast<int>(value - '0');
        return true;
    }
    if (value >= 'A' && value <= 'J') {
        digit = static_cast<int>(value - 'A');
        return true;
    }
    if (value >= 'P' && value <= 'Y') {
        digit = static_cast<int>(value - 'P');
        return true;
    }
    if (value == 'K' || value == 'L' || value == 'Z') {
        digit = 0;
        ambiguous = true;
        return true;
    }
    return false;
}

bool micEHighBit(std::uint8_t value) {
    return value >= 'P' && value <= 'Z';
}

bool parseMicEPosition(
    const std::uint8_t* destination,
    std::size_t destinationLength,
    const std::uint8_t* information,
    std::size_t informationLength,
    ParsedFrame& frame) {

    if (destination == nullptr || destinationLength < 6 ||
        information == nullptr || informationLength < 9) {
        return false;
    }

    const std::uint8_t dataType = information[0];
    if (dataType != '`' && dataType != '\'' && dataType != 0x1c && dataType != 0x1d) {
        return false;
    }

    int digits[6] = {};
    bool ambiguity[6] = {};
    for (std::size_t index = 0; index < 6; ++index) {
        if (!decodeMicEDigit(destination[index], digits[index], ambiguity[index])) {
            return false;
        }
    }

    const int latitudeDegrees = digits[0] * 10 + digits[1];
    const double latitudeMinutes =
        static_cast<double>(digits[2] * 10 + digits[3]) +
        static_cast<double>(digits[4] * 10 + digits[5]) / 100.0;
    if (latitudeDegrees > 90 || latitudeMinutes >= 60.0) {
        return false;
    }

    int longitudeDegrees = static_cast<int>(information[1]) - 28;
    if (micEHighBit(destination[4])) {
        longitudeDegrees += 100;
    }
    if (longitudeDegrees >= 180 && longitudeDegrees <= 189) {
        longitudeDegrees -= 80;
    } else if (longitudeDegrees >= 190 && longitudeDegrees <= 199) {
        longitudeDegrees -= 190;
    }

    int longitudeMinutes = static_cast<int>(information[2]) - 28;
    if (longitudeMinutes >= 60) {
        longitudeMinutes -= 60;
    }
    const int longitudeHundredths = static_cast<int>(information[3]) - 28;

    if (longitudeDegrees < 0 || longitudeDegrees > 180 ||
        longitudeMinutes < 0 || longitudeMinutes >= 60 ||
        longitudeHundredths < 0 || longitudeHundredths > 99) {
        return false;
    }

    double latitude = static_cast<double>(latitudeDegrees) + latitudeMinutes / 60.0;
    double longitude = static_cast<double>(longitudeDegrees) +
        (static_cast<double>(longitudeMinutes) +
         static_cast<double>(longitudeHundredths) / 100.0) / 60.0;

    if (!micEHighBit(destination[3])) {
        latitude = -latitude;
    }
    if (micEHighBit(destination[5])) {
        longitude = -longitude;
    }

    frame.latitude = latitude;
    frame.longitude = longitude;
    frame.symbolCode = static_cast<char>(information[7]);
    frame.symbolTable = static_cast<char>(information[8]);
    frame.hasPosition = true;
    return true;
}



bool parseUnsignedDigits(
    const std::uint8_t* data,
    std::size_t length,
    std::size_t start,
    std::size_t count,
    int& value) {

    if (data == nullptr || start + count > length || count == 0) {
        return false;
    }
    int parsed = 0;
    for (std::size_t index = 0; index < count; ++index) {
        const std::uint8_t ch = data[start + index];
        if (ch < '0' || ch > '9') {
            return false;
        }
        parsed = parsed * 10 + static_cast<int>(ch - '0');
    }
    value = parsed;
    return true;
}

bool parseSignedTemperatureF(
    const std::uint8_t* data,
    std::size_t length,
    std::size_t start,
    int& value) {

    if (data == nullptr || start + 3 > length) {
        return false;
    }
    if (data[start] == '-') {
        int magnitude = 0;
        if (!parseUnsignedDigits(data, length, start + 1, 2, magnitude)) {
            return false;
        }
        value = -magnitude;
        return true;
    }
    return parseUnsignedDigits(data, length, start, 3, value);
}

void parseWeatherInformation(
    const std::uint8_t* information,
    std::size_t length,
    ParsedFrame& frame) {

    if (information == nullptr || length == 0) {
        return;
    }

    WeatherData weather;
    unsigned parsedFields = 0;

    // Complete weather reports often use ddd/sss for direction and speed.
    // APRS 1.01 defines this data extension in knots; cddd/sddd fields below
    // are the positionless-weather representation in mph.
    for (std::size_t index = 0; index + 7 <= length; ++index) {
        int direction = 0;
        int speed = 0;
        if (parseUnsignedDigits(information, length, index, 3, direction) &&
            information[index + 3] == '/' &&
            parseUnsignedDigits(information, length, index + 4, 3, speed) &&
            direction >= 0 && direction <= 360) {
            weather.hasWindDirection = true;
            weather.windDirectionDegrees = static_cast<float>(direction);
            weather.hasWindSpeed = true;
            weather.windSpeedKmh = static_cast<float>(speed) * 1.852F;
            parsedFields += 2;
            break;
        }
    }

    for (std::size_t index = 0; index < length; ++index) {
        int value = 0;
        switch (information[index]) {
            case 'c':
                if (parseUnsignedDigits(information, length, index + 1, 3, value) &&
                    value <= 360) {
                    weather.hasWindDirection = true;
                    weather.windDirectionDegrees = static_cast<float>(value);
                    ++parsedFields;
                    index += 3;
                }
                break;
            case 's':
                if (parseUnsignedDigits(information, length, index + 1, 3, value)) {
                    weather.hasWindSpeed = true;
                    weather.windSpeedKmh = static_cast<float>(value) * 1.609344F;
                    ++parsedFields;
                    index += 3;
                }
                break;
            case 'g':
                if (parseUnsignedDigits(information, length, index + 1, 3, value)) {
                    weather.hasWindGust = true;
                    weather.windGustKmh = static_cast<float>(value) * 1.609344F;
                    ++parsedFields;
                    index += 3;
                }
                break;
            case 't':
                if (parseSignedTemperatureF(information, length, index + 1, value) &&
                    value >= -99 && value <= 199) {
                    weather.hasTemperature = true;
                    weather.temperatureC =
                        (static_cast<float>(value) - 32.0F) * (5.0F / 9.0F);
                    ++parsedFields;
                    index += 3;
                }
                break;
            case 'r':
                if (parseUnsignedDigits(information, length, index + 1, 3, value)) {
                    weather.hasRainLastHour = true;
                    weather.rainLastHourMm = static_cast<float>(value) * 0.254F;
                    ++parsedFields;
                    index += 3;
                }
                break;
            case 'p':
                if (parseUnsignedDigits(information, length, index + 1, 3, value)) {
                    weather.hasRainLast24Hours = true;
                    weather.rainLast24HoursMm = static_cast<float>(value) * 0.254F;
                    ++parsedFields;
                    index += 3;
                }
                break;
            case 'P':
                if (parseUnsignedDigits(information, length, index + 1, 3, value)) {
                    weather.hasRainToday = true;
                    weather.rainTodayMm = static_cast<float>(value) * 0.254F;
                    ++parsedFields;
                    index += 3;
                }
                break;
            case 'h':
                if (parseUnsignedDigits(information, length, index + 1, 2, value) &&
                    value <= 99) {
                    weather.hasHumidity = true;
                    weather.humidityPercent = static_cast<float>(value == 0 ? 100 : value);
                    ++parsedFields;
                    index += 2;
                }
                break;
            case 'b':
                if (parseUnsignedDigits(information, length, index + 1, 5, value) &&
                    value >= 8000 && value <= 12000) {
                    weather.hasPressure = true;
                    weather.pressureHpa = static_cast<float>(value) / 10.0F;
                    ++parsedFields;
                    index += 5;
                }
                break;
            case 'L':
                if (parseUnsignedDigits(information, length, index + 1, 3, value)) {
                    weather.hasSolarRadiation = true;
                    weather.solarRadiationWm2 = static_cast<float>(value);
                    ++parsedFields;
                    index += 3;
                }
                break;
            case 'l':
                if (parseUnsignedDigits(information, length, index + 1, 3, value)) {
                    weather.hasSolarRadiation = true;
                    weather.solarRadiationWm2 = static_cast<float>(value + 1000);
                    ++parsedFields;
                    index += 3;
                }
                break;
            default:
                break;
        }
    }

    const bool explicitWeatherPacket = information[0] == '_';
    const bool weatherSymbol = frame.symbolCode == '_';
    weather.valid = parsedFields >= 2U ||
        ((explicitWeatherPacket || weatherSymbol) && parsedFields >= 1U);
    if (weather.valid) {
        frame.weather = weather;
    }
}

bool parseObject(
    const std::uint8_t* information,
    std::size_t length,
    ParsedFrame& frame) {

    // ';' + 9-byte name + live/kill + 7-byte timestamp + position.
    if (information == nullptr || length < 18 || information[0] != ';') {
        return false;
    }
    if (information[10] != '*' && information[10] != '_') {
        return false;
    }

    frame.type = EntityType::Object;
    frame.alive = information[10] == '*';
    copyText(frame.entityName, sizeof(frame.entityName), information + 1, 9, true);
    if (frame.entityName[0] == '\0') {
        return false;
    }

    if (length > 18) {
        parsePositionAt(information + 18, length - 18, frame);
    }
    return true;
}

bool parseItem(
    const std::uint8_t* information,
    std::size_t length,
    ParsedFrame& frame) {

    if (information == nullptr || length < 5 || information[0] != ')') {
        return false;
    }

    std::size_t separatorIndex = 0;
    const std::size_t maximumIndex = length - 1 < 10 ? length - 1 : 10;
    for (std::size_t index = 4; index <= maximumIndex; ++index) {
        if (information[index] == '!' || information[index] == '_') {
            separatorIndex = index;
            break;
        }
    }
    if (separatorIndex == 0) {
        return false;
    }

    const std::size_t nameLength = separatorIndex - 1;
    if (nameLength < 3 || nameLength > 9) {
        return false;
    }

    frame.type = EntityType::Item;
    frame.alive = information[separatorIndex] == '!';
    copyText(
        frame.entityName,
        sizeof(frame.entityName),
        information + 1,
        nameLength,
        true);
    if (frame.entityName[0] == '\0') {
        return false;
    }

    const std::size_t positionIndex = separatorIndex + 1;
    if (positionIndex < length) {
        parsePositionAt(information + positionIndex, length - positionIndex, frame);
    }
    return true;
}

bool parseTnc2Internal(
    const std::uint8_t* tnc2,
    std::size_t length,
    ParsedFrame& frame,
    bool allowThirdParty) {

    if (tnc2 == nullptr || length == 0) {
        return false;
    }

    const std::uint8_t* destinationSeparator = findByte(tnc2, length, '>');
    const std::uint8_t* informationSeparator = findByte(tnc2, length, ':');
    if (destinationSeparator == nullptr || informationSeparator == nullptr ||
        destinationSeparator <= tnc2 || destinationSeparator > informationSeparator) {
        return false;
    }

    const std::size_t sourceLength =
        static_cast<std::size_t>(destinationSeparator - tnc2);
    if (sourceLength == 0 || sourceLength > MAX_SOURCE_CALL_LENGTH) {
        return false;
    }

    ParsedFrame parsed;
    copyText(parsed.source, sizeof(parsed.source), tnc2, sourceLength, false);
    parsed.valid = true;
    parsed.type = EntityType::Station;
    parsed.alive = true;
    copyText(
        parsed.entityName,
        sizeof(parsed.entityName),
        tnc2,
        sourceLength,
        false);

    const std::uint8_t* information = informationSeparator + 1;
    const std::size_t informationLength =
        length - static_cast<std::size_t>(information - tnc2);

    if (allowThirdParty && informationLength > 1 && information[0] == '}') {
        ParsedFrame inner;
        if (parseTnc2Internal(information + 1, informationLength - 1, inner, false)) {
            frame = inner;
            return true;
        }
    }

    if (informationLength > 0 && information[0] == ';') {
        if (!parseObject(information, informationLength, parsed)) {
            return false;
        }
        parseWeatherInformation(information, informationLength, parsed);
        frame = parsed;
        return true;
    }

    if (informationLength > 0 && information[0] == ')') {
        if (!parseItem(information, informationLength, parsed)) {
            return false;
        }
        parseWeatherInformation(information, informationLength, parsed);
        frame = parsed;
        return true;
    }

    const std::uint8_t* destination = destinationSeparator + 1;
    std::size_t destinationLength =
        static_cast<std::size_t>(informationSeparator - destination);
    const std::uint8_t* pathSeparator = findByte(destination, destinationLength, ',');
    if (pathSeparator != nullptr) {
        destinationLength = static_cast<std::size_t>(pathSeparator - destination);
    }
    const std::uint8_t* ssidSeparator = findByte(destination, destinationLength, '-');
    if (ssidSeparator != nullptr) {
        destinationLength = static_cast<std::size_t>(ssidSeparator - destination);
    }

    if (!parseMicEPosition(
            destination,
            destinationLength,
            information,
            informationLength,
            parsed)) {
        parsePositionInformation(information, informationLength, parsed);
    }
    parseWeatherInformation(information, informationLength, parsed);

    frame = parsed;
    return true;
}

}  // namespace

bool encodeTnc2(
    const char* tnc2,
    bool addOeHeader,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength) {

    outputLength = 0;
    if (tnc2 == nullptr || output == nullptr || outputCapacity == 0) {
        return false;
    }

    const std::size_t textLength = std::strlen(tnc2);
    const std::size_t headerLength = addOeHeader ? OE_LORA_HEADER_SIZE : 0;
    if (textLength == 0 || headerLength + textLength > outputCapacity) {
        return false;
    }

    if (addOeHeader) {
        std::memcpy(output, OE_LORA_HEADER, OE_LORA_HEADER_SIZE);
    }
    std::memcpy(output + headerLength, tnc2, textLength);
    outputLength = headerLength + textLength;
    return true;
}

bool decodeText(
    const std::uint8_t* packet,
    std::size_t packetLength,
    char* output,
    std::size_t outputCapacity,
    bool& hadOeHeader) {

    hadOeHeader = false;
    if (packet == nullptr || output == nullptr || outputCapacity == 0) {
        return false;
    }

    std::size_t offset = 0;
    if (packetLength >= OE_LORA_HEADER_SIZE &&
        std::memcmp(packet, OE_LORA_HEADER, OE_LORA_HEADER_SIZE) == 0) {
        hadOeHeader = true;
        offset = OE_LORA_HEADER_SIZE;
    }

    const std::size_t textLength = packetLength - offset;
    const std::size_t copyLength =
        textLength < outputCapacity - 1 ? textLength : outputCapacity - 1;

    for (std::size_t index = 0; index < copyLength; ++index) {
        const std::uint8_t value = packet[offset + index];
        output[index] = (value >= 32 && value <= 126)
            ? static_cast<char>(value)
            : '.';
    }
    output[copyLength] = '\0';
    return true;
}

bool decodeRaw(
    const std::uint8_t* packet,
    std::size_t packetLength,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength,
    bool& hadOeHeader) {

    outputLength = 0;
    hadOeHeader = false;
    if (packet == nullptr || output == nullptr || outputCapacity == 0) {
        return false;
    }

    std::size_t offset = 0;
    if (packetLength >= OE_LORA_HEADER_SIZE &&
        std::memcmp(packet, OE_LORA_HEADER, OE_LORA_HEADER_SIZE) == 0) {
        hadOeHeader = true;
        offset = OE_LORA_HEADER_SIZE;
    }

    const std::size_t payloadLength = packetLength - offset;
    if (payloadLength + 1 > outputCapacity) {
        return false;
    }

    if (payloadLength > 0) {
        std::memcpy(output, packet + offset, payloadLength);
    }
    output[payloadLength] = 0;
    outputLength = payloadLength;
    return true;
}

bool parseTnc2(
    const std::uint8_t* tnc2,
    std::size_t length,
    ParsedFrame& frame) {

    frame = ParsedFrame{};
    return parseTnc2Internal(tnc2, length, frame, true);
}

bool parseTnc2(const char* tnc2, ParsedFrame& frame) {
    if (tnc2 == nullptr) {
        frame = ParsedFrame{};
        return false;
    }
    return parseTnc2(
        reinterpret_cast<const std::uint8_t*>(tnc2),
        std::strlen(tnc2),
        frame);
}

namespace {

bool validPositionInput(double latitude, double longitude) {
    return std::isfinite(latitude) && std::isfinite(longitude) &&
        latitude >= -90.0 && latitude <= 90.0 &&
        longitude >= -180.0 && longitude <= 180.0;
}

void normalizeDegreesMinutes(
    double coordinate,
    int maximumDegrees,
    int& degrees,
    double& minutes,
    char& hemisphere,
    char positiveHemisphere,
    char negativeHemisphere) {

    hemisphere = coordinate < 0.0 ? negativeHemisphere : positiveHemisphere;
    const double absolute = coordinate < 0.0 ? -coordinate : coordinate;
    degrees = static_cast<int>(std::floor(absolute));
    minutes = (absolute - static_cast<double>(degrees)) * 60.0;
    minutes = std::round(minutes * 100.0) / 100.0;
    if (minutes >= 60.0) {
        minutes = 0.0;
        ++degrees;
    }
    if (degrees > maximumDegrees) {
        degrees = maximumDegrees;
        minutes = 0.0;
    }
}

void encodeBase91Four(std::uint32_t value, char output[5]) {
    constexpr std::uint32_t POWERS[4] = {91U * 91U * 91U, 91U * 91U, 91U, 1U};
    for (std::size_t index = 0; index < 4; ++index) {
        const std::uint32_t digit = value / POWERS[index];
        value %= POWERS[index];
        output[index] = static_cast<char>(digit + 33U);
    }
    output[4] = '\0';
}

}  // namespace

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
    std::size_t outputCapacity) {

    if (callsign == nullptr || callsign[0] == '\0' ||
        destination == nullptr || destination[0] == '\0' ||
        output == nullptr || outputCapacity == 0 ||
        !validPositionInput(latitude, longitude)) {
        return false;
    }

    const char* safeComment = comment != nullptr ? comment : "";
    int written = -1;

    if (!compressed) {
        int latitudeDegrees = 0;
        int longitudeDegrees = 0;
        double latitudeMinutes = 0.0;
        double longitudeMinutes = 0.0;
        char latitudeHemisphere = 'N';
        char longitudeHemisphere = 'E';
        normalizeDegreesMinutes(
            latitude, 90, latitudeDegrees, latitudeMinutes,
            latitudeHemisphere, 'N', 'S');
        normalizeDegreesMinutes(
            longitude, 180, longitudeDegrees, longitudeMinutes,
            longitudeHemisphere, 'E', 'W');

        char extension[16] = {};
        if (includeCourseSpeed && std::isfinite(courseDegrees) && std::isfinite(speedKnots)) {
            double normalizedCourse = std::fmod(courseDegrees, 360.0);
            if (normalizedCourse < 0.0) {
                normalizedCourse += 360.0;
            }
            int course = static_cast<int>(std::lround(normalizedCourse));
            if (course >= 360) {
                course = 0;
            }
            const double boundedSpeed = std::fmin(999.0, std::fmax(0.0, speedKnots));
            const int speed = static_cast<int>(std::lround(boundedSpeed));
            std::snprintf(extension, sizeof(extension), "%03u/%03u",
                          static_cast<unsigned int>(course),
                          static_cast<unsigned int>(speed));
        }

        written = std::snprintf(
            output,
            outputCapacity,
            "%s>%s:!%02d%05.2f%c%c%03d%05.2f%c%c%s%s%s",
            callsign,
            destination,
            latitudeDegrees,
            latitudeMinutes,
            latitudeHemisphere,
            symbolTable,
            longitudeDegrees,
            longitudeMinutes,
            longitudeHemisphere,
            symbolCode,
            extension,
            safeComment[0] != '\0' ? " " : "",
            safeComment);
    } else {
        const double latitudeValueDouble = 380926.0 * (90.0 - latitude);
        const double longitudeValueDouble = 190463.0 * (180.0 + longitude);
        if (latitudeValueDouble < 0.0 || longitudeValueDouble < 0.0) {
            return false;
        }

        char encodedLatitude[5] = {};
        char encodedLongitude[5] = {};
        encodeBase91Four(
            static_cast<std::uint32_t>(std::floor(latitudeValueDouble)),
            encodedLatitude);
        encodeBase91Four(
            static_cast<std::uint32_t>(std::floor(longitudeValueDouble)),
            encodedLongitude);

        char courseSpeedType[4] = {' ', 's', 'T', '\0'};
        if (includeCourseSpeed && std::isfinite(courseDegrees) && std::isfinite(speedKnots)) {
            double normalizedCourse = std::fmod(courseDegrees, 360.0);
            if (normalizedCourse < 0.0) {
                normalizedCourse += 360.0;
            }
            int courseValue = static_cast<int>(std::floor(normalizedCourse / 4.0));
            if (courseValue < 0) {
                courseValue = 0;
            } else if (courseValue > 89) {
                courseValue = 89;
            }

            const double safeSpeed = speedKnots < 0.0 ? 0.0 : speedKnots;
            int speedValue = static_cast<int>(
                std::lround(std::log(safeSpeed + 1.0) / std::log(1.08)));
            if (speedValue < 0) {
                speedValue = 0;
            } else if (speedValue > 89) {
                speedValue = 89;
            }

            courseSpeedType[0] = static_cast<char>(courseValue + 33);
            courseSpeedType[1] = static_cast<char>(speedValue + 33);
            // Current RMC fix, compressed by an "other tracker":
            // 001 11 110b = 62, then +33 for Base-91 printable ASCII.
            courseSpeedType[2] = static_cast<char>(62 + 33);
        }

        written = std::snprintf(
            output,
            outputCapacity,
            "%s>%s:!%c%s%s%c%s%s%s",
            callsign,
            destination,
            symbolTable,
            encodedLatitude,
            encodedLongitude,
            symbolCode,
            courseSpeedType,
            safeComment[0] != '\0' ? " " : "",
            safeComment);
    }

    return written > 0 && static_cast<std::size_t>(written) < outputCapacity;
}


namespace {

bool messageIdValid(const char* value) {
    if (value == nullptr || value[0] == '\0') {
        return false;
    }
    std::size_t length = 0;
    for (const char* cursor = value; *cursor != '\0'; ++cursor) {
        if (!((*cursor >= '0' && *cursor <= '9') ||
              (*cursor >= 'A' && *cursor <= 'Z') ||
              (*cursor >= 'a' && *cursor <= 'z'))) {
            return false;
        }
        ++length;
        if (length > MAX_MESSAGE_ID_LENGTH) {
            return false;
        }
    }
    return length > 0;
}

bool messageTextValid(const char* text) {
    if (text == nullptr) {
        return false;
    }
    std::size_t length = 0;
    for (const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text);
         *cursor != 0;
         ++cursor) {
        if (*cursor < 32 || *cursor > 126 || *cursor == '|' ||
            *cursor == '~' || *cursor == '{') {
            return false;
        }
        ++length;
        if (length > MAX_MESSAGE_TEXT_LENGTH) {
            return false;
        }
    }
    return length <= MAX_MESSAGE_TEXT_LENGTH;
}

bool normalizeMessageAddress(
    const char* input,
    char* output,
    std::size_t outputCapacity) {

    if (input == nullptr || output == nullptr ||
        outputCapacity < MAX_MESSAGE_ADDRESS_LENGTH + 1) {
        return false;
    }

    std::size_t length = 0;
    for (const char* cursor = input; *cursor != '\0'; ++cursor) {
        char value = *cursor;
        if (value == ' ' || value == '\t') {
            continue;
        }
        if (value >= 'a' && value <= 'z') {
            value = static_cast<char>(value - 'a' + 'A');
        }
        if (!((value >= 'A' && value <= 'Z') ||
              (value >= '0' && value <= '9') || value == '-')) {
            return false;
        }
        if (length >= MAX_MESSAGE_ADDRESS_LENGTH) {
            return false;
        }
        output[length++] = value;
    }
    output[length] = '\0';
    return length > 0;
}

bool parseMessageTnc2Internal(
    const std::uint8_t* tnc2,
    std::size_t length,
    ParsedMessage& message,
    bool allowThirdParty) {

    if (tnc2 == nullptr || length < 4) {
        return false;
    }

    const std::uint8_t* sourceEnd = findByte(tnc2, length, '>');
    if (sourceEnd == nullptr || sourceEnd == tnc2) {
        return false;
    }
    const std::size_t sourceLength = static_cast<std::size_t>(sourceEnd - tnc2);
    if (sourceLength > MAX_SOURCE_CALL_LENGTH) {
        return false;
    }

    const std::size_t afterSource = sourceLength + 1;
    if (afterSource >= length) {
        return false;
    }
    const std::uint8_t* informationSeparator = findByte(
        tnc2 + afterSource,
        length - afterSource,
        ':');
    if (informationSeparator == nullptr || informationSeparator + 1 > tnc2 + length) {
        return false;
    }

    const std::uint8_t* information = informationSeparator + 1;
    const std::size_t informationLength = static_cast<std::size_t>(
        (tnc2 + length) - information);

    if (allowThirdParty && informationLength > 1 && information[0] == '}') {
        return parseMessageTnc2Internal(
            information + 1,
            informationLength - 1,
            message,
            false);
    }

    // ':' DTI + 9-byte addressee + ':' + body.
    if (informationLength < 11 || information[0] != ':' || information[10] != ':') {
        return false;
    }

    copyText(message.source, sizeof(message.source), tnc2, sourceLength, false);
    copyText(message.addressee, sizeof(message.addressee), information + 1, 9, true);
    if (message.addressee[0] == '\0') {
        return false;
    }

    const std::uint8_t* body = information + 11;
    const std::size_t bodyLength = informationLength - 11;

    if (bodyLength >= 4 &&
        ((std::memcmp(body, "ack", 3) == 0) ||
         (std::memcmp(body, "rej", 3) == 0))) {
        const std::size_t idLength = bodyLength - 3;
        if (idLength == 0 || idLength > MAX_MESSAGE_ID_LENGTH) {
            return false;
        }
        copyText(message.messageId, sizeof(message.messageId), body + 3, idLength, false);
        if (!messageIdValid(message.messageId)) {
            return false;
        }
        message.kind = std::memcmp(body, "ack", 3) == 0
            ? MessageKind::Acknowledgement
            : MessageKind::Rejection;
        message.hasMessageId = true;
        message.valid = true;
        return true;
    }

    const std::uint8_t* idMarker = findByte(body, bodyLength, '{');
    std::size_t textLength = bodyLength;
    if (idMarker != nullptr) {
        textLength = static_cast<std::size_t>(idMarker - body);
        const std::size_t idLength = bodyLength - textLength - 1;
        if (idLength == 0 || idLength > MAX_MESSAGE_ID_LENGTH) {
            return false;
        }
        copyText(message.messageId, sizeof(message.messageId), idMarker + 1, idLength, false);
        if (!messageIdValid(message.messageId)) {
            return false;
        }
        message.hasMessageId = true;
    }

    if (textLength > MAX_MESSAGE_TEXT_LENGTH) {
        return false;
    }
    copyText(message.text, sizeof(message.text), body, textLength, false);
    if (!messageTextValid(message.text)) {
        return false;
    }

    message.kind = MessageKind::Text;
    message.valid = true;
    return true;
}

}  // namespace

bool parseMessageTnc2(
    const std::uint8_t* tnc2,
    std::size_t length,
    ParsedMessage& message) {

    message = ParsedMessage{};
    return parseMessageTnc2Internal(tnc2, length, message, true);
}

bool parseMessageTnc2(const char* tnc2, ParsedMessage& message) {
    if (tnc2 == nullptr) {
        message = ParsedMessage{};
        return false;
    }
    return parseMessageTnc2(
        reinterpret_cast<const std::uint8_t*>(tnc2),
        std::strlen(tnc2),
        message);
}

bool buildMessageTnc2(
    const char* callsign,
    const char* destination,
    const char* addressee,
    const char* text,
    const char* messageId,
    char* output,
    std::size_t outputCapacity) {

    if (callsign == nullptr || callsign[0] == '\0' ||
        destination == nullptr || destination[0] == '\0' ||
        output == nullptr || outputCapacity == 0 ||
        !messageTextValid(text)) {
        return false;
    }

    char normalizedAddress[MAX_MESSAGE_ADDRESS_LENGTH + 1] = {};
    if (!normalizeMessageAddress(addressee, normalizedAddress, sizeof(normalizedAddress))) {
        return false;
    }

    const bool includeId = messageId != nullptr && messageId[0] != '\0';
    if (includeId && !messageIdValid(messageId)) {
        return false;
    }

    const int written = std::snprintf(
        output,
        outputCapacity,
        "%s>%s::%-9s:%s%s%s",
        callsign,
        destination,
        normalizedAddress,
        text,
        includeId ? "{" : "",
        includeId ? messageId : "");
    return written > 0 && static_cast<std::size_t>(written) < outputCapacity;
}

bool buildMessageResponseTnc2(
    const char* callsign,
    const char* destination,
    const char* addressee,
    const char* messageId,
    bool accepted,
    char* output,
    std::size_t outputCapacity) {

    if (callsign == nullptr || callsign[0] == '\0' ||
        destination == nullptr || destination[0] == '\0' ||
        output == nullptr || outputCapacity == 0 ||
        !messageIdValid(messageId)) {
        return false;
    }

    char normalizedAddress[MAX_MESSAGE_ADDRESS_LENGTH + 1] = {};
    if (!normalizeMessageAddress(addressee, normalizedAddress, sizeof(normalizedAddress))) {
        return false;
    }

    const int written = std::snprintf(
        output,
        outputCapacity,
        "%s>%s::%-9s:%s%s",
        callsign,
        destination,
        normalizedAddress,
        accepted ? "ack" : "rej",
        messageId);
    return written > 0 && static_cast<std::size_t>(written) < outputCapacity;
}

}  // namespace Aprs

namespace Aprs {
namespace {

struct Tnc2HeaderParts {
    std::size_t greater = 0;
    std::size_t colon = 0;
    std::size_t destinationEnd = 0;
    bool valid = false;
};

struct TokenSpan {
    std::size_t begin = 0;
    std::size_t end = 0;
};

bool parseTnc2HeaderParts(
    const std::uint8_t* frame,
    std::size_t length,
    Tnc2HeaderParts& parts) {

    parts = Tnc2HeaderParts{};
    if (frame == nullptr || length < 5) {
        return false;
    }

    const std::uint8_t* greaterPtr = findByte(frame, length, '>');
    const std::uint8_t* colonPtr = findByte(frame, length, ':');
    if (greaterPtr == nullptr || colonPtr == nullptr || greaterPtr == frame ||
        greaterPtr >= colonPtr || colonPtr + 1 > frame + length) {
        return false;
    }

    parts.greater = static_cast<std::size_t>(greaterPtr - frame);
    parts.colon = static_cast<std::size_t>(colonPtr - frame);
    parts.destinationEnd = parts.colon;
    for (std::size_t index = parts.greater + 1; index < parts.colon; ++index) {
        if (frame[index] == ',') {
            parts.destinationEnd = index;
            break;
        }
    }
    if (parts.destinationEnd <= parts.greater + 1) {
        return false;
    }

    // TNC2 headers are printable ASCII and may not contain whitespace.
    for (std::size_t index = 0; index < parts.colon; ++index) {
        const std::uint8_t value = frame[index];
        if (value < 33 || value > 126) {
            return false;
        }
    }
    parts.valid = true;
    return true;
}

char upperAscii(char value) {
    return value >= 'a' && value <= 'z'
        ? static_cast<char>(value - 'a' + 'A')
        : value;
}

bool spanEqualsText(
    const std::uint8_t* frame,
    const TokenSpan& span,
    const char* text,
    bool ignoreTrailingStar = true) {

    if (frame == nullptr || text == nullptr || span.end < span.begin) {
        return false;
    }
    std::size_t end = span.end;
    if (ignoreTrailingStar && end > span.begin && frame[end - 1] == '*') {
        --end;
    }
    const std::size_t length = end - span.begin;
    if (std::strlen(text) != length) {
        return false;
    }
    for (std::size_t index = 0; index < length; ++index) {
        if (upperAscii(static_cast<char>(frame[span.begin + index])) !=
            upperAscii(text[index])) {
            return false;
        }
    }
    return true;
}

bool spanStartsWithQ(const std::uint8_t* frame, const TokenSpan& span) {
    std::size_t end = span.end;
    if (end > span.begin && frame[end - 1] == '*') {
        --end;
    }
    return end - span.begin >= 2 &&
        upperAscii(static_cast<char>(frame[span.begin])) == 'Q' &&
        upperAscii(static_cast<char>(frame[span.begin + 1])) == 'A';
}

bool spanUsed(const std::uint8_t* frame, const TokenSpan& span) {
    return span.end > span.begin && frame[span.end - 1] == '*';
}

std::size_t collectPathTokens(
    const std::uint8_t* frame,
    const Tnc2HeaderParts& parts,
    TokenSpan* tokens,
    std::size_t capacity) {

    if (parts.destinationEnd >= parts.colon || tokens == nullptr || capacity == 0) {
        return 0;
    }
    std::size_t count = 0;
    std::size_t begin = parts.destinationEnd + 1;
    while (begin < parts.colon && count < capacity) {
        std::size_t end = begin;
        while (end < parts.colon && frame[end] != ',') {
            ++end;
        }
        if (end == begin) {
            return 0;
        }
        tokens[count++] = TokenSpan{begin, end};
        begin = end + 1;
    }
    return count;
}

bool sourceEqualsCall(
    const std::uint8_t* frame,
    const Tnc2HeaderParts& parts,
    const char* callsign) {

    return spanEqualsText(frame, TokenSpan{0, parts.greater}, callsign, false);
}

bool parseWideToken(
    const std::uint8_t* frame,
    const TokenSpan& span,
    int wideNumber,
    int& remaining) {

    remaining = 0;
    std::size_t end = span.end;
    if (end > span.begin && frame[end - 1] == '*') {
        --end;
    }
    if (end - span.begin != 7) {
        return false;
    }
    const char expected[] = {'W', 'I', 'D', 'E', static_cast<char>('0' + wideNumber), '-', '\0'};
    for (std::size_t index = 0; index < 6; ++index) {
        if (upperAscii(static_cast<char>(frame[span.begin + index])) != expected[index]) {
            return false;
        }
    }
    const std::uint8_t digit = frame[span.begin + 6];
    if (digit < '1' || digit > '7') {
        return false;
    }
    remaining = static_cast<int>(digit - '0');
    return true;
}

bool copyBytes(
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& position,
    const void* data,
    std::size_t length) {

    if (data == nullptr || position + length > outputCapacity) {
        return false;
    }
    std::memcpy(output + position, data, length);
    position += length;
    return true;
}

bool appendText(
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& position,
    const char* text) {

    return text != nullptr && copyBytes(
        output, outputCapacity, position, text, std::strlen(text));
}

bool headerContainsInternetOrNoGate(
    const std::uint8_t* frame,
    const Tnc2HeaderParts& parts,
    bool includeNoGate) {

    TokenSpan tokens[12] = {};
    const std::size_t count = collectPathTokens(frame, parts, tokens, 12);
    for (std::size_t index = 0; index < count; ++index) {
        const TokenSpan& token = tokens[index];
        if (spanEqualsText(frame, token, "TCPIP") ||
            spanEqualsText(frame, token, "TCPXX") ||
            spanStartsWithQ(frame, token) ||
            (includeNoGate && (spanEqualsText(frame, token, "NOGATE") ||
                               spanEqualsText(frame, token, "RFONLY")))) {
            return true;
        }
    }
    return false;
}

bool containsLineBreakingByte(const std::uint8_t* frame, std::size_t length) {
    for (std::size_t index = 0; index < length; ++index) {
        if (frame[index] == '\r' || frame[index] == '\n' || frame[index] == 0) {
            return true;
        }
    }
    return false;
}

std::uint32_t fnv1aUpdate(std::uint32_t hash, const std::uint8_t* data, std::size_t length) {
    constexpr std::uint32_t FNV_PRIME = 16777619U;
    for (std::size_t index = 0; index < length; ++index) {
        hash ^= data[index];
        hash *= FNV_PRIME;
    }
    return hash;
}

}  // namespace

bool buildDigipeatedTnc2(
    const std::uint8_t* tnc2,
    std::size_t length,
    const char* ownCallsign,
    const DigipeaterOptions& options,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength) {

    outputLength = 0;
    if (tnc2 == nullptr || ownCallsign == nullptr || ownCallsign[0] == '\0' ||
        output == nullptr || outputCapacity == 0 ||
        options.maxWideHops < 1 || options.maxWideHops > 2) {
        return false;
    }

    Tnc2HeaderParts parts;
    if (!parseTnc2HeaderParts(tnc2, length, parts) ||
        sourceEqualsCall(tnc2, parts, ownCallsign) ||
        headerContainsInternetOrNoGate(tnc2, parts, false)) {
        return false;
    }

    TokenSpan tokens[12] = {};
    const std::size_t tokenCount = collectPathTokens(tnc2, parts, tokens, 12);
    if (tokenCount == 0) {
        return false;
    }

    // Never repeat a frame that has already been used by this digipeater.
    // An unused directed MYCALL path is valid and must remain eligible.
    for (std::size_t index = 0; index < tokenCount; ++index) {
        if (spanUsed(tnc2, tokens[index]) &&
            spanEqualsText(tnc2, tokens[index], ownCallsign)) {
            return false;
        }
    }

    std::size_t candidate = tokenCount;
    for (std::size_t index = 0; index < tokenCount; ++index) {
        if (!spanUsed(tnc2, tokens[index])) {
            candidate = index;
            break;
        }
    }
    if (candidate == tokenCount) {
        return false;
    }

    enum class Action { None, Directed, Wide1, Wide2 };
    Action action = Action::None;
    int remaining = 0;
    if (spanEqualsText(tnc2, tokens[candidate], ownCallsign)) {
        action = Action::Directed;
    } else if (options.fillInWide1 &&
               parseWideToken(tnc2, tokens[candidate], 1, remaining) && remaining == 1) {
        action = Action::Wide1;
    } else if (options.traceWide2 &&
               parseWideToken(tnc2, tokens[candidate], 2, remaining) &&
               remaining <= static_cast<int>(options.maxWideHops)) {
        action = Action::Wide2;
    }
    if (action == Action::None) {
        return false;
    }

    std::size_t position = 0;
    if (!copyBytes(output, outputCapacity, position, tnc2, parts.destinationEnd)) {
        return false;
    }
    for (std::size_t index = 0; index < tokenCount; ++index) {
        const std::uint8_t comma = ',';
        if (!copyBytes(output, outputCapacity, position, &comma, 1)) {
            return false;
        }
        if (index == candidate) {
            if (!appendText(output, outputCapacity, position, ownCallsign) ||
                !appendText(output, outputCapacity, position, "*")) {
                return false;
            }
            if (action == Action::Wide2 && remaining > 1) {
                char decremented[] = ",WIDE2-1";
                decremented[7] = static_cast<char>('0' + remaining - 1);
                if (!appendText(output, outputCapacity, position, decremented)) {
                    return false;
                }
            }
        } else if (!copyBytes(
                       output,
                       outputCapacity,
                       position,
                       tnc2 + tokens[index].begin,
                       tokens[index].end - tokens[index].begin)) {
            return false;
        }
    }
    if (!copyBytes(
            output,
            outputCapacity,
            position,
            tnc2 + parts.colon,
            length - parts.colon)) {
        return false;
    }
    outputLength = position;
    return true;
}

bool buildReceiveOnlyIgateTnc2(
    const std::uint8_t* tnc2,
    std::size_t length,
    const char* igateCallsign,
    std::uint8_t* output,
    std::size_t outputCapacity,
    std::size_t& outputLength) {

    outputLength = 0;
    if (tnc2 == nullptr || igateCallsign == nullptr || igateCallsign[0] == '\0' ||
        output == nullptr || outputCapacity == 0 || containsLineBreakingByte(tnc2, length)) {
        return false;
    }

    const std::uint8_t* candidate = tnc2;
    std::size_t candidateLength = length;
    Tnc2HeaderParts outer;
    if (!parseTnc2HeaderParts(candidate, candidateLength, outer) ||
        headerContainsInternetOrNoGate(candidate, outer, true)) {
        return false;
    }

    if (outer.colon + 1 < candidateLength && candidate[outer.colon + 1] == '}') {
        const std::uint8_t* inner = candidate + outer.colon + 2;
        const std::size_t innerLength = candidateLength - outer.colon - 2;
        Tnc2HeaderParts innerParts;
        if (!parseTnc2HeaderParts(inner, innerLength, innerParts) ||
            headerContainsInternetOrNoGate(inner, innerParts, false)) {
            return false;
        }
        candidate = inner;
        candidateLength = innerLength;
    }

    Tnc2HeaderParts parts;
    if (!parseTnc2HeaderParts(candidate, candidateLength, parts) ||
        headerContainsInternetOrNoGate(candidate, parts, true) ||
        parts.colon + 1 >= candidateLength || candidate[parts.colon + 1] == '?') {
        return false;
    }

    std::size_t position = 0;
    if (!copyBytes(output, outputCapacity, position, candidate, parts.colon) ||
        !appendText(output, outputCapacity, position, ",qAO,") ||
        !appendText(output, outputCapacity, position, igateCallsign) ||
        !copyBytes(
            output,
            outputCapacity,
            position,
            candidate + parts.colon,
            candidateLength - parts.colon)) {
        return false;
    }
    outputLength = position;
    return true;
}

std::uint32_t tnc2PacketHash(const std::uint8_t* tnc2, std::size_t length) {
    Tnc2HeaderParts parts;
    if (!parseTnc2HeaderParts(tnc2, length, parts)) {
        return 0;
    }
    constexpr std::uint32_t FNV_OFFSET = 2166136261U;
    std::uint32_t hash = FNV_OFFSET;
    hash = fnv1aUpdate(hash, tnc2, parts.greater);
    const std::uint8_t greater = '>';
    hash = fnv1aUpdate(hash, &greater, 1);
    hash = fnv1aUpdate(
        hash,
        tnc2 + parts.greater + 1,
        parts.destinationEnd - parts.greater - 1);
    hash = fnv1aUpdate(
        hash,
        tnc2 + parts.colon,
        length - parts.colon);
    return hash;
}

}  // namespace Aprs
