#include "DisplayManager.h"
#include "config.h"

void DisplayManager::init(){
    Wire.begin(OLED_SDA, OLED_SCL);

    if(!_display.begin()){
        Serial.println("OLED initialization failed!");
        return;
    }

    // showBootStatus("Ashura Core", "Initializing...");
    Serial.println("Display initialized");
};

void DisplayManager::clear() {
    _display.clearBuffer();
}

void DisplayManager::sendBuffer() {
    _display.sendBuffer();
}

void DisplayManager::setFontLarge() {
    _display.setFont(u8g2_font_ncenB08_tr);
}

void DisplayManager::setFontMedium() {
    _display.setFont(u8g2_font_6x10_tr);
}

void DisplayManager::setFontSmall() {
    _display.setFont(u8g2_font_5x7_tr);
}

void DisplayManager::drawStr(int x, int y, const char* str) {
    _display.drawStr(x, y, str);
}

void DisplayManager::drawLine(int x1, int x2, int y1, int y2) {
    _display.drawLine(x1, y1, x2, y2);
}

void DisplayManager::drawBitmap(int x, int y, int w, int h, const uint8_t* bitmap) {
    _display.drawXBMP(x, y, w, h, bitmap);
}

void DisplayManager::showBootStatus(const char* line1, const char* line2, const char* line3) {
    clear();
    setFontLarge();
    drawStr(0, 15, line1);

    if(strlen(line2) > 0) {
        setFontMedium();
        drawStr(0, 35, line2);
    }
    if(strlen(line3) > 0) {
        drawStr(0, 50, line3);
    }
    sendBuffer();
}