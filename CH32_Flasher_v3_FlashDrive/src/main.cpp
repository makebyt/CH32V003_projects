// ch32_flasher_flashdrive (ЭКСПЕРИМЕНТАЛЬНАЯ ВЕРСИЯ)
//
// То же самое, что ch32_flasher_demo (SD-версия), но вместо SD-карты
// прошивки хранятся в зарезервированной области внутренней flash Pico
// (512 КБ), отформатированной как FAT. Эта же область одновременно
// отдаётся по USB как обычная флешка (drag & drop) - подключил USB,
// перекинул новый .bin через Проводник, ничего паять не нужно.
//
// Прошивка и компьютер по USB видят ОДИН И ТОТ ЖЕ реальный диск через
// общий низкоуровневый драйвер (flash_diskio.c) - не два раздельных
// хранилища.
//
// НЕ ПРОВЕРЕНО на реальном железе - собрано и слинковано здесь, логика
// программирования чипа (SWIO) не менялась и уже проверена в SD-версии,
// но сама работа с USB MSC + внутренней flash - экспериментальная, из
// коробки может потребовать доводки.
//
// Индикация (логика "температуры состояния", не светофор):
//   синий, тускло   - покой, ожидание
//   жёлтый          - идёт запись, минимум 0.7с на экране
//   зелёный, 3с     - успех записи
//   красный, ровно, 3с   - чип отвечал, но запись/стирание
//                      не выполнились
//   красный, мигает, 3с  - записалось, но verify не сошёлся
//   жёлтый, мигает ~2 раза/сек             - не смогли начать (чип не
//                      отвечает ИЛИ файла нет на диске)
//   зелёный, быстро мигает ~1-2с, потом 2с ровно - backup снят успешно
//   фиолетовый, двойной блинк, потом зелёный ровно - restore прошёл успешно
//
// Кнопка backup/restore (GP19): короткое нажатие = снять backup.bin с чипа,
// долгое (>=1.5с) = записать backup.bin обратно на чип.
//
// log.txt автоматически выгружается по UART при каждом включении питания /
// нажатии reset на Pico (перед этим - короткая белая вспышка).
//
// Based on the SWIO/RVDebug/WCHFlash modules from the PicoRVD project
// (https://github.com/Community-PIO-CH32V/PicoRVD, MIT license), the FatFs
// core (ChaN, BSD-style license) as vendored via carlk3/no-OS-FatFS-SD-SPI-
// RPi-Pico, and the general approach (flash-backed FAT + USB MSC) following
// the structure of oyama/pico-usb-flash-drive (BSD license).
//
// Wiring (плата YD-RP2040):
//   GP28  -> CH32V003 PD1 (SWIO)  через резистор 100 Ом, + подтяжка 1 кОм к 3.3V
//   3V3   -> CH32V003 VDD
//   GND   -> CH32V003 GND
//   GP16  -> кнопка 1 -> GND  (прошить 1.bin)
//   GP17  -> кнопка 2 -> GND  (прошить 2.bin)
//   GP18  -> кнопка 3 -> GND  (прошить 3.bin)
//   GP19  -> кнопка backup/restore -> GND (коротко = снять backup.bin с чипа,
//            долго >=1.5с = записать backup.bin обратно на чип)
//   GP20  -> тумблер/джампер "писать в лог всегда/только аварии"
//            (GND = всегда; в резерве, физически можно не выводить наружу -
//            достаточно джампера на плате)
//   GP2-GP5 -> свободны (SD в этой версии не используется)
//   GP26, GP27 -> ЗАРЕЗЕРВИРОВАНЫ на будущее под возможный экран (I2C SDA/SCL)
//   GP23  -> встроенный WS2812 платы YD-RP2040 (если он есть на твоей плате)
//   GP22  -> дублирующий сигнал WS2812 - для плат без встроенного диода

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "PicoSWIO.h"
#include "RVDebug.h"
#include "WCHFlash.h"
#include "utils.h"
#include "Neopixel.h"

#include "ff.h"
#include "tusb.h"
#include "bsp/board.h"
#include "flash_diskio.h"

//------------------------------------------------------------------------------
// Config

