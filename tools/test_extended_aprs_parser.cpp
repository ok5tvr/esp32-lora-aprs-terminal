#include <aprs_codec.h>
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 Aprs::ParsedFrame f;
 assert(Aprs::parseTnc2("N0CALL>APRS:;145.500  *111111z4903.50N/07201.75WrT100-060 PHG5130",f));
 assert(f.type==Aprs::EntityType::Object && f.hasPosition && f.frequency.valid && std::fabs(f.frequency.frequencyMHz-145.5f)<0.01f && f.phg.valid);
 assert(Aprs::parseTnc2("WX1>APRS:!4903.50N/07201.75W_180/010g020t068r001p002P003h55b10132",f));
 assert(f.weather.valid && f.weather.hasTemperature && f.weather.hasPressure);
 assert(Aprs::parseTnc2("TEL1>APRS:T#123,001,002,003,004,005,10101010,ok",f));
 assert(f.telemetry.valid && f.telemetry.hasSequence && f.telemetry.sequence==123 && f.telemetry.hasDigital);
 assert(Aprs::parseTnc2("OBJ1>APRS:)POINT!4903.50N/07201.75W>!EMERGENCY!",f));
 assert(f.type==Aprs::EntityType::Item && f.alertLevel==Aprs::AlertLevel::Emergency);
 assert(Aprs::parseTnc2("CMP1>APRS:!/5L!!<*e7>7P[",f));
 assert(f.positionEncoding==Aprs::PositionEncoding::Compressed || !f.hasPosition);
 std::puts("extended APRS parser tests passed");
}
