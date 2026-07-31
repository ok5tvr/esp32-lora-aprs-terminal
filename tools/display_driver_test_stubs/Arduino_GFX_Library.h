#pragma once
#include <cstdint>
constexpr std::uint16_t RGB565_BLACK=0;
class Arduino_ESP32SPI {
public:
    Arduino_ESP32SPI(int,int,int,int,int,int,bool) {}
};
class Arduino_ST7796 {
public:
    Arduino_ST7796(Arduino_ESP32SPI*,int,int,bool,int,int) {}
    bool begin(){return true;}
    void setRotation(int){}
    void fillScreen(std::uint16_t){}
    int width() const{return 480;}
    int height() const{return 320;}
    void draw16bitBeRGBBitmap(int,int,std::uint16_t*,std::uint16_t,std::uint16_t){}
    void draw16bitRGBBitmap(int,int,std::uint16_t*,std::uint16_t,std::uint16_t){}
};
