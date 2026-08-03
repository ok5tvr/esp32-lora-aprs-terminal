#pragma once

#include <WebServer.h>
#include <cstddef>
#include <cstdint>

#include "services/ota_state.h"
#include "services/ota_validation.h"
#include "services/settings_service.h"

namespace Services {

class OtaService {
public:
    using ViewState = OtaViewState;

    bool begin();
    void update(std::uint32_t now, const SettingsService::ViewState& settings);
    const ViewState& viewState() const;

private:
    void configureRoutes();
    bool startAccessPoint(bool keepStation);
    void stopAccessPoint(bool keepStation, const char* status = nullptr);
    void sendIndexPage();
    void sendStatusJson();
    void handleStopRequest();
    void handleUploadData();
    void handleUploadFinished();
    bool writeFirmwareBytes(std::uint8_t* data, std::size_t length);
    void failUpload(const char* message);
    void setStatus(const char* text);
    bool apModeActive() const;
    std::uint32_t maximumFirmwareSize() const;

    WebServer server_{80};
    ViewState view_;
    bool routesConfigured_ = false;
    bool serverStarted_ = false;
    bool desiredEnabled_ = false;
    bool previousDesiredEnabled_ = false;
    bool uploadFailed_ = false;
    bool uploadSucceeded_ = false;
    bool headerValidated_ = false;
    bool manualStopLatched_ = false;
    bool stopPending_ = false;
    std::size_t headerBytes_ = 0;
    std::uint8_t headerBuffer_[OtaValidation::ESP32_APP_HEADER_BYTES] = {};
    std::uint32_t restartAt_ = 0;
    std::uint32_t stopAt_ = 0;
    std::uint32_t lastStartAttemptAt_ = 0;
    char uploadError_[128] = {};
};

}  // namespace Services
