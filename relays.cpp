#include "relays.h"
#include "ui_draw.h" 
#include "config.h" 

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

bool r_state[5] = {false, false, false, false, false}; 

void initRelays() {
    if (hasRelay1) { 
        digitalWrite(PIN_RELAY_1, RELAY_OFF); 
        pinMode(PIN_RELAY_1, OUTPUT); 
    }
    if (hasRelay2) { 
        digitalWrite(PIN_RELAY_2, RELAY_OFF); 
        pinMode(PIN_RELAY_2, OUTPUT); 
    }
    if (hasRelay3) { 
        digitalWrite(PIN_RELAY_3, RELAY_OFF); 
        pinMode(PIN_RELAY_3, OUTPUT); 
    }
    if (hasRelay4) { 
        digitalWrite(PIN_RELAY_4, RELAY_OFF); 
        pinMode(PIN_RELAY_4, OUTPUT); 
    }
}

void setRelay(int idx, bool state) {
    // 1. CHỐT CHẶN BẢO VỆ: Nếu Relay đó chưa được cấu hình Role thì không làm gì cả
    if ((idx==1 && !hasRelay1) || (idx==2 && !hasRelay2) || 
        (idx==3 && !hasRelay3) || (idx==4 && !hasRelay4)) return;

    // 2. TÌM CHÂN GPIO TƯƠNG ỨNG
    int pin = (idx==1)?PIN_RELAY_1 : (idx==2)?PIN_RELAY_2 : (idx==3)?PIN_RELAY_3 : PIN_RELAY_4;
    
    // 3. ĐIỀU KHIỂN PHẦN CỨNG VẬT LÝ (Đã mở khóa comment)
    digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
    
    // 4. LƯU TRẠNG THÁI VÀ CẬP NHẬT GIAO DIỆN MÀN HÌNH TFT
    r_state[idx] = state;
    updateRelays(r_state[1], r_state[2], r_state[3], r_state[4]);
}