const int PIN_SWIO       = 28;
const int PIN_BUTTON1    = 16; // -> 1.bin
const int PIN_BUTTON2    = 17; // -> 2.bin
const int PIN_BUTTON3    = 18; // -> 3.bin
const int PIN_BUTTON_BACKUP = 19; // короткое = бэкап, долгое (>=1.5с) = восстановить
const int PIN_LOG_SWITCH = 20; // GND = писать в лог всегда, иначе - только аварии
const int PIN_NEOPIXEL_1 = 23; // встроенный WS2812 (на платах, где он есть)
const int PIN_NEOPIXEL_2 = 22; // дублирующий выход для плат без встроенного WS2812

const int ch32v003_flash_size = 16 * 1024; // CH32V003F4P6 и т.п. Поправь под свой вариант.

const uint32_t MIN_BUSY_MS = 700;  // минимальная видимая длительность жёлтого
const uint32_t RESULT_MS   = 3000; // сколько держим итоговый цвет

// Цвета (0-255 на канал), яркость ~20-30% от максимума.
const uint8_t COL_IDLE[3]  = {0, 0, 51};    // синий, ~20%
const uint8_t COL_BUSY[3]  = {76, 76, 0};   // жёлтый, ~30%
const uint8_t COL_OK[3]    = {0, 76, 0};    // зелёный, ~30%
const uint8_t COL_ERR[3]   = {76, 0, 0};    // чистый красный, ~30%
const uint8_t COL_LOG[3]   = {25, 25, 25};  // белый - подтверждение дампа лога
const uint8_t COL_VIOLET[3] = {38, 0, 76};  // фиолетовый ближе к синему - метка restore
const uint8_t COL_OFF[3]   = {0, 0, 0};

Neopixel led1; // GP23
Neopixel led2; // GP22

static void show(const uint8_t c[3]) {
  led1.set_color(c[0], c[1], c[2]);
  led2.set_color(c[0], c[1], c[2]);
}

static bool full_log_enabled() { return gpio_get(PIN_LOG_SWITCH) == 0; }

static void blink(const uint8_t c[3], int times, int on_ms, int off_ms) {
  for (int i = 0; i < times; i++) {
    show(c);       sleep_ms(on_ms);
    show(COL_OFF); sleep_ms(off_ms);
  }
}

//------------------------------------------------------------------------------
// Хранилище прошивок - область внутренней flash Pico, отформатированная как
// FAT и одновременно (через usb_msc_driver.c) отдаваемая наружу по USB как
// обычная флешка. Компьютер и прошивка видят один и тот же реальный диск.

const uint32_t MAX_FW_SIZE = 16 * 1024; // с запасом под flash CH32V003
static uint8_t fw_buf[MAX_FW_SIZE];

static FATFS fs;
static bool fs_mounted = false;

static bool fs_mount() {
  FRESULT fr = f_mount(&fs, "0:", 1);
  if (fr == FR_NO_FILESYSTEM) {
    printf_g("// No filesystem found on internal flash - formatting...\n");
    static uint8_t work[FF_MAX_SS];
    MKFS_PARM opt = { FM_FAT, 0, 0, 0, 0 };
    fr = f_mkfs("0:", &opt, work, sizeof(work));
    if (fr != FR_OK) {
      printf_r("f_mkfs error: %d\n", (int)fr);
      return false;
    }
    fr = f_mount(&fs, "0:", 1);
  }
  if (fr != FR_OK) {
    printf_r("f_mount error: %d\n", (int)fr);
    return false;
  }
  printf_g("// Internal flash storage mounted OK\n");
  return true;
}

// Читает файл целиком в fw_buf. true - если получилось прочитать хоть что-то.
static bool fs_load_firmware(const char* filename, uint32_t* out_len) {
  *out_len = 0;
  if (!fs_mounted) {
    printf_r("Storage not mounted - can't read %s\n", filename);
    return false;
  }
  FIL fil;
  FRESULT fr = f_open(&fil, filename, FA_READ);
  if (fr != FR_OK) {
    printf_r("f_open(%s) error: %d\n", filename, (int)fr);
    return false;
  }
  UINT br = 0;
  fr = f_read(&fil, fw_buf, MAX_FW_SIZE, &br);
  f_close(&fil);
  if (fr != FR_OK) {
    printf_r("f_read(%s) error: %d\n", filename, (int)fr);
    return false;
  }
  if (br == 0) {
    printf_r("%s is empty\n", filename);
    return false;
  }
  *out_len = br;
  printf_g("// Loaded %s: %u bytes\n", filename, (unsigned)br);
  return true;
}

