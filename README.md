# GrowPal

Firmware ESP32-S3 cho thiết bị giám sát môi trường trồng cây và điều khiển relay từ xa qua MQTT.

GrowPal tích hợp:

- Màn hình TFT ST7789 **240×240** (TFT_eSPI, sprite trên PSRAM)
- Captive Portal cấu hình **WiFi** và **MQTT** độc lập (2 tab)
- FreeRTOS: Network (Core 0) / Main + UI (Core 1)
- Hàng đợi MQTT tránh WDT khi parse JSON / ghi Flash
- Rule engine tự động hóa relay theo cảm biến

---

## Phần cứng

| Thành phần | Chi tiết |
|------------|----------|
| MCU | **ESP32-S3** (Dev Module), **OPI PSRAM**, Flash **16MB** |
| TFT | ST7789 240×240, SPI (HSPI) |
| Relay | Tối đa 4 kênh |
| Nút reset cấu hình | GPIO 0 (giữ ≥ 3 giây) |

### GPIO

| Chức năng | GPIO |
|-----------|------|
| TFT MOSI | 11 |
| TFT SCLK | 12 |
| TFT CS | 10 |
| TFT DC | 13 |
| TFT RST | 14 |
| TFT MISO | không dùng (`-1`) |
| Relay 1 | 4 |
| Relay 2 | 5 |
| Relay 3 | 15 |
| Relay 4 | 16 |
| Nút Reset WiFi/MQTT | 0 |

---

## Thư viện cần cài (Arduino Library Manager)

Cài các thư viện sau (Sketch → Include Library → Manage Libraries):

| Thư viện | Tác giả / ghi chú | Mục đích |
|----------|-------------------|----------|
| **TFT_eSPI** | Bodmer | Màn hình TFT + sprite |
| **PubSubClient** | Nick O'Leary | MQTT client |
| **ArduinoJson** | Benoit Blanchon | Parse/serialize JSON cấu hình |

Board package:

1. **File → Preferences → Additional boards manager URLs**  
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. **Tools → Board → Boards Manager** → cài **esp32 by Espressif Systems** (khuyến nghị bản ổn định mới nhất hỗ trợ ESP32-S3).

---

## Cấu hình TFT_eSPI (`User_Setup.h`)

File mẫu nằm trong repo: [`User_Setup.h`](./User_Setup.h).

**Bắt buộc** sao chép nội dung đè lên file của thư viện:

```
<Arduino libraries>/TFT_eSPI/User_Setup.h
```

Đường dẫn Windows thường gặp:

```
C:\Users\<Bạn>\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h
```

Nội dung cấu hình GrowPal:

```cpp
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
```

> `USE_HSPI_PORT` và `TFT_MISO -1` là bắt buộc trên ESP32-S3 với wiring hiện tại — thiếu sẽ dễ crash `0x00000010` hoặc xung đột GPIO relay.

Sau khi sửa `User_Setup.h`, **Compile lại toàn bộ** (không chỉ upload).

---

## Cấu hình Arduino IDE (ESP32-S3)

**Tools** menu — đặt đúng như sau:

| Mục | Giá trị |
|-----|---------|
| **Board** | `ESP32S3 Dev Module` |
| **Upload Speed** | `115200` |
| **USB Mode** | `Hardware CDC and JTAG` |
| **USB CDC On Boot** | `Disabled` |
| **USB Firmware MSC On Boot** | `Disabled` |
| **USB DFU On Boot** | `Disabled` |
| **Upload Mode** | `UART0 / Hardware CDC` |
| **CPU Frequency** | `240MHz (WiFi)` |
| **Flash Mode** | `QIO 80MHz` |
| **Flash Size** | `16MB (128Mb)` |
| **Partition Scheme** | `16M Flash (3MB APP/9.9MB FATFS)` |
| **Core Debug Level** | `None` |
| **PSRAM** | `OPI PSRAM` |
| **Arduino Runs On** | `Core 1` |
| **Events Run On** | `Core 1` |
| **Erase All Flash Before Sketch Upload** | `Disabled` |
| **JTAG Adapter** | `Disabled` |
| **Port** | cổng COM của board (ví dụ `COM5`) |
| **Programmer** | `Esptool` |

### Lưu ý quan trọng

- **PSRAM = OPI PSRAM**: bắt buộc để sprite TFT (`setColorDepth(8)` + `createSprite`) không hết RAM nội bộ.
- **Arduino Runs On / Events Run On = Core 1**: khớp kiến trúc firmware (Main/UI Core 1, Network task pin Core 0).
- Thư mục sketch phải trùng tên file `.ino`: đổi tên folder thành **`GrowPal`** rồi mở `GrowPal.ino`.

---

## Build & nạp firmware

1. Đổi tên thư mục project thành `GrowPal` (nếu đang là `OrchidPal`).
2. Cài board ESP32 + 3 thư viện ở trên.
3. Copy `User_Setup.h` vào thư mục `TFT_eSPI`.
4. Mở `GrowPal.ino` trong Arduino IDE.
5. Chọn cấu hình Tools như bảng trên.
6. **Sketch → Upload**.

Serial Monitor: baud **115200**.

---

## Cấu hình thiết bị

### Serial number

Trong `GrowPal.ino`:

```cpp
char SERIAL_NUMBER[SERIAL_NUMBER_LEN] = "GP-0S-0R-LBUJKQ-006";
```

