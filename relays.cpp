#include "relays.h"
#include "ui_draw.h"
#include "config.h"

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

bool r_state[5] = {false, false, false, false, false};

void initRelays() {
    if (sysState.hasRelay1) {
        digitalWrite(PIN_RELAY_1, RELAY_OFF);
        pinMode(PIN_RELAY_1, OUTPUT);
    }
    if (sysState.hasRelay2) {
        digitalWrite(PIN_RELAY_2, RELAY_OFF);
        pinMode(PIN_RELAY_2, OUTPUT);
    }
    if (sysState.hasRelay3) {
        digitalWrite(PIN_RELAY_3, RELAY_OFF);
        pinMode(PIN_RELAY_3, OUTPUT);
    }
    if (sysState.hasRelay4) {
        digitalWrite(PIN_RELAY_4, RELAY_OFF);
        pinMode(PIN_RELAY_4, OUTPUT);
    }
}

void setRelay(int idx, bool state) {
    if ((idx == 1 && !sysState.hasRelay1) || (idx == 2 && !sysState.hasRelay2) ||
        (idx == 3 && !sysState.hasRelay3) || (idx == 4 && !sysState.hasRelay4)) return;

    int pin = (idx == 1) ? PIN_RELAY_1 : (idx == 2) ? PIN_RELAY_2 : (idx == 3) ? PIN_RELAY_3 : PIN_RELAY_4;

    digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);

    r_state[idx] = state;
    updateRelays(r_state[1], r_state[2], r_state[3], r_state[4]);
}
