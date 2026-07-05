#ifndef UI_DRAW_H
#define UI_DRAW_H

#include "config.h" 
#include "icons.h"
#include <vector>
#include <FreeRTOS.h>
#include <semphr.h>

extern SemaphoreHandle_t uiMutex;

// --- COLORS ---
#define COL_BG            TFT_BLACK
#define COL_HEADER_TEXT   0xE73C   
#define COL_TEXT_DIM      0xBDF7   
#define COL_DIV_LINE      0x2C8A   
#define COL_DISABLED      0x3186   
#define COL_FOOTER_TEXT   TFT_WHITE 
#define COL_TEMP          0xFDA0   
#define COL_HUMID         TFT_CYAN 
#define COL_SOIL          TFT_GREEN
#define COL_LIGHT         TFT_YELLOW
#define BTN_ON_BG         0x04C0   
#define BTN_OFF_BG        0x9800   
#define BTN_BORDER_COMMON 0x630C   
#define BTN_TEXT_COLOR    TFT_WHITE

// --- FUNCTIONS ---
void initSprites();
void updateHeader(bool wifiOn, bool serverOn);
void updateSensors(float temp, float humid, int soil, int light);
void updateRelays(bool r1, bool r2, bool r3, bool r4);
void scrollFooter();          
void setFooterMsg(String msg);
void forceFooterMsg(String msg);

#endif