// Дописывает строку в log.txt. Хранилище внутреннее и всегда доступно (не
// removable, как SD), так что отдельного запасного канала логирования не
// требуется - log.txt и есть единственный лог.
static void fs_log_append(const char* text) {
  if (!fs_mounted) return;
  FIL fil;
  FRESULT fr = f_open(&fil, "log.txt", FA_OPEN_APPEND | FA_WRITE);
  if (fr != FR_OK && fr != FR_EXIST) return;
  f_printf(&fil, "%s", text);
  f_close(&fil);
}

// Сохраняет буфер fw_buf[0..len) в файл (перезаписывая, если уже есть).
static bool fs_save_firmware(const char* filename, uint32_t len) {
  if (!fs_mounted) {
    printf_r("Storage not mounted - can't save %s\n", filename);
    return false;
  }
  FIL fil;
  FRESULT fr = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (fr != FR_OK) {
    printf_r("f_open(%s, write) error: %d\n", filename, (int)fr);
    return false;
  }
  UINT bw = 0;
  fr = f_write(&fil, fw_buf, len, &bw);
  f_close(&fil);
  if (fr != FR_OK || bw != len) {
    printf_r("f_write(%s) error: %d\n", filename, (int)fr);
    return false;
  }
  printf_g("// Saved %s: %u bytes\n", filename, (unsigned)bw);
  return true;
}

//------------------------------------------------------------------------------

// Debounced "was this button just pressed" check. Active low.
// Ждёт, пока линия не будет стабильно "отпущена" (высокий уровень) минимум
// 30мс подряд - единичный кратковременный "выброс" из-за плохого контакта
// (дребезг посреди удержания, не только на самом нажатии) не считается
// настоящим отпусканием. Возвращает момент времени (time_us_32), когда линия
// реально в последний раз перешла в "отпущено" - для точного расчёта
// длительности удержания, без учёта времени самого подтверждения.
static uint32_t wait_for_stable_release(int pin) {
  uint32_t candidate = 0;
  bool have_candidate = false;
  while (true) {
    if (gpio_get(pin) != 0) {
      if (!have_candidate) { candidate = time_us_32(); have_candidate = true; }
      else if ((time_us_32() - candidate) / 1000 >= 30) return candidate;
    } else {
      have_candidate = false; // снова "нажата" - сброс кандидата на отпускание
    }
    sleep_ms(5);
  }
}

static bool button_pressed(int pin) {
  if (gpio_get(pin) != 0) return false;
  sleep_ms(30);
  if (gpio_get(pin) != 0) return false;
  wait_for_stable_release(pin); // 1 нажатие = 1 действие, дребезг не плодит лишних
  sleep_ms(30);
  return true;
}

// Кнопка backup двойного назначения: короткое нажатие -> снять backup.bin
// с чипа на SD, долгое (>=1.5с) -> записать backup.bin обратно на чип.
enum BackupBtnAction { BB_NONE, BB_SAVE, BB_RESTORE };

static BackupBtnAction backup_button_action() {
  if (gpio_get(PIN_BUTTON_BACKUP) != 0) return BB_NONE;
  sleep_ms(30);
  if (gpio_get(PIN_BUTTON_BACKUP) != 0) return BB_NONE; // шум

  uint32_t press_start = time_us_32();
  uint32_t release_time = wait_for_stable_release(PIN_BUTTON_BACKUP);
  uint32_t held_ms = (release_time - press_start) / 1000;
  sleep_ms(30);

  return held_ms >= 1500 ? BB_RESTORE : BB_SAVE;
}

//------------------------------------------------------------------------------

enum FlashResult { FLR_OK, FLR_NO_RESPONSE, FLR_WRITE_FAILED, FLR_VERIFY_FAIL, FLR_NO_FILE };

static const char* result_str(FlashResult r) {
  switch (r) {
    case FLR_OK:           return "OK";
    case FLR_NO_RESPONSE:  return "NO_RESPONSE";   // чип вообще не откликнулся
    case FLR_WRITE_FAILED: return "WRITE_FAILED";  // отвечал, но запись/стирание не прошли
    case FLR_NO_FILE:      return "NO_FILE";       // SD/файл недоступны
    default:              return "VERIFY_FAIL";   // записалось, но сверка не сошлась
  }
}

