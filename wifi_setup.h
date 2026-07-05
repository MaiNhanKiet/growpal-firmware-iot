#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

void initWifi();
void loopWifi();
void resetWifiConfig();

/** Load MQTT broker settings from Preferences into mqttServer / mqttPort / ... */
void loadMqttConfig();

/**
 * Bật SoftAP + Captive Portal để cấu hình lại (mất WiFi / MQTT lỗi kéo dài).
 * An toàn gọi nhiều lần — bỏ qua nếu portal đang chạy.
 */
void openConfigPortal(const char* reason);

#endif
