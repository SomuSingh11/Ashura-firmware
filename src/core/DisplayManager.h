#pragma once
#include "pins.h"
#include <U8g2lib.h>
#include <Wire.h>
#include <Arduino.h>

// ============================================
// DisplayManager - Low-level OLED driver wrapper
// Owns the U8G2 object. All drawing goes through here.
// Screens call these methods; they never touch U8G2 directly.
// ============================================

class DisplayManager {
    public:
        void init() {
            Wire.begin(OLED_SDA, OLED_SCL);
            _display.begin();
            _display.setContrast(255);
            clear();
            sendBuffer();
            Serial.println("[Display] ✅ SSD1306 ready");
        };

        // ===== Buffer Control =====
        void clear(){ _display.clearBuffer(); };
        void sendBuffer(){ _display.sendBuffer(); };

        // ===== Font Helpers =====
        void setFontLarge(){ _display.setFont(u8g2_font_5x7_tr); }
        void setFontMedium(){ _display.setFont(u8g2_font_6x10_tr); }
        void setFontSmall(){ _display.setFont(u8g2_font_10x20_tr); }

        // ===== Drawing Primitives =====
        void drawStr(int x, int y, const char* str){
            _display.drawStr(x, y, str);
        };
        void drawLine(int x1, int x2, int y1, int y2) {
            _display.drawLine(x1, y1, x2, y2);
        };
        void drawRect(int x, int y, int w, int h) {
            _display.drawFrame(x, y, w, h);
        };
        void drawFilledRect(int x, int y, int w, int h) {
            _display.drawBox(x, y, w, h);
        };
        void drawCircle(int x, int y, int r) {
            _display.drawCircle(x, y, r);
        };
        void drawPixel(int x, int y) {
            _display.drawPixel(x, y);
        };

        // ===== Dimensions =====
        int getWidth() const {return OLED_WIDTH;};
        int getHeight() const {return OLED_HEIGHT;};

        // Expose raw U8G2 for advanced use (if needed) {escape hatch pattern}
        U8G2& raw () { return _display; }

    private:
        U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display{U8G2_R0, U8X8_PIN_NONE};
};