Mọi MQTT topic được build dạng:

```text
device/<SERIAL_NUMBER>/status
device/<SERIAL_NUMBER>/telemetry
device/<SERIAL_NUMBER>/action_log
device/<SERIAL_NUMBER>/message
device/<SERIAL_NUMBER>/control/#
device/<SERIAL_NUMBER>/config
```

### WiFi & MQTT (Captive Portal)

Không hard-code broker trong firmware. Cấu hình qua portal:

| Tab | Nút | Lưu Flash |
|-----|-----|-----------|
| **WiFi** | Save WiFi | Namespace `wifi_creds` (chỉ khi kết nối thành công + verify) |
| **MQTT** | Save MQTT | Namespace `mqtt_creds` (verify read-back) |

Nếu **không** bấm Save MQTT, thiết bị giữ **factory default**:

| Tham số | Mặc định |
|---------|----------|
| Broker | `167.71.201.144` |
| Port | `1883` |
| User | `admin` |
| Pass | `123456` |

### Mở portal

SoftAP SSID: **`GrowPal_XXXX`** (XXXX từ MAC).

Portal tự bật khi:

- Chưa có WiFi đã lưu
- Kết nối WiFi thất bại lúc boot
- **Mất WiFi** lúc đang chạy
- **MQTT lỗi kéo dài** (~60 giây / 12 lần thử)
- Giữ nút GPIO 0 ≥ 3 giây (xóa WiFi + MQTT, restart)

Cách dùng:

1. Kết nối điện thoại/laptop vào `GrowPal_XXXX`.
2. Mở trình duyệt (thường tự vào captive portal, hoặc `http://192.168.4.1`).
3. Tab **WiFi**: chọn mạng → Save WiFi.
4. Tab **MQTT** (tuỳ chọn): sửa broker → Save MQTT.

---

## Kiến trúc firmware

```text
Core 0 — NetworkTask
  loopWifi() / loopMqtt()
  mqttCallback → chỉ xQueueSend (không parse JSON / không ghi Flash)

Core 1 — loop()
  xQueueReceive → processMqttMessage (JSON, Preferences, UI)
  cảm biến, rule engine, scroll footer
  uiMutex bảo vệ TFT sprites
```

| File | Vai trò |
|------|---------|
| `GrowPal.ino` | `setup` / `loop`, `sysState`, rule engine, MQTT queue drain |
| `config.h` | Pin, buffer, `SystemState`, topic, MQTT runtime buffers |
| `wifi_setup.*` | Captive portal, NVS WiFi/MQTT, SoftAP |
| `web_portal.h` | HTML/CSS/JS portal (2 tab, Lucide SVG) |
| `mqtt_handler.*` | PubSubClient, publish telemetry/log |
| `ui_draw.*` | Header / sensors / relays / footer sprites |
| `sensor.*` / `relays.*` | Đọc cảm biến (mock), điều khiển relay |
| `User_Setup.h` | Mẫu cấu hình TFT_eSPI |

---

## MQTT — ví dụ kiểm tra

Giả sử `SERIAL_NUMBER = GP-0S-0R-LBUJKQ-006` và broker local:

```bash
# Broker Docker (tuỳ chọn)
docker run -d --name growpal-mqtt -p 1883:1883 eclipse-mosquitto:2

# Bật relay 1
mosquitto_pub -h 127.0.0.1 -p 1883 -u admin -P 123456 \
  -t "device/GP-0S-0R-LBUJKQ-006/control/1" -m "ON"

# Footer message
mosquitto_pub -h 127.0.0.1 -p 1883 -u admin -P 123456 \
  -t "device/GP-0S-0R-LBUJKQ-006/message" -m "Hello GrowPal"

# Theo dõi toàn bộ topic thiết bị
mosquitto_sub -h 127.0.0.1 -p 1883 -u admin -P 123456 \
  -t "device/GP-0S-0R-LBUJKQ-006/#" -v
```

### Publish từ thiết bị

- `.../status` — `{"status":"ONLINE"}` (retained), LWT `OFFLINE`
- `.../telemetry` — ví dụ `{"temp":28.5,"humid":65.0,"soil":70,"light":3200}`
- `.../action_log` — log rule engine bật relay

### Subscribe trên thiết bị

- `.../message` — chạy chữ footer
- `.../control/#` — điều khiển relay thủ công
- `.../config` — hot-reload hardware + logic rules (JSON NestJS)

---

## Xử lý sự cố nhanh

| Hiện tượng | Kiểm tra |
|------------|----------|
| Crash `0x00000010` / TFT đen | `USE_HSPI_PORT`, `TFT_MISO -1`, chân SPI đúng |
| Sprite fail / UI lỗi | **PSRAM = OPI PSRAM**, Serial có dòng `createSprite failed` |
| Không upload | Port COM, Upload Mode `UART0 / Hardware CDC`, tốc độ 115200 |
| Không vào portal | SSID `GrowPal_XXXX`, giữ nút reset 3s để xóa cấu hình cũ |
| MQTT không online | Tab MQTT trên portal, Serial log `MQTT Error: n` |

---

## Giấy phép / ghi chú

Firmware thương mại cho sản phẩm **GrowPal**. Điều chỉnh `SERIAL_NUMBER` và thông số MQTT theo từng thiết bị trước khi xuất xưởng.
