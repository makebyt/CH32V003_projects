cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(ch32_flasher_flashdrive C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_executable(ch32_flasher_flashdrive
  src/main.cpp
  src/PicoSWIO.cpp
  src/RVDebug.cpp
  src/WCHFlash.cpp
  src/utils.cpp
  src/flash_diskio.c
  src/usb_descriptors.c
  src/usb_msc_driver.c
  src/ff15/ff.c
  src/ff15/ffsystem.c
  src/ff15/ffunicode.c
)

pico_generate_pio_header(ch32_flasher_flashdrive ${CMAKE_CURRENT_SOURCE_DIR}/src/singlewire.pio)
pico_generate_pio_header(ch32_flasher_flashdrive ${CMAKE_CURRENT_SOURCE_DIR}/src/ws2812.pio)

target_compile_definitions(ch32_flasher_flashdrive PRIVATE PARAM_ASSERTIONS_ENABLE_ALL=1)

target_include_directories(ch32_flasher_flashdrive PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ff15
)

pico_enable_stdio_usb(ch32_flasher_flashdrive 0)  # USB занят под MSC, консоль - на UART
pico_enable_stdio_uart(ch32_flasher_flashdrive 1)
pico_add_extra_outputs(ch32_flasher_flashdrive)

target_link_libraries(ch32_flasher_flashdrive
  pico_stdlib
  hardware_pio
  hardware_flash
  tinyusb_device
  tinyusb_board
)
