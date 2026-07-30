#include <aprs_codec.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

static std::string asText(const std::uint8_t* data, std::size_t length) {
    return std::string(reinterpret_cast<const char*>(data), length);
}

int main() {
    std::uint8_t output[512] = {};
    std::size_t length = 0;

    Aprs::DigipeaterOptions fill;
    fill.fillInWide1 = true;
    fill.traceWide2 = false;
    fill.maxWideHops = 2;
    const char* one = "OK1ABC-7>APRS,WIDE1-1,WIDE2-1:!4900.00N/01300.00E>test";
    assert(Aprs::buildDigipeatedTnc2(
        reinterpret_cast<const std::uint8_t*>(one), std::strlen(one),
        "OK5TVR-17", fill, output, sizeof(output), length));
    assert(asText(output, length) ==
        "OK1ABC-7>APRS,OK5TVR-17*,WIDE2-1:!4900.00N/01300.00E>test");

    Aprs::DigipeaterOptions wide;
    wide.fillInWide1 = false;
    wide.traceWide2 = true;
    wide.maxWideHops = 2;
    const char* two = "OK2XYZ>APRS,WIDE2-2:>hello";
    assert(Aprs::buildDigipeatedTnc2(
        reinterpret_cast<const std::uint8_t*>(two), std::strlen(two),
        "OK5TVR-17", wide, output, sizeof(output), length));
    assert(asText(output, length) ==
        "OK2XYZ>APRS,OK5TVR-17*,WIDE2-1:>hello");

    const char* directed = "OK2XYZ>APRS,OK5TVR-17:>direct";
    assert(Aprs::buildDigipeatedTnc2(
        reinterpret_cast<const std::uint8_t*>(directed), std::strlen(directed),
        "OK5TVR-17", wide, output, sizeof(output), length));
    assert(asText(output, length) ==
        "OK2XYZ>APRS,OK5TVR-17*:>direct");

    const char* used = "OK2XYZ>APRS,OK5TVR-17*,WIDE2-1:>hello";
    assert(!Aprs::buildDigipeatedTnc2(
        reinterpret_cast<const std::uint8_t*>(used), std::strlen(used),
        "OK5TVR-17", wide, output, sizeof(output), length));

    const char* abusive = "OK2XYZ>APRS,WIDE2-3:>hello";
    assert(!Aprs::buildDigipeatedTnc2(
        reinterpret_cast<const std::uint8_t*>(abusive), std::strlen(abusive),
        "OK5TVR-17", wide, output, sizeof(output), length));

    const char* gate = "OK1ABC>APRS,WIDE1-1:>status";
    assert(Aprs::buildReceiveOnlyIgateTnc2(
        reinterpret_cast<const std::uint8_t*>(gate), std::strlen(gate),
        "OK5TVR-17", output, sizeof(output), length));
    assert(asText(output, length) ==
        "OK1ABC>APRS,WIDE1-1,qAO,OK5TVR-17:>status");

    std::string longGate = "OK1ABC>APRS:>" + std::string(235, 'A');
    assert(longGate.size() <= 252);
    assert(Aprs::buildReceiveOnlyIgateTnc2(
        reinterpret_cast<const std::uint8_t*>(longGate.data()), longGate.size(),
        "OK5TVR-17", output, sizeof(output), length));
    assert(length > 255 && length + 2 <= 512);

    const char* nogate = "OK1ABC>APRS,NOGATE:>status";
    assert(!Aprs::buildReceiveOnlyIgateTnc2(
        reinterpret_cast<const std::uint8_t*>(nogate), std::strlen(nogate),
        "OK5TVR-17", output, sizeof(output), length));

    const char* query = "OK1ABC>APRS:?APRS?";
    assert(!Aprs::buildReceiveOnlyIgateTnc2(
        reinterpret_cast<const std::uint8_t*>(query), std::strlen(query),
        "OK5TVR-17", output, sizeof(output), length));

    const char* thirdParty =
        "OK9GATE>APRS,WIDE1-1:}OK1ABC>APRS,WIDE2-1:>inner";
    assert(Aprs::buildReceiveOnlyIgateTnc2(
        reinterpret_cast<const std::uint8_t*>(thirdParty), std::strlen(thirdParty),
        "OK5TVR-17", output, sizeof(output), length));
    assert(asText(output, length) ==
        "OK1ABC>APRS,WIDE2-1,qAO,OK5TVR-17:>inner");

    const char* outerFromIs =
        "OK9GATE>APRS,TCPIP*:}OK1ABC>APRS,WIDE2-1:>internet";
    assert(!Aprs::buildReceiveOnlyIgateTnc2(
        reinterpret_cast<const std::uint8_t*>(outerFromIs), std::strlen(outerFromIs),
        "OK5TVR-17", output, sizeof(output), length));

    const char* fromIs =
        "OK9GATE>APRS:}OK1ABC>APRS,TCPIP,OK9GATE*:>internet";
    assert(!Aprs::buildReceiveOnlyIgateTnc2(
        reinterpret_cast<const std::uint8_t*>(fromIs), std::strlen(fromIs),
        "OK5TVR-17", output, sizeof(output), length));

    const char* h1 = "OK1ABC>APRS,WIDE1-1:>same";
    const auto hash1 = Aprs::tnc2PacketHash(
        reinterpret_cast<const std::uint8_t*>(h1), std::strlen(h1));
    const char* h2 = "OK1ABC>APRS,OTHER*:>same";
    const auto hash2 = Aprs::tnc2PacketHash(
        reinterpret_cast<const std::uint8_t*>(h2), std::strlen(h2));
    assert(hash1 == hash2);

    std::cout << "digi/igate core tests passed\n";
    return 0;
}
