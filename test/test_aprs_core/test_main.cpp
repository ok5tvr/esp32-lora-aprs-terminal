#include <unity.h>

#include <aprs_codec.h>
#include <cstddef>
#include <cstdint>
#include <cstring>

void test_encode_with_oe_header() {
    std::uint8_t output[64] = {};
    std::size_t length = 0;
    TEST_ASSERT_TRUE(Aprs::encodeTnc2("OK5TVR>APRS:>TEST", true, output, sizeof(output), length));
    TEST_ASSERT_EQUAL_UINT(3 + std::strlen("OK5TVR>APRS:>TEST"), length);
    TEST_ASSERT_EQUAL_HEX8(0x3C, output[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, output[1]);
    TEST_ASSERT_EQUAL_HEX8(0x01, output[2]);
}

void test_decode_with_oe_header() {
    const std::uint8_t packet[] = {0x3C, 0xFF, 0x01, 'A', 'P', 'R', 'S'};
    char text[16] = {};
    bool hadHeader = false;
    TEST_ASSERT_TRUE(Aprs::decodeText(packet, sizeof(packet), text, sizeof(text), hadHeader));
    TEST_ASSERT_TRUE(hadHeader);
    TEST_ASSERT_EQUAL_STRING("APRS", text);
}

void test_decode_raw_preserves_mice_binary() {
    const std::uint8_t packet[] = {0x3C, 0xFF, 0x01, 'A', ':', 0x1c, 0x1d, 'Z'};
    std::uint8_t raw[16] = {};
    std::size_t rawLength = 0;
    bool hadHeader = false;
    TEST_ASSERT_TRUE(Aprs::decodeRaw(
        packet, sizeof(packet), raw, sizeof(raw), rawLength, hadHeader));
    TEST_ASSERT_TRUE(hadHeader);
    TEST_ASSERT_EQUAL_UINT(5, rawLength);
    TEST_ASSERT_EQUAL_HEX8(0x1c, raw[2]);
    TEST_ASSERT_EQUAL_HEX8(0x1d, raw[3]);
}

void test_parse_uncompressed_position_and_symbol() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK5TVR-15>APRS,WIDE1-1:!4944.20N/01323.10E>LoRa APRS",
        frame));
    TEST_ASSERT_TRUE(frame.valid);
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_EQUAL_STRING("OK5TVR-15", frame.source);
    TEST_ASSERT_EQUAL_STRING("OK5TVR-15", frame.entityName);
    TEST_ASSERT_EQUAL_CHAR('/', frame.symbolTable);
    TEST_ASSERT_EQUAL_CHAR('>', frame.symbolCode);
    TEST_ASSERT_DOUBLE_WITHIN(0.00001, 49.7366667, frame.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.00001, 13.3850000, frame.longitude);
}

void test_parse_compressed_position() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK5TVR-15>APRS:!/5A+>Qpnj>compressed",
        frame));
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, 49.7366667, frame.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, 13.3850000, frame.longitude);
}

void test_parse_mice_position_and_symbol() {
    const std::uint8_t frameBytes[] = {
        'O','K','1','A','B','C','>','S','3','2','U','V','T',':',
        '`','(','_','f','n','"','O','j','/'};
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(frameBytes, sizeof(frameBytes), frame));
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_EQUAL(Aprs::EntityType::Station, frame.type);
    TEST_ASSERT_EQUAL_STRING("OK1ABC", frame.source);
    TEST_ASSERT_EQUAL_CHAR('/', frame.symbolTable);
    TEST_ASSERT_EQUAL_CHAR('j', frame.symbolCode);
    TEST_ASSERT_DOUBLE_WITHIN(0.00001, 33.4273333, frame.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.00001, -112.1290000, frame.longitude);
}

void test_parse_live_object() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK1ABC>APRS:;LEADER   *092345z4903.50N/07201.75W>088/036",
        frame));
    TEST_ASSERT_EQUAL(Aprs::EntityType::Object, frame.type);
    TEST_ASSERT_TRUE(frame.alive);
    TEST_ASSERT_EQUAL_STRING("OK1ABC", frame.source);
    TEST_ASSERT_EQUAL_STRING("LEADER", frame.entityName);
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_EQUAL_CHAR('/', frame.symbolTable);
    TEST_ASSERT_EQUAL_CHAR('>', frame.symbolCode);
    TEST_ASSERT_DOUBLE_WITHIN(0.00001, 49.0583333, frame.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.00001, -72.0291667, frame.longitude);
}

void test_parse_killed_object() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK1ABC>APRS:;LEADER   _092345z4903.50N/07201.75W>",
        frame));
    TEST_ASSERT_EQUAL(Aprs::EntityType::Object, frame.type);
    TEST_ASSERT_FALSE(frame.alive);
    TEST_ASSERT_EQUAL_STRING("LEADER", frame.entityName);
}

