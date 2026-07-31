#pragma once
#include <stdint.h>

void flash_block_read(uint32_t lba, uint8_t* buff);
void flash_block_write(uint32_t lba, const uint8_t* buff);
uint32_t flash_block_count(void);
