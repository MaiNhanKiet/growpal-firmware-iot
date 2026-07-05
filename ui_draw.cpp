#include "ui_draw.h"

TFT_eSprite sprHeader  = TFT_eSprite(&tft);
TFT_eSprite sprSensors = TFT_eSprite(&tft);
TFT_eSprite sprRelays  = TFT_eSprite(&tft);
TFT_eSprite sprFooter  = TFT_eSprite(&tft);

int scrollX = 240;
String currentMsg = "System Initializing...";
std::vector<String> msgQueue;
const int MAX_QUEUE_SIZE = 10;

/** Header/footer need 16-bit so gradient colors are not crushed to black (8-bit palette). */
static bool createSpriteChecked(TFT_eSprite& spr, int16_t w, int16_t h, uint8_t depth, const char* name) {
    spr.setColorDepth(depth);
    void* buf = spr.createSprite(w, h);
    if (buf == nullptr) {
        Serial.printf("ERROR: %s createSprite(%d,%d) depth=%u failed - RAM exhausted!\n",
                      name, w, h, depth);
        return false;
    }
    return true;
}

void initSprites() {
    createSpriteChecked(sprHeader,  240, 36,  16, "sprHeader");
    createSpriteChecked(sprSensors, 240, 140,  8, "sprSensors");
    createSpriteChecked(sprRelays,  240, 44,   8, "sprRelays");
    createSpriteChecked(sprFooter,  240, 20,  16, "sprFooter");
}

void updateHeader(bool wifiOn, bool serverOn) {
    if (xSemaphoreTake(uiMutex, portMAX_DELAY)) {
        sprHeader.fillSprite(COL_BG);
        for (int i = 0; i < 36; i++) {
            uint16_t color = sprHeader.color565(0, i, (int)(i * 1.5f));
            sprHeader.drawFastHLine(0, i, 240, color);
        }
        sprHeader.drawFastHLine(0, 35, 240, COL_DIV_LINE);

        sprHeader.setFreeFont(&FreeSansBoldOblique12pt7b);
        sprHeader.setTextColor(COL_HEADER_TEXT);
        sprHeader.setTextDatum(ML_DATUM);
        sprHeader.drawString("GrowPal", 10, 18);

        uint16_t wifiColor = wifiOn ? TFT_WHITE : COL_DISABLED;
        const unsigned char* wIcon = wifiOn ? wifi_icon : wifi_off_icon;
        sprHeader.drawXBitmap(180, 6, wIcon, 24, 24, wifiColor);

        uint16_t serverColor = serverOn ? TFT_WHITE : COL_DISABLED;
        const unsigned char* sIcon = serverOn ? server_icon : server_off_icon;
        sprHeader.drawXBitmap(212, 6, sIcon, 24, 24, serverColor);

        sprHeader.pushSprite(0, 0);
        xSemaphoreGive(uiMutex);
    }
}

void drawOneCard(int x, int y, int w, int h, String title, float value, String unit, const unsigned char* icon, uint16_t color, bool active) {
    uint16_t frameColor = active ? color : COL_DISABLED;
    sprSensors.drawRoundRect(x, y, w, h, 8, frameColor);
    sprSensors.drawXBitmap(x + 8, y + 15, icon, 24, 24, active ? color : COL_DISABLED);

    sprSensors.setTextDatum(TL_DATUM); sprSensors.setTextFont(1);
    sprSensors.setTextColor(active ? COL_TEXT_DIM : COL_DISABLED);
    sprSensors.drawString(title, x + 38, y + 17);

    sprSensors.setTextColor(active ? color : COL_DISABLED);
    sprSensors.setTextDatum(BR_DATUM);
    String valStr = active ? ((title == "LIGHT") ? String((int)value) : String(value, 1)) : "--";
    sprSensors.drawString(valStr, x + w - 20, y + h - 10, 4);

    sprSensors.setTextColor(active ? COL_TEXT_DIM : COL_DISABLED);
    sprSensors.setTextFont(1);
    sprSensors.drawString(unit, x + w - 6, y + h - 10);
}

