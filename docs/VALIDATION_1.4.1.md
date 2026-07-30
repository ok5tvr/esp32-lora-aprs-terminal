# Validation 1.4.1

- verified that `src/services/radio_service.cpp` contains no reference to the removed `ViewState::lastPacket` member
- verified that both `StationStore::ingest()` and `WeatherStore::ingest()` receive `view_.lastPacketText`
- verified firmware version string `1.4.1`
- no hardware pin, radio, GPS, tracker, parser, DIGI/iGate, power or Stopař behavior changed
