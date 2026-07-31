// FlashLog.h
//
// Простой текстовый лог, живущий в последнем секторе flash-памяти RP2040
// (переживает выключение питания). Не файловая система - просто кольцевой
// текстовый буфер на 4 КБ, целиком перезаписываемый при каждом обновлении.
//
// При старте лог подгружается прямо из flash (она memory-mapped, читать
// можно как обычную память - никакого специального API не нужно).
// Смотреть лог - через log_dump_uart() (например, по долгому нажатию кнопки).

#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

// Последний сектор flash - подальше от кода программы.
#define FLOG_SIZE   FLASH_SECTOR_SIZE // 4096
#define FLOG_OFFSET (PICO_FLASH_SIZE_BYTES - FLOG_SIZE)

class FlashLog {
public:
  void load() {
    const uint8_t* p = (const uint8_t*)(XIP_BASE + FLOG_OFFSET);
    len = 0;
    while (len < FLOG_SIZE - 1 && p[len] != 0xFF) len++;
    memcpy(buf, p, len);
    buf[len] = 0;
  }

  void append(const char* fmt, ...) {
    char line[96];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    if (n <= 0) return;
    if (n > (int)sizeof(line) - 1) n = sizeof(line) - 1;

    if (len + n >= FLOG_SIZE) {
      // Буфер полон - выкидываем самые старые строки, освобождая место.
      size_t need = (len + n) - FLOG_SIZE + 256;
      if (need > len) need = len;
      size_t drop = 0;
      while (drop < need && drop < len) {
        char* nl = (char*)memchr(buf + drop, '\n', len - drop);
        if (!nl) { drop = len; break; }
        drop = (nl - buf) + 1;
      }
      memmove(buf, buf + drop, len - drop);
      len -= drop;
    }

    memcpy(buf + len, line, n);
    len += n;
    buf[len] = 0;
  }

  // Пишет во flash весь текущий буфер целиком (erase + program одного сектора).
  void flush() {
    static uint8_t page[FLOG_SIZE];
    memset(page, 0xFF, FLOG_SIZE);
    memcpy(page, buf, len);

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLOG_OFFSET, FLOG_SIZE);
    flash_range_program(FLOG_OFFSET, page, FLOG_SIZE);
    restore_interrupts(ints);
  }

  void dump_uart() {
    printf("\n===== LOG (%u/%u bytes) =====\n", (unsigned)len, (unsigned)FLOG_SIZE);
    for (size_t i = 0; i < len; i++) putchar(buf[i]);
    printf("\n===== END LOG =====\n");
  }

private:
  char buf[FLOG_SIZE];
  size_t len = 0;
};
