#pragma once

#include <cstddef>
#include <cstdint>

#include "drivers/sx1278_driver.h"
#include "services/digi_igate_service.h"
#include "services/message_store.h"
#include "services/station_store.h"
#include "services/weather_store.h"

namespace Services {

class RadioService {
public:
    struct ViewState {
        bool initialized = false;
        bool receiving = false;
        bool transmitting = false;
        bool lastPacketHadOeHeader = false;
        std::int16_t lastError = 0;
        std::uint32_t receivedPackets = 0;
        std::uint32_t transmittedPackets = 0;
        std::uint32_t receiveErrors = 0;
        float lastRssiDbm = 0.0F;
        float lastSnrDb = 0.0F;
        float lastFrequencyErrorHz = 0.0F;
        char lastPacketText[256] = "Cekam na prvni paket...";
    };

    bool begin();
    void update(
        std::uint32_t now,
        const SettingsService::ViewState& settings);
    bool sendTestPacket(const char* callsign, std::uint32_t now);
    bool sendTnc2(const char* frame, std::uint32_t now);
    bool queueMessage(
        const char* recipient,
        const char* text,
        std::uint32_t now,
        char* errorText,
        std::size_t errorTextCapacity);
    const ViewState& viewState() const;
    const StationStore::ViewState& stationViewState() const;
    const WeatherStore::ViewState& weatherViewState() const;
    const MessageStore::ViewState& messageViewState() const;
    const DigiIgateService::ViewState& digiIgateViewState() const;

private:
    void refreshDriverStatus();
    void serviceMessageTransmit(std::uint32_t now, const char* ownCallsign);
    void serviceDigiTransmit(std::uint32_t now);
    bool sendTnc2Bytes(
        const std::uint8_t* frame,
        std::size_t frameLength,
        std::uint32_t now);

    Drivers::Sx1278Driver driver_;
    StationStore stationStore_;
    WeatherStore weatherStore_;
    MessageStore messageStore_;
    DigiIgateService digiIgate_;
    ViewState view_;
};

}  // namespace Services
