# OrchidPal

OrchidPal là firmware cho ESP32 giúp giám sát môi trường trồng lan và điều khiển relay từ xa qua MQTT. Dự án tích hợp giao diện TFT 240x240 để theo dõi trạng thái trực tiếp và captive portal để cấu hình WiFi nhanh mà không cần hard-code SSID/password.

## Tổng quan tính năng

- Hiển thị trạng thái **WiFi + MQTT** theo thời gian thực trên TFT.
- Hiển thị dữ liệu môi trường gồm:
  - Nhiệt độ
  - Độ ẩm không khí
  - Độ ẩm đất
  - Cường độ ánh sáng
- Điều khiển tối đa **4 relay** qua MQTT.
- Nhận và hiển thị thông điệp chạy chữ ở footer từ MQTT topic `message`.
- Tự mở **captive portal** khi chưa có WiFi hoặc mất kết nối.
- Lưu thông tin WiFi vào NVS (Preferences), tự kết nối lại sau reboot.
- Nhấn giữ nút reset 3 giây để xóa WiFi đã lưu và khởi động lại.

## Phần cứng

- MCU: ESP32
- Màn hình: TFT dùng thư viện `TFT_eSPI` (240x240)
- GPIO relay (mặc định):
  - Relay 1: `GPIO 4`
  - Relay 2: `GPIO 5`
  - Relay 3: `GPIO 15`
  - Relay 4: `GPIO 16`
- Nút reset WiFi: `GPIO 0` (`INPUT_PULLUP`)

> Lưu ý: Trong phiên bản hiện tại, phần `digitalWrite` điều khiển relay vật lý đang được comment. Trạng thái relay hiện chỉ cập nhật trên UI.

## Cấu hình nhanh

### 1) Cập nhật cấu hình hệ thống

Trong `config.h`:

- `MQTT_SERVER`
- `MQTT_PORT`
- `MQTT_USER`
- `MQTT_PASS`
- `PIN_RELAY_1..4`
- `PIN_BUTTON_RESET`

Trong `OrchidPal.ino`:

- `SERIAL_NUMBER`

`SERIAL_NUMBER` được dùng để build toàn bộ MQTT topic theo từng thiết bị.

### 2) Build và nạp firmware

#### Arduino IDE (khuyến nghị)

1. Cài board **ESP32 by Espressif Systems** trong Board Manager.
2. Cài thư viện:
   - `TFT_eSPI`
   - `PubSubClient`
3. Mở file `OrchidPal.ino`.
4. Chọn đúng board ESP32 và cổng COM.
5. Upload firmware.

#### PlatformIO (tuỳ chọn)

Nếu dùng PlatformIO, hãy tự tạo `platformio.ini` phù hợp board ESP32 và khai báo các dependency tương ứng.

## MQTT topics

Ví dụ với:

- `SERIAL_NUMBER = SN-ORCHID-0001`

### Publish

- `device/SN-ORCHID-0001/status`
  - Online: `{"status":"ONLINE"}`
  - Last Will: `{"status":"OFFLINE"}`
- `device/SN-ORCHID-0001/telemetry`
  - Ví dụ payload: `{"temp":28.5,"humid":65.0,"soil":70,"light":3200}`
  - Chỉ bao gồm các trường đang được bật cờ `hasTemp/hasHumid/hasSoil/hasLight`.

### Subscribe

- `device/SN-ORCHID-0001/message`
  - Nội dung text hiển thị tại footer.
- `device/SN-ORCHID-0001/control/#`
  - Ví dụ:
    - `device/SN-ORCHID-0001/control/1`
    - `device/SN-ORCHID-0001/control/2`
    - `device/SN-ORCHID-0001/control/3`
    - `device/SN-ORCHID-0001/control/4`

### Quy tắc parse lệnh relay

Payload được hiểu là **ON** nếu chứa một trong các giá trị:

- `ON`
- `true`
- `1`

Các trường hợp còn lại được hiểu là **OFF**.

## Captive portal WiFi

Khi chưa có WiFi hoặc xác thực thất bại, thiết bị sẽ mở AP cấu hình:

- SSID: `OrchidPal_XXXX` (dựa trên MAC)
- URL portal: `http://192.168.4.1`

