#pragma once
#include "Arduino.h"
#include <cstddef>
#include <cstdint>
#include <functional>
enum HTTPMethod { HTTP_GET, HTTP_POST };
enum UploadStatus { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
struct HTTPUpload {
    UploadStatus status=UPLOAD_FILE_START;
    String filename;
    std::uint8_t* buf=nullptr;
    std::size_t currentSize=0;
};
class WebServer {
public:
    explicit WebServer(int) {}
    template<class F> void on(const char*, HTTPMethod, F) {}
    template<class F, class U> void on(const char*, HTTPMethod, F, U) {}
    template<class F> void onNotFound(F) {}
    void begin() {}
    void stop() {}
    void handleClient() {}
    HTTPUpload& upload() { return upload_; }
    void send(int) {}
    void send(int, const char*, const char* = nullptr) {}
    void send(int, const char*, const String&) {}
    void sendHeader(const char*, const char*, bool=false) {}
private: HTTPUpload upload_;
};
