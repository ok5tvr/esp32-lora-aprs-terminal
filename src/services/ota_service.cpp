#include "services/ota_service.h"

#include <Arduino.h>
#include <Update.h>
#include <WiFi.h>
#include <cstdio>
#include <cstring>

#include "app_config.h"
#include "app/localization.h"
#include "app_log.h"

namespace Services {
namespace {

const IPAddress OTA_IP(192, 168, 4, 1);
const IPAddress OTA_GATEWAY(192, 168, 4, 1);
const IPAddress OTA_SUBNET(255, 255, 255, 0);

bool timeReached(std::uint32_t now, std::uint32_t target) {
    return static_cast<std::int32_t>(now - target) >= 0;
}

bool hasBinExtension(const String& filename) {
    String lower = filename;
    lower.toLowerCase();
    return lower.endsWith(".bin");
}

}  // namespace

bool OtaService::begin() {
    view_ = ViewState{};
    WiFi.persistent(false);
    configureRoutes();
    return routesConfigured_;
}

void OtaService::update(
    std::uint32_t now,
    const SettingsService::ViewState& settings) {

    desiredEnabled_ = settings.otaEnabled;
    view_.enabled = desiredEnabled_;

    if (desiredEnabled_) {
        if (!serverStarted_ || !apModeActive()) {
            if (lastStartAttemptAt_ == 0 ||
                now - lastStartAttemptAt_ >= AppConfig::OTA_RETRY_INTERVAL_MS) {
                lastStartAttemptAt_ = now;
                startAccessPoint(settings.igateEnabled);
            }
        }
    } else if (serverStarted_ || apModeActive()) {
        stopAccessPoint(settings.igateEnabled);
    }

    if (serverStarted_) {
        server_.handleClient();
    }

    if (view_.restartPending && timeReached(now, restartAt_)) {
        LOG_I("OTA", "%s", "Restarting after successful firmware update");
        delay(50);
        ESP.restart();
    }
}

const OtaService::ViewState& OtaService::viewState() const {
    return view_;
}

void OtaService::configureRoutes() {
    if (routesConfigured_) {
        return;
    }

    server_.on("/", HTTP_GET, [this]() { sendIndexPage(); });
    server_.on(
        "/update",
        HTTP_POST,
        [this]() { handleUploadFinished(); },
        [this]() { handleUploadData(); });
    server_.on("/favicon.ico", HTTP_GET, [this]() { server_.send(204); });
    server_.onNotFound([this]() {
        server_.sendHeader("Location", "/", true);
        server_.send(302, "text/plain", "");
    });
    routesConfigured_ = true;
}

bool OtaService::startAccessPoint(bool keepStation) {
    if (serverStarted_) {
        server_.stop();
        serverStarted_ = false;
    }

    const wifi_mode_t targetMode = keepStation ? WIFI_AP_STA : WIFI_AP;
    if (!WiFi.mode(targetMode)) {
        setStatus(App::Localization::text(
            "OTA: nelze nastavit rezim WiFi.",
            "OTA: cannot set Wi-Fi mode."));
        return false;
    }

    WiFi.softAPdisconnect(true);
    if (!WiFi.softAPConfig(OTA_IP, OTA_GATEWAY, OTA_SUBNET)) {
        setStatus(App::Localization::text(
            "OTA: nelze nastavit adresu 192.168.4.1.",
            "OTA: cannot configure 192.168.4.1."));
        return false;
    }
    if (!WiFi.softAP(
            AppConfig::OTA_AP_SSID,
            AppConfig::OTA_AP_PASSWORD,
            AppConfig::OTA_AP_CHANNEL,
            false,
            AppConfig::OTA_AP_MAX_CLIENTS)) {
        setStatus(App::Localization::text(
            "OTA: pristupovy bod se nespustil.",
            "OTA: access point failed to start."));
        return false;
    }

    server_.begin();
    serverStarted_ = true;
    view_.accessPointActive = true;
    uploadFailed_ = false;
    uploadSucceeded_ = false;
    view_.uploadActive = false;
    view_.uploadedBytes = 0;
    setStatus(App::Localization::text(
        "OTA aktivni: 192.168.4.1",
        "OTA active: 192.168.4.1"));
    LOG_I(
        "OTA",
        "AP %s started at %s; station preserved=%u",
        AppConfig::OTA_AP_SSID,
        WiFi.softAPIP().toString().c_str(),
        keepStation ? 1U : 0U);
    return true;
}

void OtaService::stopAccessPoint(bool keepStation) {
    if (view_.uploadActive) {
        Update.abort();
    }
    if (serverStarted_) {
        server_.stop();
    }
    serverStarted_ = false;
    WiFi.softAPdisconnect(true);
    WiFi.mode(keepStation ? WIFI_STA : WIFI_OFF);
    view_.accessPointActive = false;
    view_.uploadActive = false;
    view_.restartPending = false;
    view_.uploadedBytes = 0;
    setStatus(App::Localization::text("OTA vypnuto", "OTA disabled"));
    LOG_I("OTA", "%s", "OTA access point stopped");
}

void OtaService::sendIndexPage() {
    String page;
    page.reserve(5000);
    page += F("<!doctype html><html lang='cs'><head><meta charset='utf-8'>");
    page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    page += F("<title>LoRa APRS OTA</title><style>");
    page += F("body{font-family:system-ui,sans-serif;background:#0b1424;color:#f4f7ff;margin:0;padding:24px}");
    page += F("main{max-width:620px;margin:auto;background:#17243a;border:1px solid #31425f;border-radius:14px;padding:24px}");
    page += F("h1{font-size:1.55rem;margin-top:0;color:#56c7ff}p{line-height:1.5;color:#bdcae0}");
    page += F("input,button{font:inherit;width:100%;box-sizing:border-box;margin-top:12px;padding:12px;border-radius:9px}");
    page += F("input{background:#0b1424;color:#fff;border:1px solid #526785}button{border:0;background:#2764d8;color:#fff;font-weight:700}");
    page += F("button:disabled{opacity:.55}progress{width:100%;height:18px;margin-top:18px}.small{font-size:.9rem;color:#92a7c7}");
    page += F("#status{min-height:1.5em;color:#ffcf70}</style></head><body><main>");
    page += F("<h1>LoRa APRS Terminal - OTA</h1><p>Aktualni firmware / Current firmware: <strong>");
    page += AppConfig::FIRMWARE_VERSION;
    page += F("</strong></p><p>Vyberte pouze soubor <code>firmware.bin</code> vytvoreny pro tuto desku. Behem nahravani nevypinejte napajeni.</p>");
    page += F("<p class='small'>Select only the <code>firmware.bin</code> built for this board. Do not remove power during upload.</p>");
    page += F("<form id='form'><input id='file' name='firmware' type='file' accept='.bin,application/octet-stream' required>");
    page += F("<button id='send' type='submit'>Nahrat firmware / Upload firmware</button></form>");
    page += F("<progress id='progress' max='100' value='0'></progress><p id='status'></p>");
    page += F("<script>const f=document.getElementById('form'),i=document.getElementById('file'),b=document.getElementById('send'),p=document.getElementById('progress'),s=document.getElementById('status');");
    page += F("f.onsubmit=e=>{e.preventDefault();if(!i.files.length)return;const d=new FormData();d.append('firmware',i.files[0]);const x=new XMLHttpRequest();x.open('POST','/update');");
    page += F("x.upload.onprogress=e=>{if(e.lengthComputable)p.value=Math.round(e.loaded*100/e.total)};x.onload=()=>{s.textContent=x.responseText;b.disabled=false};x.onerror=()=>{s.textContent='Chyba spojeni / Connection error';b.disabled=false};");
    page += F("b.disabled=true;s.textContent='Nahravam / Uploading...';x.send(d)};</script></main></body></html>");
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "text/html; charset=utf-8", page);
}