void test_parse_live_item() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK2XYZ>APRS:)AID#2!4903.50N/07201.75WA",
        frame));
    TEST_ASSERT_EQUAL(Aprs::EntityType::Item, frame.type);
    TEST_ASSERT_TRUE(frame.alive);
    TEST_ASSERT_EQUAL_STRING("OK2XYZ", frame.source);
    TEST_ASSERT_EQUAL_STRING("AID#2", frame.entityName);
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_EQUAL_CHAR('/', frame.symbolTable);
    TEST_ASSERT_EQUAL_CHAR('A', frame.symbolCode);
}


void test_parse_compressed_object() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK1ABC>APRS:;LEADER   *092345z/5L!!<*e7>7P[",
        frame));
    TEST_ASSERT_EQUAL(Aprs::EntityType::Object, frame.type);
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_EQUAL_CHAR('/', frame.symbolTable);
    TEST_ASSERT_EQUAL_CHAR('>', frame.symbolCode);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, 49.5, frame.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, -72.75, frame.longitude);
}

void test_parse_compressed_item() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK2XYZ>APRS:)MOBIL!\\5L!!<*e79VsT",
        frame));
    TEST_ASSERT_EQUAL(Aprs::EntityType::Item, frame.type);
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_EQUAL_CHAR('\\', frame.symbolTable);
    TEST_ASSERT_EQUAL_CHAR('9', frame.symbolCode);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, 49.5, frame.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, -72.75, frame.longitude);
}


void test_parse_complete_weather_report() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK5WX>APRS:!4944.20N/01323.10E_270/010g020t050r004p010P015h82b10132",
        frame));
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_TRUE(frame.weather.valid);
    TEST_ASSERT_TRUE(frame.weather.hasTemperature);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 10.0F, frame.weather.temperatureC);
    TEST_ASSERT_TRUE(frame.weather.hasHumidity);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 82.0F, frame.weather.humidityPercent);
    TEST_ASSERT_TRUE(frame.weather.hasPressure);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 1013.2F, frame.weather.pressureHpa);
    TEST_ASSERT_TRUE(frame.weather.hasWindDirection);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 270.0F, frame.weather.windDirectionDegrees);
    TEST_ASSERT_TRUE(frame.weather.hasWindSpeed);
    TEST_ASSERT_FLOAT_WITHIN(0.02F, 18.52F, frame.weather.windSpeedKmh);
    TEST_ASSERT_TRUE(frame.weather.hasWindGust);
    TEST_ASSERT_FLOAT_WITHIN(0.02F, 32.18688F, frame.weather.windGustKmh);
    TEST_ASSERT_TRUE(frame.weather.hasRainLastHour);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 1.016F, frame.weather.rainLastHourMm);
}

void test_parse_positionless_weather_report() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK5WX-2>APRS:_07281430c180s010g020t068h50b10130",
        frame));
    TEST_ASSERT_FALSE(frame.hasPosition);
    TEST_ASSERT_TRUE(frame.weather.valid);
    TEST_ASSERT_TRUE(frame.weather.hasWindSpeed);
    TEST_ASSERT_FLOAT_WITHIN(0.02F, 16.09344F, frame.weather.windSpeedKmh);
    TEST_ASSERT_TRUE(frame.weather.hasTemperature);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 20.0F, frame.weather.temperatureC);
}

void test_third_party_frame_uses_inner_original_source() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "IGATE>APRS:}OK2AAA-5>APRS:!4900.00N/01400.00E>inner",
        frame));
    TEST_ASSERT_EQUAL_STRING("OK2AAA-5", frame.source);
    TEST_ASSERT_TRUE(frame.hasPosition);
}


void test_build_uncompressed_tracker_position() {
    char output[192] = {};
    TEST_ASSERT_TRUE(Aprs::buildPositionTnc2(
        "OK5TVR-15",
        "APRS",
        49.7366667,
        13.3850000,
        '/',
        '>',
        false,
        true,
        123.0,
        45.0,
        "LoRa tracker",
        output,
        sizeof(output)));
    TEST_ASSERT_EQUAL_STRING(
        "OK5TVR-15>APRS:!4944.20N/01323.10E>123/045 LoRa tracker",
        output);

    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(output, frame));
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, 49.7366667, frame.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, 13.3850000, frame.longitude);
}