void updateSensors(float temp, float humid, int soil, int light) {
    if (xSemaphoreTake(uiMutex, portMAX_DELAY)) {
        sprSensors.fillSprite(COL_BG);
        int w = (240 - 12) / 2;
        drawOneCard(4, 4, w, 64, "TEMP", temp, "C", temperature_icon, COL_TEMP, sysState.hasTemp);
        drawOneCard(4 + w + 4, 4, w, 64, "HUMID", humid, "%", humidity_icon, COL_HUMID, sysState.hasHumid);
        drawOneCard(4, 72, w, 64, "SOIL", soil, "%", leaf_icon, COL_SOIL, sysState.hasSoil);
        drawOneCard(4 + w + 4, 72, w, 64, "LIGHT", (float)light, "lx", sun_icon, COL_LIGHT, sysState.hasLight);
        sprSensors.pushSprite(0, 36);
        xSemaphoreGive(uiMutex);
    }
}

void drawRelayButton(int x, int y, int w, int h, String label, bool state, bool active) {
    uint16_t bg = (!active) ? COL_BG : (state ? BTN_ON_BG : BTN_OFF_BG);
    uint16_t border = (!active) ? COL_DISABLED : BTN_BORDER_COMMON;
    uint16_t text = (!active) ? COL_DISABLED : BTN_TEXT_COLOR;

    if (active) sprRelays.fillRoundRect(x, y, w, h, 6, bg);
    else sprRelays.drawRoundRect(x, y, w, h, 6, border);
    if (active) sprRelays.drawRoundRect(x, y, w, h, 6, border);

    sprRelays.setTextFont(2); sprRelays.setTextColor(text);
    sprRelays.setTextDatum(MC_DATUM);
    sprRelays.drawString(label, x + w / 2, y + (h / 2) - 1);
}

void updateRelays(bool r1, bool r2, bool r3, bool r4) {
    if (xSemaphoreTake(uiMutex, portMAX_DELAY)) {
        sprRelays.fillSprite(COL_BG);
        int w = (240 - 20) / 4;
        drawRelayButton(4, 5, w, 34, "RL 1", r1, sysState.hasRelay1);
        drawRelayButton(4 + (w+4)*1, 5, w, 34, "RL 2", r2, sysState.hasRelay2);
        drawRelayButton(4 + (w+4)*2, 5, w, 34, "RL 3", r3, sysState.hasRelay3);
        drawRelayButton(4 + (w+4)*3, 5, w, 34, "RL 4", r4, sysState.hasRelay4);
        sprRelays.pushSprite(0, 176);
        xSemaphoreGive(uiMutex);
    }
}

void setFooterMsg(String msg) {
    if (xSemaphoreTake(uiMutex, portMAX_DELAY)) {
        if (currentMsg != msg) {
            bool exists = false;
            for (const String& existing : msgQueue) {
                if (existing == msg) { exists = true; break; }
            }
            if (!exists && msgQueue.size() < MAX_QUEUE_SIZE) {
                msgQueue.push_back(msg);
            }
        }
        xSemaphoreGive(uiMutex);
    }
}

void scrollFooter() {
    if (xSemaphoreTake(uiMutex, portMAX_DELAY)) {
        sprFooter.fillSprite(COL_BG);
        for (int i = 0; i < 20; i++) {
            uint16_t color = sprFooter.color565(0, (int)(i * 1.5f), i * 2);
            sprFooter.drawFastHLine(0, i, 240, color);
        }
        sprFooter.drawFastHLine(0, 0, 240, COL_DIV_LINE);

        sprFooter.setTextFont(2);
        sprFooter.setTextColor(COL_FOOTER_TEXT);
        sprFooter.setTextDatum(TL_DATUM);

        int msgWidth = sprFooter.textWidth(currentMsg);
        scrollX--;

        sprFooter.drawString(currentMsg, scrollX, 3);

        int gap = 70;
        int tempX = scrollX;
        int tempW = msgWidth;

        for (size_t i = 0; i < msgQueue.size(); i++) {
            int nextX = tempX + tempW + gap;

            if (nextX >= 240) break;

            String nextMsg = msgQueue[i];
            sprFooter.drawString(nextMsg, nextX, 3);

            tempX = nextX;
            tempW = sprFooter.textWidth(nextMsg);
        }

        if (scrollX < -msgWidth) {
            if (!msgQueue.empty()) {
                scrollX = scrollX + msgWidth + gap;
                currentMsg = msgQueue.front();
                msgQueue.erase(msgQueue.begin());
            } else {
                scrollX = 240;
            }
        }

        sprFooter.pushSprite(0, 220);
        xSemaphoreGive(uiMutex);
    }
}

void forceFooterMsg(String msg) {
    if (xSemaphoreTake(uiMutex, portMAX_DELAY)) {
        msgQueue.clear();
        currentMsg = msg;
        scrollX = 200;
        xSemaphoreGive(uiMutex);
    }
}
