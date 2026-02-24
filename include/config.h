#pragma once

// ============================================
// WiFi Configuration
// ============================================
#define WIFI_SSID "Mr. ROBOT"
#define WIFI_PASSWORD "hyperspacex"

// ============================================
// WebSocket Server Configuration
// ============================================
#define WS_SERVER_HOST "192.168.31.70"  
#define WS_SERVER_PORT 3000
#define WS_SERVER_PATH "/ws"

// ============================================
// Device Configuration
// ============================================
#define DEVICE_ID "esp32-core-01"
#define DEVICE_NAME "Ashura Core ESP32"

// ============================================
// OLED Display Configuration (SSD1306 128x64)
// ============================================
#define OLED_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// ============================================
// UI Layout Configuration
// ============================================
#define UI_TITLE_Y 10
#define UI_TITLE_LINE_Y 12

#define UI_ROW_HEIGHT 12
#define UI_FIRST_ROW_Y 25

#define UI_MAX_VISIBLE_ITEMS 4
#define UI_HINT_PADDING 38

// ============================================
// Timing Configuration
// ============================================
#define HEARTBEAT_INTERVAL 10000  // 10 seconds
#define RECONNECT_DELAY 5000      // 5 seconds
#define SCREEN_TIMEOUT 30000      // 30 seconds
#define SCREENSAVER_TIMEOUT 20000 // 20 seconds
#define LASTMESSAGE_HOMESCREEN_TIMEOUT 10000 // 10 seconds
#define BUTTON_DEBOUNCE_MS 30
#define NTP_SYNC_INTERVAL 30000 // 30 seconds

// ============================================
// Application Configuration
// ============================================
#define CLOCKAPP_LIST_SIZE 1
#define GAMESAPP_LIST_SIZE 1

// ============================================
// Debug Configuration
// ============================================
#define DEBUG_SERIAL true
#define SERIAL_BAUD 115200