// ESIG-регистры CH32V00x: объём flash по факту в чипе + уникальный ID.
const uint32_t ADDR_ESIG_FLACAP = 0x1FFFF7E0;
const uint32_t ADDR_ESIG_UNIID1 = 0x1FFFF7E8;
const uint32_t ADDR_ESIG_UNIID2 = 0x1FFFF7EC;
const uint32_t ADDR_ESIG_UNIID3 = 0x1FFFF7F0;

static void log_chip_signature(RVDebug* rvd) {
  uint32_t flacap = rvd->get_mem_u32(ADDR_ESIG_FLACAP) & 0xFFFF; // КБ
  uint32_t uid1 = rvd->get_mem_u32(ADDR_ESIG_UNIID1);
  uint32_t uid2 = rvd->get_mem_u32(ADDR_ESIG_UNIID2);
  uint32_t uid3 = rvd->get_mem_u32(ADDR_ESIG_UNIID3);
  printf_g("// Chip: flash=%luKB uid=%08lx%08lx%08lx\n",
           (unsigned long)flacap, (unsigned long)uid1, (unsigned long)uid2, (unsigned long)uid3);
  if (flacap != (uint32_t)(ch32v003_flash_size / 1024)) {
    printf_r("// WARNING: chip reports %luKB flash, конфиг ожидает %dKB\n",
             (unsigned long)flacap, ch32v003_flash_size / 1024);
  }
}

// DMI-регистр DATA0 - scratch-регистр отладочного модуля, не завязан на
// реальную flash/память чипа. Настоящее двустороннее эхо не перепутать с
// "тишиной через подтяжку" при отключённом шлейфе.
const uint32_t LINK_TEST_PATTERN = 0xA5A55A5A;

static bool verify_real_link(RVDebug* rvd) {
  rvd->set_data0(LINK_TEST_PATTERN);
  uint32_t readback = rvd->get_data0();
  return readback == LINK_TEST_PATTERN;
}

//------------------------------------------------------------------------------

static uint32_t op_counter = 0;

// Пишет итог операции в log.txt на внутренней flash. Хранилище не removable
// (в отличие от SD) - отдельный запасной канал логирования не нужен.
static void log_result(uint32_t op_id, uint32_t t0, const char* what, FlashResult res) {
  if (!full_log_enabled() && res == FLR_OK) return; // тумблер выключен - только аварии

  char line[128];
  snprintf(line, sizeof(line), "#%lu t=%lus %s: %s (%lums)\n",
           (unsigned long)op_id, (unsigned long)(t0/1000000), what,
           result_str(res), (unsigned long)((time_us_32()-t0)/1000));

  fs_log_append(line);
}

static FlashResult flash_target(PicoSWIO* swio, RVDebug* rvd, WCHFlash* flash,
                                 const char* filename, const char* slot_name) {
  printf_g("\n// ---- Flashing %s (%s) ----\n", slot_name, filename);
  show(COL_BUSY);
  uint32_t t0 = time_us_32();
  uint32_t op_id = ++op_counter;

  FlashResult res = FLR_OK;
  uint32_t len = 0;

  if (!fs_load_firmware(filename, &len)) {
    res = FLR_NO_FILE;
  }

  // Полная переинициализация SWIO/DM перед каждой операцией - если
  // отладочный модуль на чипе застрял в непонятном состоянии после обрыва
  // связи, обычного halt() может не хватить, а этот сброс (с 8мкс
  // low-импульсом на линии) надёжно возвращает чистое состояние.
  if (res == FLR_OK) {
    swio->reset(PIN_SWIO);
    rvd->init();

    if (!rvd->halt() || !rvd->comm_ok || !verify_real_link(rvd)) {
      printf_r("chip not responding (halt/link check failed)\n");
      res = FLR_NO_RESPONSE;
    } else {
      log_chip_signature(rvd);
    }
  }

  if (res == FLR_OK) {
    flash->wipe_chip();
    if (!rvd->comm_ok) res = FLR_WRITE_FAILED;
  }

  if (res == FLR_OK) {
    flash->write_flash(0, (void*)fw_buf, len);
    if (!rvd->comm_ok) res = FLR_WRITE_FAILED;
  }

  if (res == FLR_OK) {
    bool verify_ok = flash->verify_flash(0, (void*)fw_buf, len);
    if (!rvd->comm_ok) {
      res = FLR_WRITE_FAILED;
    } else if (!verify_ok) {
      printf_r("verify_flash() mismatch!\n");
      res = FLR_VERIFY_FAIL;
    }
  }

  if (res != FLR_NO_RESPONSE && res != FLR_NO_FILE) {
    rvd->reset();
    rvd->resume();
  }

  uint32_t elapsed_ms = (time_us_32() - t0) / 1000;
  if (elapsed_ms < MIN_BUSY_MS) sleep_ms(MIN_BUSY_MS - elapsed_ms);

  printf_g("// Flash %s: %s\n", slot_name, result_str(res));
  log_result(op_id, t0, slot_name, res);

  return res;
}