void test_build_compressed_tracker_position() {
    char output[192] = {};
    TEST_ASSERT_TRUE(Aprs::buildPositionTnc2(
        "OK5TVR-15",
        "APRS",
        49.7366667,
        13.3850000,
        '/',
        '>',
        true,
        true,
        123.0,
        45.0,
        "LoRa tracker",
        output,
        sizeof(output)));

    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(output, frame));
    TEST_ASSERT_TRUE(frame.hasPosition);
    TEST_ASSERT_EQUAL_CHAR('/', frame.symbolTable);
    TEST_ASSERT_EQUAL_CHAR('>', frame.symbolCode);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, 49.7366667, frame.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(0.00002, 13.3850000, frame.longitude);
}

void test_build_tracker_rejects_invalid_position() {
    char output[64] = {};
    TEST_ASSERT_FALSE(Aprs::buildPositionTnc2(
        "OK5TVR-15", "APRS", 91.0, 13.0, '/', '>', false,
        false, 0.0, 0.0, "", output, sizeof(output)));
}


void test_build_and_parse_aprs_message() {
    char output[192] = {};
    TEST_ASSERT_TRUE(Aprs::buildMessageTnc2(
        "OK5TVR-15", "APRS", "OK1ABC-7", "Ahoj", "003",
        output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING(
        "OK5TVR-15>APRS::OK1ABC-7 :Ahoj{003",
        output);

    Aprs::ParsedMessage message;
    TEST_ASSERT_TRUE(Aprs::parseMessageTnc2(output, message));
    TEST_ASSERT_TRUE(message.valid);
    TEST_ASSERT_EQUAL(Aprs::MessageKind::Text, message.kind);
    TEST_ASSERT_EQUAL_STRING("OK5TVR-15", message.source);
    TEST_ASSERT_EQUAL_STRING("OK1ABC-7", message.addressee);
    TEST_ASSERT_EQUAL_STRING("Ahoj", message.text);
    TEST_ASSERT_TRUE(message.hasMessageId);
    TEST_ASSERT_EQUAL_STRING("003", message.messageId);
}

void test_build_and_parse_message_ack() {
    char output[192] = {};
    TEST_ASSERT_TRUE(Aprs::buildMessageResponseTnc2(
        "OK1ABC-7", "APRS", "OK5TVR-15", "003", true,
        output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING(
        "OK1ABC-7>APRS::OK5TVR-15:ack003",
        output);

    Aprs::ParsedMessage message;
    TEST_ASSERT_TRUE(Aprs::parseMessageTnc2(output, message));
    TEST_ASSERT_EQUAL(Aprs::MessageKind::Acknowledgement, message.kind);
    TEST_ASSERT_EQUAL_STRING("003", message.messageId);
}

void test_message_rejects_forbidden_character() {
    char output[192] = {};
    TEST_ASSERT_FALSE(Aprs::buildMessageTnc2(
        "OK5TVR-15", "APRS", "OK1ABC-7", "spatny{text", "001",
        output, sizeof(output)));
}

void test_parse_third_party_message() {
    Aprs::ParsedMessage message;
    TEST_ASSERT_TRUE(Aprs::parseMessageTnc2(
        "IGATE>APRS:}OK1ABC-7>APRS::OK5TVR-15:Ahoj{123",
        message));
    TEST_ASSERT_EQUAL_STRING("OK1ABC-7", message.source);
    TEST_ASSERT_EQUAL_STRING("OK5TVR-15", message.addressee);
    TEST_ASSERT_EQUAL_STRING("Ahoj", message.text);
}


void test_parse_extended_aprs_fields() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "WX1>APRS:!4903.50N/07201.75W_PHG5360 145.650MHz T088 +060 T#123,001,002,003,004,005,10101010 !EMERGENCY!",
        frame));
    TEST_ASSERT_EQUAL(Aprs::PositionFormat::Uncompressed, frame.positionFormat);
    TEST_ASSERT_TRUE(frame.phg.valid);
    TEST_ASSERT_EQUAL_UINT16(25, frame.phg.powerWatts);
    TEST_ASSERT_EQUAL_UINT32(80, frame.phg.heightFeet);
    TEST_ASSERT_TRUE(frame.frequency.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 145.650F, frame.frequency.frequencyMhz);
    TEST_ASSERT_TRUE(frame.frequency.hasTone);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 88.0F, frame.frequency.toneHz);
    TEST_ASSERT_TRUE(frame.frequency.hasOffset);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, 0.6F, frame.frequency.offsetMhz);
    TEST_ASSERT_TRUE(frame.telemetry.valid);
    TEST_ASSERT_EQUAL_UINT16(123, frame.telemetry.sequence);
    TEST_ASSERT_EQUAL_UINT16(5, frame.telemetry.analog[4]);
    TEST_ASSERT_TRUE(frame.telemetry.digitalValid);
    TEST_ASSERT_TRUE(frame.telemetry.digital[0]);
    TEST_ASSERT_TRUE(frame.emergency);
}