void OtaService::handleUploadData() {
    HTTPUpload& upload = server_.upload();

    if (upload.status == UPLOAD_FILE_START) {
        uploadFailed_ = false;
        uploadSucceeded_ = false;
        firstUploadChunk_ = true;
        uploadError_[0] = '\0';
        view_.uploadActive = true;
        view_.uploadedBytes = 0;
        ++view_.revision;

        if (!hasBinExtension(upload.filename)) {
            failUpload(App::Localization::text(
                "Neplatny soubor: je vyzadovan firmware .bin.",
                "Invalid file: a .bin firmware is required."));
            return;
        }
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
            failUpload(Update.errorString());
            return;
        }
        setStatus(App::Localization::text(
            "OTA: firmware se nahrava...",
            "OTA: firmware upload in progress..."));
        LOG_I("OTA", "Upload started: %s", upload.filename.c_str());
        return;
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFailed_) {
            return;
        }
        if (firstUploadChunk_) {
            firstUploadChunk_ = false;
            if (upload.currentSize == 0 || upload.buf[0] != 0xE9) {
                failUpload(App::Localization::text(
                    "Soubor nema platnou hlavicku ESP32 firmware.",
                    "The file does not have a valid ESP32 firmware header."));
                return;
            }
        }
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            failUpload(Update.errorString());
            return;
        }
        view_.uploadedBytes += upload.currentSize;
        return;
    }

    if (upload.status == UPLOAD_FILE_END) {
        view_.uploadActive = false;
        if (!uploadFailed_ && Update.end(true)) {
            uploadSucceeded_ = true;
            setStatus(App::Localization::text(
                "OTA dokonceno; zarizeni se restartuje.",
                "OTA complete; device will restart."));
            LOG_I(
                "OTA",
                "Upload complete: %u bytes",
                static_cast<unsigned>(view_.uploadedBytes));
        } else if (!uploadFailed_) {
            failUpload(Update.errorString());
        }
        ++view_.revision;
        return;
    }

    if (upload.status == UPLOAD_FILE_ABORTED) {
        if (Update.isRunning()) {
            Update.abort();
        }
        failUpload(App::Localization::text(
            "Nahravani bylo preruseno.",
            "Upload was aborted."));
    }
}

