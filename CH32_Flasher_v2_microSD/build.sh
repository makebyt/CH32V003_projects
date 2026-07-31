#!/usr/bin/env bash
# cmake build -> uf2 in project root.
# Прошивки на SD-карту не входят в этот скрипт - просто копируешь
# 1.bin/2.bin/3.bin на карту отдельно, прошивка Pico от них не зависит.
set -e
cd "$(dirname "$0")"

echo "=== Configuring and building (cmake) ==="
mkdir -p build
cmake -B build
cmake --build build -j4

echo
echo "=== Copying result ==="
cp build/ch32_flasher_demo.uf2 .

echo
echo "Done: ch32_flasher_demo.uf2 готов в корне проекта."
echo "Залей его на Pico через BOOTSEL."
echo "Прошивки (1.bin/2.bin/3.bin) и log.txt - на SD-карте, отдельно от этой сборки."
