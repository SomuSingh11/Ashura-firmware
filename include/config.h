#pragma once

// ============================================
// WiFi Configuration
// ============================================
#define WIFI_SSID "********"
#define WIFI_PASSWORD "********"

// ============================================
// WebSocket Server Configuration
// ============================================
#define WS_SERVER_HOST "192.168.31.224"  
#define WS_SERVER_PORT 3000
#define WS_SERVER_PATH "/ws"

// ============================================
// Device Configuration
// ============================================
#define DEVICE_ID "esp32-core-01"
#define DEVICE_NAME "Ashura Core"
#define FIRMWARE_VERSION "v1.0"
#define DEVELOPED_BY "k4ge"

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

#define SUBMENU_ITEM_HEIGHT 16
#define SUBMENU_ITEMS_ON_SCREEN 4

#define WLED_DEVICE_MENU_ITEM_HEIGHT 16
#define WLED_DEVICE_MENU_ITEMS_ON_SCREEN 4
#define WLED_DEVICE_MENU_START_Y 12

// ============================================
// Timing Configuration
// ============================================
#define HEARTBEAT_INTERVAL 10000  // 10 seconds
#define RECONNECT_DELAY 5000      // 5 seconds
#define SCREEN_TIMEOUT 30000      // 30 seconds
#define SCREENSAVER_TIMEOUT 200000 // 200 seconds
#define LASTMESSAGE_HOMESCREEN_TIMEOUT 10000 // 10 seconds
#define BUTTON_DEBOUNCE_MS 30
#define NTP_SYNC_INTERVAL 30000 // 30 seconds
#define LONG_PRESS_MS 1000 // 1 second

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

// ============================================
// UTC offset (seconds)
// ============================================
#define UTC_OFFSET_SEC       (5 * 3600 + 30 * 60)   // IST +5:30

// ============================================
// WLED HTTP Configuration
// ============================================
#define WLED_HTTPCLIENT_TIMEOUT 3000
#define WLED_MAX_DEVICES 8 

// ============================================
// Notification Configuration
// ============================================
#define MAX_NOTIFICATION_BUFFER 10