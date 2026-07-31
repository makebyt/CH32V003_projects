// usb_msc_driver.c
//
// TinyUSB MSC callbacks - отдают компьютеру ровно те же блоки, что читает и
// пишет наша собственная FatFs (через flash_diskio.c). Один и тот же кусок
// flash виден и компьютеру по USB, и прошивке изнутри.
//
// На основе структуры примера из oyama/pico-usb-flash-drive.

#include <string.h>
#include "tusb.h"
#include "flash_diskio.h"

#define DISK_BLOCK_SIZE 512u

static bool ejected = false;

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
  (void) lun;
  const char vid[] = "CH32fl";
  const char pid[] = "Firmware Storage";
  const char rev[] = "1.0";
  memcpy(vendor_id, vid, strlen(vid));
  memcpy(product_id, pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
  (void) lun;
  if (ejected) {
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return false;
  }
  return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
  (void) lun;
  *block_count = flash_block_count();
  *block_size  = DISK_BLOCK_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
  (void) lun; (void) power_condition;
  if (load_eject) {
    if (!start) ejected = true;
  }
  return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  (void) lun; (void) offset;
  if (lba >= flash_block_count()) return -1;
  flash_block_read(lba, (uint8_t*)buffer);
  return (int32_t) bufsize;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
  (void) lun;
  return true;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  (void) lun; (void) offset;
  if (lba >= flash_block_count()) return -1;
  flash_block_write(lba, buffer);
  return (int32_t) bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
  (void) buffer; (void) bufsize;
  tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
  return -1;
}
