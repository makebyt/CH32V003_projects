# ch32_flasher_v2 (SD card)

A standalone CH32V003 flasher built on a **YD-RP2040** board. 4 buttons
(write 1.bin / 2.bin / 3.bin + backup/restore), onboard WS2812 shows status.
Target firmware images live on a microSD card — the RP2040's own firmware is
built once and left alone; to change what gets flashed onto the CH32V003 you
just copy a new `.bin` onto the card from any computer's file explorer.

Status: **verified on real hardware**, works reliably (write, verify,
erase, backup/restore, logging).

## Authors

- **Makebyt** — concept, requirements, hardware wiring, breadboarding,
  real-hardware testing, every bug found and every fix direction proposed
  along the way.
- **Claude (Anthropic)** — code implementation based on those requirements,
  and adaptation of third-party open-source libraries (see "Credits" below).

---

## Features

- 3 firmware slots (`1.bin`, `2.bin`, `3.bin`) plus a dedicated `backup.bin`
  slot
- Automatic verify after every write
- One-button backup/restore: short press — pull a copy of the chip's
  current firmware onto the card, long press (≥1.5s) — write it back
- Operation log in `log.txt` on the card (switch/jumper: log everything, or
  failures only)
- WS2812-only status indication — no text display, status is read from
  color and blink pattern
- "Real link" verification for the target chip (doesn't mistake a floating
  pulled-up line for an actual response)
- Full SWIO re-initialization before every operation, so the device doesn't
  get stuck after the debug wire is disconnected and reconnected

## Wiring

![Wiring diagram](scheme/scheme_v2.png)

```
YD-RP2040                  CH32V003 (target chip)
------------------------------------------------
GP16 --- Button 1 --- GND        (flash 1.bin)
GP17 --- Button 2 --- GND        (flash 2.bin)
GP18 --- Button 3 --- GND        (flash 3.bin)
GP19 --- Backup/restore button --- GND
GP20 --- Switch/jumper --- GND    (log: always / errors only)

GP28 ---[100 ohm]---+------------- PD1 (SWIO)
                     |
                  [1 kohm]
                     |
3V3 -----------------+---------------- VDD
GND ---------------------------------- GND

GP23 - onboard WS2812 (if present on your board)
GP22 - mirrored WS2812 output (for boards without an onboard LED)
GP26, GP27 - reserved for a possible future display

           microSD module (SPI)
GP2  --- SCK
GP3  --- MOSI
GP4  --- MISO
GP5  --- CS
3V3  --- VCC   (try VBUS/5V instead if the module doesn't work reliably)
GND  --- GND
```

The 100 ohm series resistor plus the 1 kohm pull-up on the SWIO line were
chosen to balance short-circuit protection against clean signal edges on a
single bidirectional wire.

Full YD-RP2040 board pinout reference (for locating physical pins) —
`scheme/YD-RP2040_black.png` (black board revision) and
`scheme/RP2040_green.png` (green revision, pinout may differ).

## Status LED

| Color | When |
|---|---|
| dim blue | idle |
| yellow | write in progress (held for at least 0.7s) |
| green, 3s | write succeeded |
| red, solid, 3s | chip responded, but write/erase itself failed |
| red, blinking, 3s | write succeeded, but verify didn't match |
| yellow, blinking ~2/s | couldn't even start (chip not responding OR file missing) |
| green, fast blink then solid | backup pulled from chip successfully |
| violet double-blink, then green | restore completed successfully |

## Changing firmware

1. Remove the SD card (or use a card reader if it's already in the device).
2. Copy your file onto the card under the right name: `1.bin`, `2.bin`, or
   `3.bin`.
3. Put the card back in — no need to rebuild or reflash the Pico.

## Building (one-time)

```
./build.sh     # Linux/macOS
build.bat      # Windows
```

Both scripts configure and build the cmake project, dropping a ready
`ch32_flasher_v2.uf2` in the project root — flash it onto the Pico via
BOOTSEL.

A prebuilt `.uf2` is also included under `prebuilt/`.

## Troubleshooting

- Yellow blinking — either the chip isn't responding (wiring / power /
  pull-up resistor) or the SD card didn't mount / the file is missing —
  check the UART log (GP0, 115200 8N1).
- Red blinking (verify fail) — the chip is reachable, but something's off
  with the write: shorten the wire, check the shared GND, add a 100nF
  decoupling cap near the CH32V003.
- `ch32v003_flash_size` in `src/main.cpp` is set to 16KB — adjust it if
  your specific chip variant has a different flash size.
- **Known unfixed bug**: after physically removing and reinserting the SD
  card, the device sometimes doesn't detect it again without a Pico reset
  (press the reset button on the board).

## Credits and third-party components

This project is built on top of a few open-source libraries — the core
low-level code (talking SWIO, driving the RISC-V Debug Module, reading the
SD card) wasn't written from scratch, it's adapted from existing open
projects:

- **[PicoRVD](https://github.com/Community-PIO-CH32V/PicoRVD)** (MIT
  license, © 2023 aappleby) — the `PicoSWIO`, `RVDebug`, and `WCHFlash`
  modules: the SWIO physical layer over PIO, RISC-V Debug Module access,
  and CH32V003 flash erase/write/verify.
- **[pico-examples](https://github.com/raspberrypi/pico-examples)**
  (BSD-3-Clause, © 2020 Raspberry Pi (Trading) Ltd.) — the WS2812 PIO
  program.
- **[no-OS-FatFS-SD-SPI-RPi-Pico](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico)**
  (Apache License 2.0, © Carl John Kugler III) — the SPI SD card driver and
  its FatFs integration for the Pico SDK.
- **[FatFs](http://elm-chan.org/fsw/ff/00index_e.html)** (ChaN, BSD-style
  license) — the FAT filesystem itself, vendored as part of
  no-OS-FatFS-SD-SPI-RPi-Pico.
- **[Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)**
  (BSD-3-Clause) — the base RP2040 SDK.

The application logic (buttons, indication, backup/restore, link-failure
handling, automatic log dump, timing) was written from scratch for this
specific standalone-appliance use case.

## Brief development history

1. Proof of concept: a minimal prototype on a bare Pico, one firmware image
   baked into the code, one button and one LED — just to confirm SWIO would
   actually work between an RP2040 and a CH32V003 at all.
2. Three firmware slots, WS2812 status indication, log stored in the Pico's
   internal flash.
3. Found and fixed a bug: after `reset()`, the target core stays halted —
   a separate `resume()` call is required.
4. Found and fixed a bug: the SWIO interface wasn't being re-initialized
   before every operation, so the chip would sometimes get "stuck" after
   the debug wire was reconnected — fixed by doing a full re-init before
   every button press.
5. Switched from firmware-baked-into-the-code to an SD card — the
   Windows/Linux build scripts were reduced to a one-time Pico build.
6. Added backup/restore, split error reporting into three distinct
   categories (no response / command failed / verify mismatch), added a
   "real link" check via a scratch register (to avoid mistaking a floating
   pulled-up line for an actual response), tuned the color scheme and
   brightness.
7. Fixed button debounce on release (a long press was sometimes registered
   as a short one due to poor breadboard contact) — added a stable-release
   confirmation.
