cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(ch32_flasher_demo C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party_FatFs_SPI build_FatFs_SPI)

add_executable(ch32_flasher_demo
  src/main.cpp
  src/PicoSWIO.cpp
  src/RVDebug.cpp
  src/WCHFlash.cpp
  src/utils.cpp
  src/hw_config.c
)

pico_generate_pio_header(ch32_flasher_demo ${CMAKE_CURRENT_SOURCE_DIR}/src/singlewire.pio)
pico_generate_pio_header(ch32_flasher_demo ${CMAKE_CURRENT_SOURCE_DIR}/src/ws2812.pio)

target_compile_definitions(ch32_flasher_demo PRIVATE PARAM_ASSERTIONS_ENABLE_ALL=1)

target_include_directories(ch32_flasher_demo PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

pico_enable_stdio_usb(ch32_flasher_demo 0)
pico_enable_stdio_uart(ch32_flasher_demo 1)
pico_add_extra_outputs(ch32_flasher_demo)

target_link_libraries(ch32_flasher_demo
  pico_stdlib
  hardware_pio
  hardware_flash
  FatFs_SPI
)
