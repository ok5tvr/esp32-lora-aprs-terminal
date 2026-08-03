#include "services/ota_service.h"

#include <Arduino.h>
#include <Update.h>
#include <WiFi.h>
#include <algorithm>
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

String jsonEscape(const char* text) {
    String escaped;
    if (text == nullptr) {
        return escaped;
    }
    escaped.reserve(std::strlen(text) + 8U);
    for (const char* current = text; *current != '\0'; ++current) {
        if (*current == '\\' || *current == '"') {
            escaped += '\\';
        }
        if (*current == '\n' || *current == '\r') {
            escaped += ' ';
        } else {
            escaped += *current;
        }
    }
    return escaped;
}

}  // namespace

bool OtaService::begin() {
    view_ = ViewState{};
    WiFi.persistent(false);
    configureRoutes();
    view_.maximumFirmwareBytes = maximumFirmwareSize();
    setStatus(App::Localization::text("OTA vypnuto", "OTA disabled"));
    return routesConfigured_;
}

void OtaService::update(
    std::uint32_t now,
    const SettingsService::ViewState& settings) {

    desiredEnabled_ = settings.otaEnabled;
    view_.enabled = desiredEnabled_;

    if (desiredEnabled_ && !previousDesiredEnabled_) {
        manualStopLatched_ = false;
        view_.manuallyStopped = false;
        lastStartAttemptAt_ = 0;
    } else if (!desiredEnabled_ && previousDesiredEnabled_) {
        manualStopLatched_ = false;
        view_.manuallyStopped = false;
        if (!serverStarted_) {
            setStatus(App::Localization::text("OTA vypnuto", "OTA disabled"));
        }
    }
    previousDesiredEnabled_ = desiredEnabled_;

    if (stopPending_ && timeReached(now, stopAt_)) {
        stopPending_ = false;
        manualStopLatched_ = true;
        view_.manuallyStopped = true;
        stopAccessPoint(
            settings.igateEnabled,
            App::Localization::text(
                "OTA zastaveno; vypnete a znovu zapnete OTA v Nastaveni.",
                "OTA stopped; toggle OTA off and on in Settings."));
    }

    if (desiredEnabled_ && !manualStopLatched_) {
        if (!serverStarted_ || !apModeActive()) {
            if (lastStartAttemptAt_ == 0U ||
                now - lastStartAttemptAt_ >= AppConfig::OTA_RETRY_INTERVAL_MS) {
                lastStartAttemptAt_ = now;
                startAccessPoint(settings.igateEnabled);
            }
        }
    } else if (!desiredEnabled_ && (serverStarted_ || apModeActive())) {
        stopAccessPoint(settings.igateEnabled);
    }

    if (serverStarted_) {
        server_.handleClient();
        const std::uint8_t clients = WiFi.softAPgetStationNum();
        if (clients != view_.connectedClients) {
            view_.connectedClients = clients;
            ++view_.revision;
        }
    } else if (view_.connectedClients != 0U) {
        view_.connectedClients = 0U;
        ++view_.revision;
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
    server_.on("/status", HTTP_GET, [this]() { sendStatusJson(); });
    server_.on("/stop", HTTP_POST, [this]() { handleStopRequest(); });
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
    manualStopLatched_ = false;
    view_.manuallyStopped = false;
    view_.accessPointActive = true;
    view_.maximumFirmwareBytes = maximumFirmwareSize();
    uploadFailed_ = false;
    uploadSucceeded_ = false;
    view_.uploadActive = false;
    view_.uploadedBytes = 0;
    setStatus(App::Localization::text(
        "OTA aktivni: 192.168.4.1",
        "OTA active: 192.168.4.1"));
    LOG_I(
        "OTA",
        "AP %s started at %s; max firmware %u bytes; station preserved=%u",
        AppConfig::OTA_AP_SSID,
        WiFi.softAPIP().toString().c_str(),
        static_cast<unsigned>(view_.maximumFirmwareBytes),
        keepStation ? 1U : 0U);
    return true;
}

void OtaService::stopAccessPoint(bool keepStation, const char* status) {
    if (view_.uploadActive && Update.isRunning()) {
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
    view_.connectedClients = 0;
    setStatus(status != nullptr
        ? status
        : App::Localization::text("OTA vypnuto", "OTA disabled"));
    LOG_I("OTA", "%s", "OTA access point stopped");
}

void OtaService::sendIndexPage() {
    String page;
    page.reserve(7600);
    page += F("<!doctype html><html lang='en'><head><meta charset='utf-8'>");
    page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    page += F("<title>LoRa APRS OTA</title><style>");
    page += F("body{font-family:system-ui,sans-serif;background:#0b1424;color:#f4f7ff;margin:0;padding:24px}");
    page += F("main{max-width:620px;margin:auto;background:#17243a;border:1px solid #31425f;border-radius:14px;padding:24px}");
    page += F("h1{font-size:1.55rem;margin-top:0;color:#56c7ff}p{line-height:1.5;color:#bdcae0}");
    page += F("code{color:#ffcf70}input,button{font:inherit;width:100%;box-sizing:border-box;margin-top:12px;padding:12px;border-radius:9px}");
    page += F("input{background:#0b1424;color:#fff;border:1px solid #526785}button{border:0;background:#2764d8;color:#fff;font-weight:700;cursor:pointer}");
    page += F("button.secondary{background:#35445c}button:disabled{opacity:.55}progress{width:100%;height:18px;margin-top:18px}.small{font-size:.9rem;color:#92a7c7}");
    page += F("#status{min-height:1.5em;color:#ffcf70}.ok{color:#42d392}.error{color:#ff6b6b}</style></head><body><main>");
    page += F("<h1>LoRa APRS Terminal - Web OTA</h1><p>Current firmware: <strong>");
    page += AppConfig::FIRMWARE_VERSION;
    page += F("</strong><br>Maximum accepted image: <strong id='maximum'>");
    page += String(view_.maximumFirmwareBytes / 1024U);
    page += F(" KB</strong></p>");
    page += F("<p>Select only the <code>firmware.bin</code> built for the Waveshare ESP32-Touch-LCD-3.5. The device validates the ESP32 application header before writing.</p>");
    page += F("<p class='small'>During upload, keep USB or battery power connected and remain on the LoRa-APRS-OTA Wi-Fi network.</p>");
    page += F("<form id='form'><input id='file' name='firmware' type='file' accept='.bin,application/octet-stream' required>");
    page += F("<button id='send' type='submit'>Upload firmware</button></form>");
    page += F("<progress id='progress' max='100' value='0'></progress><p id='status'>Ready.</p>");
    page += F("<button id='stop' class='secondary' type='button'>Stop OTA access point</button>");
    page += F("<script>const max=");
    page += String(view_.maximumFirmwareBytes);
    page += F(",f=document.getElementById('form'),i=document.getElementById('file'),b=document.getElementById('send'),p=document.getElementById('progress'),s=document.getElementById('status'),stop=document.getElementById('stop');");
    page += F("function msg(t,c=''){s.textContent=t;s.className=c}f.onsubmit=e=>{e.preventDefault();if(!i.files.length)return;const file=i.files[0];if(!file.name.toLowerCase().endsWith('.bin')){msg('Select a .bin firmware file.','error');return}if(file.size>max){msg('File is larger than the OTA application slot.','error');return}const d=new FormData();d.append('firmware',file);const x=new XMLHttpRequest();x.open('POST','/update');");
    page += F("x.upload.onprogress=e=>{if(e.lengthComputable)p.value=Math.round(e.loaded*100/e.total)};x.onload=()=>{msg(x.responseText,x.status===200?'ok':'error');if(x.status!==200)b.disabled=false};x.onerror=()=>{msg('Connection error. The running firmware remains unchanged.','error');b.disabled=false};");
    page += F("b.disabled=true;stop.disabled=true;msg('Uploading...');x.send(d)};");
    page += F("stop.onclick=async()=>{if(!confirm('Stop the OTA access point?'))return;stop.disabled=true;const r=await fetch('/stop',{method:'POST'});msg(await r.text(),r.ok?'ok':'error')};");
    page += F("setInterval(async()=>{try{const r=await fetch('/status',{cache:'no-store'});if(!r.ok)return;const j=await r.json();if(!j.uploadActive&&p.value===0)msg(j.status)}catch(e){}},1500);</script></main></body></html>");
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "text/html; charset=utf-8", page);
}

void OtaService::sendStatusJson() {
    String json;
    json.reserve(360);
    json += F("{\"version\":\"");
    json += AppConfig::FIRMWARE_VERSION;
    json += F("\",\"enabled\":");
    json += view_.enabled ? F("true") : F("false");
    json += F(",\"accessPointActive\":");
    json += view_.accessPointActive ? F("true") : F("false");
    json += F(",\"uploadActive\":");
    json += view_.uploadActive ? F("true") : F("false");
    json += F(",\"uploadedBytes\":");
    json += String(view_.uploadedBytes);
    json += F(",\"maximumFirmwareBytes\":");
    json += String(view_.maximumFirmwareBytes);
    json += F(",\"clients\":");
    json += String(view_.connectedClients);
    json += F(",\"status\":\"");
    json += jsonEscape(view_.statusText);
    json += F("\"}");
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json; charset=utf-8", json);
}

void OtaService::handleStopRequest() {
    if (view_.uploadActive) {
        server_.send(
            409,
            "text/plain; charset=utf-8",
            App::Localization::text(
                "OTA nelze zastavit behem nahravani.",
                "OTA cannot be stopped during upload."));
        return;
    }
    server_.send(
        200,
        "text/plain; charset=utf-8",
        App::Localization::text(
            "OTA pristupovy bod se zastavuje...",
            "OTA access point is stopping..."));
    stopPending_ = true;
    stopAt_ = millis() + 300U;
}

void OtaService::handleUploadData() {
    HTTPUpload& upload = server_.upload();

    if (upload.status == UPLOAD_FILE_START) {
        uploadFailed_ = false;
        uploadSucceeded_ = false;
        headerValidated_ = false;
        headerBytes_ = 0;
        std::memset(headerBuffer_, 0, sizeof(headerBuffer_));
        uploadError_[0] = '\0';
        view_.uploadActive = true;
        view_.uploadedBytes = 0;
        view_.maximumFirmwareBytes = maximumFirmwareSize();
        ++view_.revision;

        if (!OtaValidation::hasFirmwareBinExtension(upload.filename.c_str())) {
            failUpload(App::Localization::text(
                "Neplatny soubor: je vyzadovan firmware .bin.",
                "Invalid file: a .bin firmware is required."));
            return;
        }
        if (view_.maximumFirmwareBytes == 0U) {
            failUpload(App::Localization::text(
                "OTA oddil neni dostupny. Nahrajte spravnou tabulku oddilu pres USB.",
                "OTA partition unavailable. Flash the correct partition table over USB."));
            return;
        }
        if (!Update.begin(view_.maximumFirmwareBytes, U_FLASH)) {
            failUpload(Update.errorString());
            return;
        }
        setStatus(App::Localization::text(
            "OTA: firmware se nahrava...",
            "OTA: firmware upload in progress..."));
        LOG_I(
            "OTA",
            "Upload started: %s; slot %u bytes",
            upload.filename.c_str(),
            static_cast<unsigned>(view_.maximumFirmwareBytes));
        return;
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFailed_ || upload.currentSize == 0U) {
            return;
        }

        std::size_t offset = 0;
        if (!headerValidated_) {
            const std::size_t needed = sizeof(headerBuffer_) - headerBytes_;
            const std::size_t copyLength = std::min<std::size_t>(needed, upload.currentSize);
            std::memcpy(headerBuffer_ + headerBytes_, upload.buf, copyLength);
            headerBytes_ += copyLength;
            offset += copyLength;

            if (headerBytes_ == sizeof(headerBuffer_)) {
                if (!OtaValidation::hasValidEsp32AppHeader(
                        headerBuffer_, sizeof(headerBuffer_))) {
                    failUpload(App::Localization::text(
                        "Soubor neni platna aplikace ESP32 pro OTA.",
                        "The file is not a valid ESP32 OTA application image."));
                    return;
                }
                headerValidated_ = true;
                if (!writeFirmwareBytes(headerBuffer_, sizeof(headerBuffer_))) {
                    return;
                }
            }
        }

        if (headerValidated_ && offset < upload.currentSize) {
            writeFirmwareBytes(upload.buf + offset, upload.currentSize - offset);
        }
        return;
    }

    if (upload.status == UPLOAD_FILE_END) {
        view_.uploadActive = false;
        if (!uploadFailed_ && !headerValidated_) {
            failUpload(App::Localization::text(
                "Firmware je prilis kratky nebo nema platnou hlavicku.",
                "Firmware is too short or has no valid application header."));
        }
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
            "Nahravani bylo preruseno. Puvodni firmware zustava aktivni.",
            "Upload was aborted. The original firmware remains active."));
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
            "Aktualizace selhala. Puvodni firmware zustava aktivni.",
            "Update failed. The original firmware remains active.");
    server_.send(400, "text/plain; charset=utf-8", message);
}

bool OtaService::writeFirmwareBytes(
    std::uint8_t* data,
    std::size_t length) {

    if (data == nullptr || length == 0U || uploadFailed_) {
        return !uploadFailed_;
    }
    if (view_.uploadedBytes > view_.maximumFirmwareBytes ||
        length > view_.maximumFirmwareBytes - view_.uploadedBytes) {
        failUpload(App::Localization::text(
            "Firmware je vetsi nez OTA oddil.",
            "Firmware is larger than the OTA application slot."));
        return false;
    }
    if (Update.write(data, length) != length) {
        failUpload(Update.errorString());
        return false;
    }
    view_.uploadedBytes += static_cast<std::uint32_t>(length);
    return true;
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

std::uint32_t OtaService::maximumFirmwareSize() const {
    const std::uint32_t freeSketch = ESP.getFreeSketchSpace();
    if (freeSketch <= 0x1000U) {
        return 0U;
    }
    return (freeSketch - 0x1000U) & 0xFFFFF000U;
}

}  // namespace Services
