#include "font.h"

const uint8_t F_0[5] PROGMEM = {0b111,0b101,0b101,0b101,0b111};
const uint8_t F_1[5] PROGMEM = {0b010,0b110,0b010,0b010,0b111};
const uint8_t F_2[5] PROGMEM = {0b111,0b001,0b111,0b100,0b111};
const uint8_t F_3[5] PROGMEM = {0b111,0b001,0b111,0b001,0b111};
const uint8_t F_4[5] PROGMEM = {0b101,0b101,0b111,0b001,0b001};
const uint8_t F_5[5] PROGMEM = {0b111,0b100,0b111,0b001,0b111};
const uint8_t F_6[5] PROGMEM = {0b111,0b100,0b111,0b101,0b111};
const uint8_t F_7[5] PROGMEM = {0b111,0b001,0b010,0b010,0b010};
const uint8_t F_8[5] PROGMEM = {0b111,0b101,0b111,0b101,0b111};
const uint8_t F_9[5] PROGMEM = {0b111,0b101,0b111,0b001,0b111};
const uint8_t F_H[5] PROGMEM = {0b101,0b101,0b111,0b101,0b101};
const uint8_t F_T[5] PROGMEM = {0b111,0b010,0b010,0b010,0b010};
const uint8_t F_X[5] PROGMEM = {0b101,0b010,0b010,0b010,0b101};
const uint8_t F_DEGC[5] PROGMEM = {
  0b000,
  0b111,
  0b100,
  0b111
};
const uint8_t F_PCT[5] PROGMEM = {
  0b101,
  0b001,
  0b010,
  0b100,
  0b101
};

const uint8_t* digitFont(int d) {
  switch (d) {
    case 0: return F_0;
    case 1: return F_1;
    case 2: return F_2;
    case 3: return F_3;
    case 4: return F_4;
    case 5: return F_5;
    case 6: return F_6;
    case 7: return F_7;
    case 8: return F_8;
    case 9: return F_9;
    default: return F_0;
  }
}
