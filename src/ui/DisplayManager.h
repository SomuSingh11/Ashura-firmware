#pragma once
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
        void init();

        void clear();
        void sendBuffer();
        void setFontLarge();
        void setFontMedium();
        void setFontSmall();
        void drawStr(int x, int y, const char* str);
        void drawLine(int x1, int x2, int y1, int y2);
        int getWidth() const {return 128;};
        int getHeight() const {return 64;};

        void showBootStatus(
            const char* line1,
            const char* line2 = "",
            const char* line3 = ""
        );

        // Boot/splash helper (used before UIManager is ready)
        U8G2& raw () { return _display; } // Expose raw U8G2 for advanced use (if needed) {escape hatch pattern}

    private:
        U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display{U8G2_R0, /* reset=*/ U8X8_PIN_NONE};
};