// =============================================================================
// GrowPal — TFT_eSPI User_Setup (ESP32-S3 + ST7789 240x240)
// =============================================================================
// Sao chép file này đè lên:
//   <Arduino libraries>/TFT_eSPI/User_Setup.h
// hoặc đặt đường dẫn tương ứng trong User_Setup_Select.h của TFT_eSPI.
// =============================================================================

#define USER_SETUP_INFO "User_Setup"

#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// --- BỘ CHÂN CHO ESP32-S3 ---
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   13
#define TFT_RST  14
#define TFT_MISO -1    // Bắt buộc -1 để né chân Relay 3

// === CHÌA KHÓA FIX LỖI CRASH (0x00000010) ===
#define USE_HSPI_PORT  // Ép sang HSPI để né vùng nhớ cấm của ESP32-S3

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF

#define SPI_FREQUENCY  27000000