// Снимает содержимое flash чипа и сохраняет как backup.bin на SD - "снял
// копию перед экспериментами, чтобы было куда вернуться". Восстановление
// делается отдельно, просто вызовом flash_target() с filename="backup.bin" -
// это та же самая операция записи, что и для 1.bin/2.bin/3.bin.
static FlashResult backup_target(PicoSWIO* swio, RVDebug* rvd, WCHFlash* flash) {
  printf_g("\n// ---- Backup ----\n");
  show(COL_BUSY);
  uint32_t t0 = time_us_32();
  uint32_t op_id = ++op_counter;

  if (!fs_mounted) {
    printf_r("Storage not mounted - can't save backup\n");
    uint32_t elapsed_ms = (time_us_32() - t0) / 1000;
    if (elapsed_ms < MIN_BUSY_MS) sleep_ms(MIN_BUSY_MS - elapsed_ms);
    log_result(op_id, t0, "BACKUP", FLR_NO_FILE);
    return FLR_NO_FILE;
  }

  swio->reset(PIN_SWIO);
  rvd->init();

  FlashResult res = FLR_OK;

  if (!rvd->halt() || !rvd->comm_ok || !verify_real_link(rvd)) {
    printf_r("chip not responding (halt/link check failed)\n");
    res = FLR_NO_RESPONSE;
  } else {
    log_chip_signature(rvd);
  }

  if (res == FLR_OK) {
    rvd->get_block_aligned(0x08000000, fw_buf, ch32v003_flash_size);
    if (!rvd->comm_ok) {
      res = FLR_WRITE_FAILED;
    } else if (!fs_save_firmware("backup.bin", ch32v003_flash_size)) {
      res = FLR_NO_FILE; // прочитали чип, но сохранить на SD не вышло
    }
  }

  if (res != FLR_NO_RESPONSE) {
    rvd->reset();
    rvd->resume();
  }

  uint32_t elapsed_ms = (time_us_32() - t0) / 1000;
  if (elapsed_ms < MIN_BUSY_MS) sleep_ms(MIN_BUSY_MS - elapsed_ms);

  printf_g("// Backup: %s\n", result_str(res));
  log_result(op_id, t0, "BACKUP", res);

  return res;
}

static void show_result(FlashResult res) {
  switch (res) {
    case FLR_OK:
      show(COL_OK);
      sleep_ms(RESULT_MS);
      break;
    case FLR_NO_RESPONSE:
    case FLR_NO_FILE:
      // Оба случая - "не смогли начать" (чипа нет ИЛИ файла/карты нет),
      // осознанно один и тот же жёлтый мигающий сигнал.
      blink(COL_BUSY, 6, 250, 250);
      break;
    case FLR_WRITE_FAILED:
      show(COL_ERR);
      sleep_ms(RESULT_MS);
      break;
    default: // FLR_VERIFY_FAIL
      blink(COL_ERR, 5, 300, 300);
      break;
  }
  show(COL_IDLE);
}

// Индикация именно для "снял backup" (не restore - restore это обычная
// запись, использует show_result() как и 1/2/3.bin). Успех сделан явно
// не похожим на обычную запись - это было чтение, а не запись, полезно
// не путать спросонья, что именно только что произошло.
static void show_backup_result(FlashResult res) {
  if (res == FLR_OK) {
    blink(COL_OK, 6, 150, 150); // быстрое мигание зелёным ~1.8с
    show(COL_OK);
    sleep_ms(2000);             // потом 2с ровно
  } else {
    show(COL_ERR);
    sleep_ms(RESULT_MS);
  }
  show(COL_IDLE);
}

// Индикация именно для "restore" - technически та же запись, что и
// 1/2/3.bin (через flash_target), но перед итоговым зелёным добавлена
// небольшая фиолетовая двойная вспышка - едва заметная метка "это было
// восстановление", ошибки показываются как обычно (без изменений).
static void show_restore_result(FlashResult res) {
  if (res == FLR_OK) {
    blink(COL_VIOLET, 2, 175, 175); // двойной блинк фиолетовым, ~0.7с
    show(COL_OK);
    sleep_ms(RESULT_MS);
  } else {
    show_result(res); // ошибки - как у обычной записи, без изменений
    return;
  }
  show(COL_IDLE);
}

