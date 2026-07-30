#include <cassert>
#include <cstring>

#define private public
#include "services/gps_service.h"
#undef private

int main() {
    Services::GpsService gps;
    const char* first = "$GNRMC,180001.00,A,4947.1800,N,01317.1000,E,0.20,15.0,290726,,,A*68\r\n";
    for (const char* p = first; *p != '\0'; ++p) {
        gps.processDiagnosticCharacter(*p, 1000U);
    }

    assert(std::strcmp(gps.view_.lastSentenceType, "GNRMC") == 0);
    assert(std::strcmp(
        gps.view_.lastNmeaSentence,
        "$GNRMC,180001.00,A,4947.1800,N,01317.1000,E,0.20,15.0,290726,,,A*68") == 0);
    assert(std::strchr(gps.view_.lastNmeaSentence, '\r') == nullptr);
    assert(std::strchr(gps.view_.lastNmeaSentence, '\n') == nullptr);

    const char* second = "$GPGGA,180002.00,4947.1800,N,01317.1000,E,1,08,0.9,330.0,M,45.0,M,,*5A\n";
    for (const char* p = second; *p != '\0'; ++p) {
        gps.processDiagnosticCharacter(*p, 2000U);
    }

    assert(std::strcmp(gps.view_.lastSentenceType, "GPGGA") == 0);
    assert(std::strcmp(
        gps.view_.lastNmeaSentence,
        "$GPGGA,180002.00,4947.1800,N,01317.1000,E,1,08,0.9,330.0,M,45.0,M,,*5A") == 0);

    // Embedded NUL/control bytes must not terminate the displayed C string.
    const char third[] = {'$', '\0', 'G', 'N', 'G', 'S', 'A', ',', 'A', ',', '3', '*', '0', '0', '\r'};
    for (char c : third) {
        gps.processDiagnosticCharacter(c, 3000U);
    }
    assert(std::strcmp(gps.view_.lastNmeaSentence, "$GNGSA,A,3*00") == 0);

    // A lone '$' must not replace the previously completed line.
    gps.processDiagnosticCharacter('$', 4000U);
    gps.processDiagnosticCharacter('\r', 4000U);
    assert(std::strcmp(gps.view_.lastNmeaSentence, "$GNGSA,A,3*00") == 0);
    return 0;
}
