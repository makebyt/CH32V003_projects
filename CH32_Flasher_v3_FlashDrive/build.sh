#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"

echo "=== Configuring and building (cmake) ==="
mkdir -p build
cmake -B build
cmake --build build -j4

echo
echo "=== Copying result ==="
cp build/ch32_flasher_flashdrive.uf2 .

echo
echo "Done: ch32_flasher_flashdrive.uf2 готов в корне проекта."
echo "Залей его на Pico через BOOTSEL."
