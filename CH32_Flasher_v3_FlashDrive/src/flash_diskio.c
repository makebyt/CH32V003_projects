// flash_diskio.c
//
// Реализация низкоуровневого интерфейса FatFs (disk_read/disk_write/...)
// поверх зарезервированной области внутренней flash RP2040 - вместо SD-карты.
// Та же самая FatFs, что и в SD-версии, просто другой "диск" под капотом.
//
// TinyUSB MSC (usb_msc_driver.c) читает/пишет ровно те же блоки через
// flash_block_read()/flash_block_write() - поэтому и наша прошивка (через
// FatFs), и компьютер по USB видят один и тот же реальный FAT.

#include <string.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "ff.h"
#include "diskio.h"
#include "flash_diskio.h"

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

// 512 КБ под "диск" - с запасом под 3 прошивки + backup.bin + log.txt
// (у CH32V003 максимум 16КБ на файл, 4х16=64КБ занятых данных - места
// с большим запасом). Подальше от конца flash, как и наш прежний лог.
#define FLASH_STORAGE_SIZE   (512 * 1024)
#define FLASH_STORAGE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_STORAGE_SIZE)

#define SECTOR_SIZE 512u // логический размер блока FAT

//------------------------------------------------------------------------------

void flash_block_read(uint32_t lba, uint8_t* buff) {
  const uint8_t* src = (const uint8_t*)(XIP_BASE + FLASH_STORAGE_OFFSET + (uint32_t)lba * SECTOR_SIZE);
  memcpy(buff, src, SECTOR_SIZE);
}

void flash_block_write(uint32_t lba, const uint8_t* buff) {
  uint32_t byte_offset = lba * SECTOR_SIZE;

  // NOR flash: стереть можно только целым сектором 4096 байт, и только потом
  // программировать. Читаем сектор целиком, накладываем новые 512 байт в
  // нужном месте, стираем и перезаписываем весь сектор целиком.
  uint32_t flash_sector = (byte_offset / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
  uint32_t offset_in_sector = byte_offset % FLASH_SECTOR_SIZE;

  static uint8_t tmp[FLASH_SECTOR_SIZE];
  memcpy(tmp, (const uint8_t*)(XIP_BASE + FLASH_STORAGE_OFFSET + flash_sector), FLASH_SECTOR_SIZE);
  memcpy(tmp + offset_in_sector, buff, SECTOR_SIZE);

  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_STORAGE_OFFSET + flash_sector, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_STORAGE_OFFSET + flash_sector, tmp, FLASH_SECTOR_SIZE);
  restore_interrupts(ints);
}

uint32_t flash_block_count(void) {
  return FLASH_STORAGE_SIZE / SECTOR_SIZE;
}

//------------------------------------------------------------------------------
// FatFs diskio interface

DSTATUS disk_status(BYTE pdrv) {
  (void)pdrv;
  return 0; // всегда готов - внутренняя flash не может "пропасть", в отличие от SD
}

DSTATUS disk_initialize(BYTE pdrv) {
  (void)pdrv;
  return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
  (void)pdrv;
  for (UINT i = 0; i < count; i++) {
    flash_block_read((uint32_t)sector + i, buff + i * SECTOR_SIZE);
  }
  return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
  (void)pdrv;
  for (UINT i = 0; i < count; i++) {
    flash_block_write((uint32_t)sector + i, buff + i * SECTOR_SIZE);
  }
  return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
  (void)pdrv;
  switch (cmd) {
    case CTRL_SYNC:
      return RES_OK; // пишем сразу, откладывать нечего
    case GET_SECTOR_COUNT:
      *(LBA_t*)buff = flash_block_count();
      return RES_OK;
    case GET_SECTOR_SIZE:
      *(WORD*)buff = SECTOR_SIZE;
      return RES_OK;
    case GET_BLOCK_SIZE:
      *(DWORD*)buff = FLASH_SECTOR_SIZE / SECTOR_SIZE; // размер стираемого блока, в секторах
      return RES_OK;
    default:
      return RES_OK;
  }
}

DWORD get_fattime(void) {
  return 0; // нет RTC - для наших целей не критично
}