//------------------------------------------------------------------------------

int main() {
  stdio_uart_init_full(uart0, 115200, 0, 1); // GP0/GP1, отладочный лог

  board_init();
  tud_init(BOARD_TUD_RHPORT); // USB MSC - показываем компьютеру ту же flash,
                              // что читает и пишет сама прошивка

  gpio_init(PIN_BUTTON1); gpio_set_dir(PIN_BUTTON1, GPIO_IN); gpio_pull_up(PIN_BUTTON1);
  gpio_init(PIN_BUTTON2); gpio_set_dir(PIN_BUTTON2, GPIO_IN); gpio_pull_up(PIN_BUTTON2);
  gpio_init(PIN_BUTTON3); gpio_set_dir(PIN_BUTTON3, GPIO_IN); gpio_pull_up(PIN_BUTTON3);
  gpio_init(PIN_BUTTON_BACKUP); gpio_set_dir(PIN_BUTTON_BACKUP, GPIO_IN); gpio_pull_up(PIN_BUTTON_BACKUP);
  gpio_init(PIN_LOG_SWITCH); gpio_set_dir(PIN_LOG_SWITCH, GPIO_IN); gpio_pull_up(PIN_LOG_SWITCH);

  led1.init(PIN_NEOPIXEL_1);
  led2.init(PIN_NEOPIXEL_2);
  show(COL_IDLE);

  printf_g("\n\n// ch32_flasher_flashdrive starting (internal flash: 1.bin/2.bin/3.bin)\n");

  fs_mounted = fs_mount();

  // Короткая белая вспышка - сигнал, что сейчас пойдёт дамп log.txt в UART -
  // и сам дамп: открываем log.txt и построчно печатаем в консоль. Не нужно
  // нажимать ничего отдельно, просто включил питание (или нажал reset) с
  // подключенным UART-переходником - лог сам появится в терминале.
  show(COL_LOG); sleep_ms(500); show(COL_IDLE);
  if (fs_mounted) {
    FIL fil;
    if (f_open(&fil, "log.txt", FA_READ) == FR_OK) {
      printf("\n===== log.txt =====\n");
      char buf[128];
      UINT br;
      while (f_read(&fil, buf, sizeof(buf) - 1, &br) == FR_OK && br > 0) {
        buf[br] = 0;
        printf("%s", buf);
      }
      printf("\n===== end log.txt =====\n");
      f_close(&fil);
    }
  }

  PicoSWIO* swio = new PicoSWIO();
  swio->reset(PIN_SWIO);

  RVDebug* rvd = new RVDebug(swio, 16);
  rvd->init();

  WCHFlash* flash = new WCHFlash(rvd, ch32v003_flash_size);
  flash->reset();

  printf_g("// Ready. B1(GP16)=1.bin  B2(GP17)=2.bin  B3(GP18)=3.bin  B4(GP19)=backup/restore\n");

  while (1) {
    tud_task(); // обслуживание USB MSC - вызывать регулярно

    if (button_pressed(PIN_BUTTON1)) {
      FlashResult r = flash_target(swio, rvd, flash, "1.bin", "slot1");
      show_result(r);
    }
    if (button_pressed(PIN_BUTTON2)) {
      FlashResult r = flash_target(swio, rvd, flash, "2.bin", "slot2");
      show_result(r);
    }
    if (button_pressed(PIN_BUTTON3)) {
      FlashResult r = flash_target(swio, rvd, flash, "3.bin", "slot3");
      show_result(r);
    }

    BackupBtnAction bb = backup_button_action();
    if (bb == BB_SAVE) {
      FlashResult r = backup_target(swio, rvd, flash);
      show_backup_result(r);
    } else if (bb == BB_RESTORE) {
      // Восстановление - та же самая операция записи, что и для 1/2/3.bin,
      // просто с именем файла backup.bin. Индикация - как у обычной записи,
      // но с небольшой фиолетовой меткой перед итоговым зелёным.
      FlashResult r = flash_target(swio, rvd, flash, "backup.bin", "restore");
      show_restore_result(r);
    }
  }

  return 0;
}