void OtaService::handleUploadFinished() {
    server_.sendHeader("Connection", "close");
    server_.sendHeader("Cache-Control", "no-store");
    if (uploadSucceeded_ && !uploadFailed_) {
        server_.send(
            200,
            "text/plain; charset=utf-8",
            App::Localization::text(
                "Firmware byl nahran. Zarizeni se restartuje...",
                "Firmware uploaded. Device is restarting..."));
        view_.restartPending = true;
        restartAt_ = millis() + AppConfig::OTA_RESTART_DELAY_MS;
        ++view_.revision;
        return;
    }

    const char* message = uploadError_[0] != '\0'
        ? uploadError_
        : App::Localization::text(
            "Aktualizace selhala.",
            "Update failed.");
    server_.send(400, "text/plain; charset=utf-8", message);
}

void OtaService::failUpload(const char* message) {
    uploadFailed_ = true;
    uploadSucceeded_ = false;
    view_.uploadActive = false;
    if (Update.isRunning()) {
        Update.abort();
    }
    std::snprintf(
        uploadError_,
        sizeof(uploadError_),
        "%s",
        message != nullptr && message[0] != '\0'
            ? message
            : App::Localization::text("Neznama chyba OTA.", "Unknown OTA error."));
    setStatus(uploadError_);
    LOG_E("OTA", "%s", uploadError_);
}

void OtaService::setStatus(const char* text) {
    std::snprintf(
        view_.statusText,
        sizeof(view_.statusText),
        "%s",
        text != nullptr ? text : "");
    view_.accessPointActive = serverStarted_ && apModeActive();
    ++view_.revision;
}

bool OtaService::apModeActive() const {
    const wifi_mode_t mode = WiFi.getMode();
    return mode == WIFI_AP || mode == WIFI_AP_STA;
}

}  // namespace Services