void test_frequency_object() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK1ABC>APRS:;145.650-R*092345z4903.50N/07201.75Wr145.650MHz T088 -060",
        frame));
    TEST_ASSERT_EQUAL(Aprs::EntityType::Object, frame.type);
    TEST_ASSERT_TRUE(frame.frequency.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001F, 145.650F, frame.frequency.frequencyMhz);
    TEST_ASSERT_FLOAT_WITHIN(0.01F, -0.6F, frame.frequency.offsetMhz);
}


void test_path_analysis_direct_packet() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK1ABC-7>APRS,WIDE1-1,WIDE2-1:!4900.00N/01400.00E>direct",
        frame));
    TEST_ASSERT_TRUE(frame.path.valid);
    TEST_ASSERT_TRUE(frame.path.direct);
    TEST_ASSERT_EQUAL_UINT8(0, frame.path.digipeaterHops);
    TEST_ASSERT_EQUAL_STRING("WIDE1-1,WIDE2-1", frame.path.path);
    TEST_ASSERT_EQUAL_STRING("", frame.path.lastDigipeater);
}

void test_path_analysis_repeated_packet() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK1ABC-7>APRS,OK0AAA-2*,WIDE2-1*,WIDE2-1:!4900.00N/01400.00E>via digi",
        frame));
    TEST_ASSERT_TRUE(frame.path.valid);
    TEST_ASSERT_FALSE(frame.path.direct);
    TEST_ASSERT_EQUAL_UINT8(2, frame.path.digipeaterHops);
    TEST_ASSERT_EQUAL_STRING("OK0AAA-2*,WIDE2-1*,WIDE2-1", frame.path.path);
    TEST_ASSERT_EQUAL_STRING("WIDE2-1", frame.path.lastDigipeater);
}

void test_path_analysis_ignores_internet_tokens() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "OK1ABC>APRS,TCPIP*,qAC,T2SERVER:!4900.00N/01400.00E>internet",
        frame));
    TEST_ASSERT_TRUE(frame.path.valid);
    TEST_ASSERT_TRUE(frame.path.direct);
    TEST_ASSERT_EQUAL_UINT8(0, frame.path.digipeaterHops);
}

void test_third_party_path_uses_inner_frame() {
    Aprs::ParsedFrame frame;
    TEST_ASSERT_TRUE(Aprs::parseTnc2(
        "IGATE>APRS,TCPIP*:}OK2AAA-5>APRS,OK0XYZ-2*,WIDE2-1:!4900.00N/01400.00E>inner",
        frame));
    TEST_ASSERT_EQUAL_STRING("OK2AAA-5", frame.source);
    TEST_ASSERT_FALSE(frame.path.direct);
    TEST_ASSERT_EQUAL_UINT8(1, frame.path.digipeaterHops);
    TEST_ASSERT_EQUAL_STRING("OK0XYZ-2*,WIDE2-1", frame.path.path);
    TEST_ASSERT_EQUAL_STRING("OK0XYZ-2", frame.path.lastDigipeater);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_encode_with_oe_header);
    RUN_TEST(test_decode_with_oe_header);
    RUN_TEST(test_decode_raw_preserves_mice_binary);
    RUN_TEST(test_parse_uncompressed_position_and_symbol);
    RUN_TEST(test_parse_compressed_position);
    RUN_TEST(test_parse_mice_position_and_symbol);
    RUN_TEST(test_parse_live_object);
    RUN_TEST(test_parse_killed_object);
    RUN_TEST(test_parse_live_item);
    RUN_TEST(test_parse_compressed_object);
    RUN_TEST(test_parse_compressed_item);
    RUN_TEST(test_parse_complete_weather_report);
    RUN_TEST(test_parse_positionless_weather_report);
    RUN_TEST(test_third_party_frame_uses_inner_original_source);
    RUN_TEST(test_build_uncompressed_tracker_position);
    RUN_TEST(test_build_compressed_tracker_position);
    RUN_TEST(test_build_tracker_rejects_invalid_position);
    RUN_TEST(test_build_and_parse_aprs_message);
    RUN_TEST(test_build_and_parse_message_ack);
    RUN_TEST(test_message_rejects_forbidden_character);
    RUN_TEST(test_parse_third_party_message);
    RUN_TEST(test_parse_extended_aprs_fields);
    RUN_TEST(test_frequency_object);
    RUN_TEST(test_path_analysis_direct_packet);
    RUN_TEST(test_path_analysis_repeated_packet);
    RUN_TEST(test_path_analysis_ignores_internet_tokens);
    RUN_TEST(test_third_party_path_uses_inner_frame);
    return UNITY_END();
}
