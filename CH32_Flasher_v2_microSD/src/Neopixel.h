// Тонкая обёртка над официальным PIO-драйвером WS2812 из pico-examples
// (https://github.com/raspberrypi/pico-examples, BSD-3-Clause,
//  Copyright (c) 2020 Raspberry Pi (Trading) Ltd.), см. ws2812.pio.
//
// Работает на pio1, чтобы не конфликтовать с PicoSWIO, который жёстко
// использует pio0 (и сбрасывает его целиком при инициализации).

#pragma once

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

class Neopixel {
public:
  void init(uint pin) {
    pio = pio1;
    uint offset = pio_add_program(pio, &ws2812_program);
    sm = pio_claim_unused_sm(pio, true);
    ws2812_program_init(pio, sm, offset, pin, 800000.f, false /* RGB, not RGBW */);
  }

  void set_color(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t grb = ((uint32_t)r << 8) | ((uint32_t)g << 16) | (uint32_t)b;
    pio_sm_put_blocking(pio, sm, grb << 8u);
  }

private:
  PIO pio;
  uint sm;
};