### API nội bộ của portal

- `GET /`
- `GET /scan`
- `POST /save`
- `GET /status`

### Luồng cấu hình

1. Kết nối điện thoại/laptop vào WiFi `OrchidPal_XXXX`.
2. Mở trình duyệt và vào portal.
3. Chọn mạng WiFi, nhập mật khẩu, nhấn lưu.
4. Thiết bị kiểm tra kết nối:
   - Thành công: lưu vào NVS và chuyển sang STA-only.
   - Thất bại: báo sai mật khẩu để nhập lại.

## Kiến trúc runtime

- **Core 0**: network task
  - `loopWifi()`
  - `loopMqtt()`
- **Core 1**: main loop
  - Xử lý nút reset
  - Cập nhật footer scrolling mỗi 30ms
  - Cập nhật khung sensor mỗi 2000ms (đọc/publish hiện đang comment)

UI được bảo vệ bằng `uiMutex` để tránh xung đột khi vẽ từ nhiều luồng.

## Cấu trúc mã nguồn

- `OrchidPal.ino`: điểm vào chính (`setup/loop`), tạo network task, khởi tạo topic
- `config.h`: cấu hình chung, pin mapping, extern
- `wifi_setup.h/.cpp`: captive portal, lưu credential, reconnect
- `mqtt_handler.h/.cpp`: kết nối broker, subscribe/publish, callback relay
- `sensor.h/.cpp`: model `SensorData`, dữ liệu mô phỏng random walk
- `relays.h/.cpp`: init/set relay, cập nhật trạng thái UI
- `ui_draw.h/.cpp`: toàn bộ phần render UI
- `web_portal.h`: HTML/CSS/JS của captive portal
- `icons.h`: bitmap icon cho TFT

## Chạy MQTT server bằng Docker

Bạn có thể bật nhanh Mosquitto local bằng Docker (không dùng IP hard-code):

```bash
docker run -d --name orchidpal-mqtt -p 1883:1883 eclipse-mosquitto:2
```

Kiểm tra container đang chạy:

```bash
docker ps
```

> Nếu cần dừng/xoá container:
>
> - Dừng: `docker stop orchidpal-mqtt`
> - Xoá: `docker rm orchidpal-mqtt`

## Kiểm thử MQTT nhanh (dùng host dạng `xxxx`)

Đặt biến host server để test linh hoạt:

```bash
MQTT_HOST=xxxx
```

Ví dụ:

- Nếu chạy local: `MQTT_HOST=localhost`
- Nếu server LAN: `MQTT_HOST=192.168.1.10`
- Nếu domain public: `MQTT_HOST=mqtt.example.com`

Bật Relay 1:

```bash
mosquitto_pub -h "$MQTT_HOST" -p 1883 -t "device/SN-ORCHID-0001/control/1" -m "ON"
```

Gửi message hiển thị footer:

```bash
mosquitto_pub -h "$MQTT_HOST" -p 1883 -t "device/SN-ORCHID-0001/message" -m "Hello OrchidPal"
```

Theo dõi message từ thiết bị:

```bash
mosquitto_sub -h "$MQTT_HOST" -p 1883 -t "device/SN-ORCHID-0001/#" -v
```

## Hạn chế hiện tại

- Dữ liệu sensor đang là mô phỏng, chưa đọc từ cảm biến thực.
- Đoạn đọc sensor và publish telemetry định kỳ đang comment.
- `digitalWrite` relay vật lý đang comment.
- Thông tin MQTT đang hard-code trong mã nguồn.

## Hướng phát triển đề xuất

- Tích hợp cảm biến thật (DHT/soil/light).
- Bật lại telemetry định kỳ sau khi hoàn thiện sensor pipeline.
- Bật điều khiển relay vật lý trong `setRelay()` theo nhu cầu phần cứng.
- Tách thông tin nhạy cảm (MQTT/WiFi) sang cơ chế cấu hình an toàn hơn.

---

Nếu bạn muốn, mình có thể viết thêm bản README nâng cao gồm sơ đồ wiring chi tiết, checklist bring-up và mục troubleshooting theo từng lỗi phổ biến.
