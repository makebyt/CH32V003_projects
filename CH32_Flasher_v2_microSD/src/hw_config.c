// hw_config.c
//
// Настройка SD-карты под нашу распиновку (плата YD-RP2040):
//   GP2 = SCK, GP3 = MOSI, GP4 = MISO, GP5 = CS  (аппаратный SPI0)
//
// Based on the example from carlk3/no-OS-FatFS-SD-SPI-RPi-Pico
// (Apache License 2.0, Copyright 2021 Carl John Kugler III)

#include <string.h>
#include "my_debug.h"
#include "hw_config.h"
#include "ff.h"
#include "diskio.h"

static spi_t spis[] = {
    {
        .hw_inst = spi0,
        .miso_gpio = 4,
        .mosi_gpio = 3,
        .sck_gpio = 2,
        .baud_rate = 12500 * 1000,
    }
};

static sd_card_t sd_cards[] = {
    {
        .pcName = "0:",
        .spi = &spis[0],
        .ss_gpio = 5,
        .use_card_detect = false,   // нет отдельного пина card-detect - не подключали
        .card_detect_gpio = -1,
        .card_detected_true = -1
    }
};

size_t sd_get_num() { return count_of(sd_cards); }
sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}
size_t spi_get_num() { return count_of(spis); }
spi_t *spi_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return &spis[num];
    } else {
        return NULL;
    }
}
