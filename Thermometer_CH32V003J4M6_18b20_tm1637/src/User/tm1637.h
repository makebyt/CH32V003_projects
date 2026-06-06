#ifndef TM1637_H
#define TM1637_H

#include <stdint.h>

void TM1637_Init(void);
void TM1637_SetBrightness(uint8_t level);
void TM1637_DisplayDigit(uint8_t pos, uint8_t value, uint8_t dot);
void TM1637_DisplayOff(void);

#endif
