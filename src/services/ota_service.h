#pragma once

#include <WebServer.h>
#include <cstddef>
#include <cstdint>

#include "services/settings_service.h"

namespace Services {

class OtaService {
public:
    struct ViewState {
        bool enabled = false;
        bool accessPointActive = false;
        bool uploadActive = false;
        bool restartPending = false;
        std::uint32_t uploadedBytes = 0;
        std::uint32_t revision = 0;
        char statusText[112] = "OTA vypnuto";
    };

    bool begin();
    void update(std::uint32_t now, const SettingsService::ViewState& settings);
    const ViewState& viewState() const;

private:
    void configureRoutes();
    bool startAccessPoint(bool keepStation);
    void stopAccessPoint(bool keepStation);
    void sendIndexPage();
    void handleUploadData();
    void handleUploadFinished();
    void failUpload(const char* message);
    void setStatus(const char* text);
    bool apModeActive() const;

    WebServer server_{80};
    ViewState view_;
    bool routesConfigured_ = false;
    bool serverStarted_ = false;
    bool desiredEnabled_ = false;
    bool uploadFailed_ = false;
    bool uploadSucceeded_ = false;
    bool firstUploadChunk_ = true;
    std::uint32_t restartAt_ = 0;
    std::uint32_t lastStartAttemptAt_ = 0;
    char uploadError_[128] = {};
};

}  // namespace Services
